/* 
 * File:   HttpServer.cpp
 * Author: sergey
 * 
 * Created on February 16, 2013, 1:18 PM
 */

#include "HttpServer.h"

//
//	Trace function
//
#include "trace.h"


namespace propeller
{
    
    namespace http
    {

        propeller::Connection* Server::newConnection( ConnectionThread& thread, sys::Socket* socket )
        {
            return new http::Connection( thread, socket );
        }

        Connection::Exception::Exception( HttpProtocol::Status status )
        : propeller::Connection::Exception( status == HttpProtocol::Ok ? Exception::RequestNotComplete : Exception::RequestError ), m_status( status )
        {

        }

        propeller::Response* Connection::createResponse( const propeller::Connection::Exception* exception )
        {
            TRACE_ENTERLEAVE();
            
            HttpProtocol::Status status = HttpProtocol::Ok;
            
            if ( exception )
            {
                status = ( ( Exception* ) exception )->status( );
            }
            
            TRACE("status %d", status);

            Response* response = new Response( *this, status );
            if ( status != HttpProtocol::Ok )
            {
                response->setBody( );
            }
            
            return response;
        }
        
        propeller::Request* Connection::createRequest()
        {
            TRACE_ENTERLEAVE();
            
            return new Request( *this );
        }
        
        void Connection::process( propeller::Request* request, propeller::Response* response )
        {
            TRACE_ENTERLEAVE( );

            const char* header = ( ( Request* ) request )->header( "connection" );
            if ( header && strcmp( header, "close" ) == 0 )
            {
                setClose( );
            }

            m_thread.server().process( request, response );
        }

        const char* Request::header( const char* name ) const
        {
            std::string lowercaseName = name;
            std::transform( lowercaseName.begin( ), lowercaseName.end( ), lowercaseName.begin( ), ::tolower );

            HeaderMap::const_iterator i = m_headers.find( lowercaseName );

            if ( i != m_headers.end( ) )
            {
                return i->second.c_str( );
            }

            return NULL;
        }

        void Request::parse( )
        {

            //
            //	try to parse out buffer
            //

            if ( !strlen( m_method ) )
            {
                //
                //	check request line
                //
                char* requestLine;

                requestLine = m_connection.readLine( );

                if ( !requestLine )
                {
                    free( requestLine );
                    throw Connection::Exception( HttpProtocol::BadRequest );
                }

                int read = sscanf( requestLine, "%s %s %s", m_method, m_uri, m_protocol );

                if ( read != 3 )
                {
                    free( requestLine );
                    throw Connection::Exception( HttpProtocol::BadRequest );
                }

                TRACE( "%s", requestLine );

                free( requestLine );
            }

            if ( !m_parsedHeader )
            {
                //
                //  see if we have all headers
                //
                if ( m_connection.searchInput( "\r\n\r\n", 4 ) == -1 )
                {
                    throw Connection::Exception( HttpProtocol::Ok );
                }

                //
                //	parse headers
                //
                for (;; )
                {

                    char* line;

                    line = m_connection.readLine( );

                    if ( !line )
                    {
                        free( line );
                        throw Connection::Exception( HttpProtocol::BadRequest );
                    }

                    const char* separator = strstr( line, ": " );

                    if ( !separator )
                    {
                        free( line );
                        break;
                    }

                    std::string name;
                    unsigned long nameLength = separator - line;
                    name.append( line, nameLength );
                    const char* end = line + strlen( line );
                    unsigned int separatorLength = 2;
                    std::string value;
                    unsigned long valueLength = end - ( separator + separatorLength );
                    value.append( separator + separatorLength, valueLength );

                    //
                    //  lowercase
                    //
                    std::transform( name.begin( ), name.end( ), name.begin( ), ::tolower );
                    std::transform( value.begin( ), value.end( ), value.begin( ), ::tolower );

                    m_headers[ name ] = value;
                    free( line );
                    TRACE( "%s: %s", name.c_str( ), value.c_str( ) );
                }
            }

            m_parsedHeader = true;

            //	body is required for PUT and POST method requests
            //
            bool needBody = m_method[0] == 'P';

            //
            //	check request headers
            //
            if ( needBody )
            {
                const char* length = header( "content-length" );

                if ( length )
                {
                    //
                    //	Get body length
                    //
                    sscanf( length, "%d", &m_bodyLength );
                }
                else
                {
                    //
                    //	Aways require content length
                    //
                    throw Connection::Exception( HttpProtocol::LengthRequired );
                }

                if ( !m_body )
                {
                    m_body = new char[ m_bodyLength + 1];
                    m_body[ m_bodyLength ] = 0;
                }

                m_read += m_connection.read( m_body + m_read, m_bodyLength - m_read );

                if ( m_read != m_bodyLength )
                {
                    //
                    //  wait for more data
                    //
                    throw Connection::Exception( HttpProtocol::Ok );
                }
            }
        }

        Request::Request( Connection& connection )
        : m_bodyLength( 0 ),  m_parsedHeader( false ), m_read( 0 ), propeller::Request( connection )
        {
            TRACE_ENTERLEAVE( );

            m_method[0] = 0;
            m_uri[0] = 0;
            m_protocol[0] = 0;
            m_header[0] = 0;
        }

        Response::Response( Connection& connection, unsigned int status )
        : m_status( status ), propeller::Response( connection ), m_init( false )
        {
            TRACE_ENTERLEAVE( );
        }

        void Response::init( )
        {
            if ( m_init )
            {
                return;
            }

            m_init = true;

            TRACE_ENTERLEAVE( );

            char responseLine[64] = "0";

            sprintf( responseLine, "HTTP/1.1 %u %s\r\n", m_status, HttpProtocol::reason( m_status ).c_str( ) );
            TRACE( "%s", responseLine );

            m_connection.write( responseLine, strlen( responseLine ) );

            char serverVersion[32] = "\0";
            sprintf( serverVersion, "%s/%s", PROPELLER_NAME, PROPELLER_VERSION );
            addHeader( "Server", serverVersion );
        }

        void Response::addHeader( const char* name, const char* value )
        {
            init( );

            m_connection.write( name, strlen( name ) );
            m_connection.write( ": ", 2 );
            m_connection.write( value, strlen( value ) );
            m_connection.write( "\r\n", 2 );
        }

        void Response::setBody( const char* body, unsigned int length )
        {
            init( );

            if ( m_status > 400 )
            {
                m_connection.setClose( );
            }

            if ( !body )
            {
                addHeader( "Content-Length", "0" );
                m_connection.write( "\r\n", 2, m_connection.getClose( ) );

                return;
            }

            char buffer[16];
            sprintf( buffer, "%u", length ? length : ( unsigned int ) strlen( body ) );
            addHeader( "Content-Length", buffer );
            m_connection.write( "\r\n", 2 );
            m_connection.write( body, length ? length : strlen( body ), m_connection.getClose( ) );

        }
    }

}
