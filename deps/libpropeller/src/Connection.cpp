/*
Copyright 2012 Sergey Zavadski

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */

#include "Connection.h"

namespace propeller
{

Connection::Connection( Server::ConnectionThread& thread, Server::ProcessPool& pool, sys::Socket* socket )
 : libevent::Connection( socket, thread.base() ), m_request( NULL ), m_thread( thread ),  m_needClose( false ), m_pool( pool ),  m_initialized( false ), m_ref( 0 )
 {
    TRACE_ENTERLEAVE();
    ref();
    
 }

Connection::~Connection()
{
    TRACE_ENTERLEAVE();
    
    if ( m_request )
    {
        delete m_request;
    }
}

void Connection::onWrite( )
{
    TRACE_ENTERLEAVE();
    
    if ( !m_initialized )
    {
        //
        //  set timeouts if needed
        //
        if ( m_thread.server( ).connectionReadTimeout( ) > 0 )
        {
            setReadTimeout( m_thread.server( ).connectionReadTimeout( ) );
        }

        if ( m_thread.server( ).connectionWriteTimeout( ) > 0 )
        {
            setWriteTimeout( m_thread.server( ).connectionWriteTimeout( ) );
        }
    }
    
    if ( m_close )
    {
        disable();
        onClose();
    }
}

//
//  on read callback  
//
void Connection::onRead( )
{
    TRACE_ENTERLEAVE( );
    
    try
    {
        if ( !m_request )
        {
            try
            {
                m_request = new Request( *this );
            }
            catch (...)
            {
                TRACE_ERROR("failed to allocate memory", "");
                Response response( *this, 500 );
                response.setBody();
                return;
            }
                    
            
        }

        //
        //  try to parse request
        //
        m_request->parse();

        //
        //  dispatch request for processing
        //
        ref();
        
        Response* response = NULL;

        try
        {
            response = new Response( *this, HttpProtocol::Ok );
        }
        catch (...)
        {
            TRACE_ERROR("failed to allocate memory", "");
            Response response( *this, 500 );
            response.setBody();
            return;
        }
        
        m_pool.process( m_request, response );

        const char* header = m_request->header( "connection" );

        if ( header && strcmp( header, "close" ) == 0 ) ///|| strstr( m_request->protocol(), "1.0" ) )
        {
            m_needClose = true;
        }

        m_request = NULL;
    }
    catch ( HttpProtocol::Status status )
    {
        TRACE("error: %d", status );

        if ( status != HttpProtocol::Ok )
        {
            delete m_request;

            m_request = NULL;
            
            {
                Response response( *this, status );
                response.setBody();
            }

        }
    }
}

void Connection::checkDelete()
{
    TRACE_ENTERLEAVE();
    
    deref( true );
}

void Connection::onClose( )
{
    TRACE_ENTERLEAVE();
    
    deref();
}

Request::~Request()
{
    TRACE_ENTERLEAVE();
    
    //
    //  set close connection flag if needed
    //
    if ( m_body )
    {
        delete[] m_body;
    }
}

const char* Request::header( const char* name ) const
{
    std::string lowercaseName = name;
    std::transform( lowercaseName.begin( ), lowercaseName.end( ), lowercaseName.begin( ), ::tolower );
    
    HeaderMap::const_iterator i = m_headers.find( lowercaseName );

    if ( i != m_headers.end( ) )
    {
       return i->second.c_str();
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

        if ( !requestLine  )
        {
            free( requestLine );
            throw HttpProtocol::BadRequest;
        }

        int read = sscanf( requestLine, "%s %s %s", m_method, m_uri, m_protocol );

        if ( read != 3 )
        {
            free( requestLine );
            throw HttpProtocol::BadRequest;
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
            throw HttpProtocol::Ok;
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
                throw HttpProtocol::BadRequest;
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
            throw HttpProtocol::LengthRequired;
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
            throw HttpProtocol::Ok;
        }
    }
}


Request::Request( Connection& connection )
: m_bodyLength( 0 ), m_body( NULL ), m_connection( connection ), m_parsedHeader( false ), m_read( 0 )
{
    TRACE_ENTERLEAVE( );

    m_method[0] = 0;
    m_uri[0] = 0;
    m_protocol[0] = 0;
    m_header[0] = 0;
    
    m_timestamp = sys::General::getMillisecondTimestamp();
}


Response::Response( Connection& connection, unsigned int status )
: m_status( status ), m_connection( connection ), m_init( false )
{
    TRACE_ENTERLEAVE( );
}

Response::~Response( )
{
    TRACE_ENTERLEAVE();
    
    m_connection.checkDelete( );
 }

void Response::init( )
{
    if ( m_init )
    {
        return;
    }

    m_init = true;

    TRACE_ENTERLEAVE();

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
    init();
    
    m_connection.write( name, strlen( name ) );
    m_connection.write( ": ", 2 );
    m_connection.write( value, strlen( value ) );
    m_connection.write( "\r\n", 2 );
}

void Response::setBody( const char* body, unsigned int length  )
{
    init();
    
    if ( m_status > 400 )
    {
        m_connection.setClose();
    }
    
    if ( !body )
    {
        addHeader( "Content-Length", "0");
        m_connection.write( "\r\n", 2, m_connection.needClose() );
        
        return;
    }

    char buffer[16];
    sprintf( buffer, "%u", length ? length : ( unsigned int ) strlen( body ) );
    addHeader( "Content-Length", buffer );
    m_connection.write( "\r\n", 2 );
    m_connection.write( body, length ? length : strlen( body ), m_connection.needClose() );
    
}

}