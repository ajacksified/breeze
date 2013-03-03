/* 
 * File:   HttpServer.h
 * Author: sergey
 *
 * Created on February 16, 2013, 1:18 PM
 */

#ifndef HTTPSERVER_H
#define	HTTPSERVER_H

#include "Server.h"
#include "HttpProtocol.h"

namespace propeller
{
    namespace http
    {
        class Connection;

        /**
         * HTTP request class         
         */
        class Request : public propeller::Request
        {
            friend class Connection;
            
        public:
            /**
             * Get HTTP method
             * @return HTTP method string
             */
            const char* method( ) const
            {
                return m_method;
            }

            /**
             * Get request URI
             * @return string containing the request URI
             */
            const char* uri( ) const
            {
                return m_uri;
            }
    
            /**
             * Get request protocol 
             * @return string containing the request protocol
             */
            const char* protocol( ) const
            {
                return m_protocol;
            }

            /**
             * Get header value
             * @param name header name
             * @return header value
             */
            const char* header( const char* name ) const;

            typedef std::map< std::string, std::string > HeaderMap;

            /**
             * Get all request headers
             * @return HeaderMap with headers
             */
            const HeaderMap& headers( ) const
            {
                return m_headers;
            }
            
        private:
            Request( Connection& connection );
            virtual void parse( );

        private:
            char m_method[64];
            char m_uri[128];
            char m_protocol[64];
            char m_header[2048];

            HeaderMap m_headers;
            unsigned int m_length;
            unsigned int m_bodyLength;
            bool m_parsedHeader;
            unsigned int m_read;
        };

        /**
         * HTTP response class 
         */
        class Response : public propeller::Response
        {
            friend class Connection;

        public:

            /**
             * Get response status code
             * @return status code number
             */
            unsigned int status( ) const
            {
                return m_status;
            }

            /**
             * Set status code
             * @param status HTTP status code number
             */
            void setStatus( unsigned int status )
            {
                m_status = status;
            }

            /**
             * Add header
             * @param name header name
             * @param value header value
             */
            void addHeader( const char* name, const char* value );
            /**
             * Set body
             * @param body pointer to buffer 
             * @param length if length is 0 then body is assumed to be zero terminated string otherwise length of buffer passed as first parameter
             */
            void setBody( const char* body = NULL, unsigned int length = 0 );

        private:
            Response( Connection& connection, unsigned int status = HttpProtocol::Ok );
            void init( );

        private:
            unsigned int m_status;
            bool m_init;
            
        };  

        /**
         *  HTTP connection
         */
        class Connection : public propeller::Connection
        {
            friend class Request;
            friend class Response;

        public:
            Connection( Server::ConnectionThread& thread, sys::Socket* socket )
            : propeller::Connection( thread, socket )
            {
                
            }
            
            class Exception : public propeller::Connection::Exception
            {
            public:
                Exception( HttpProtocol::Status status );
                
                const HttpProtocol::Status status() const
                {
                    return m_status;
                }
                
            private:
                HttpProtocol::Status m_status;
            };
            
            virtual propeller::Response* createResponse( const propeller::Connection::Exception* exception = NULL );
            virtual propeller::Request* createRequest( );
            virtual void process( propeller::Request* request, propeller::Response* response );
        };
        
        /**
         * HTTP server
         */
        class PROPELLER_API Server : public propeller::Server
        {
        public:
            
            /**
             * Constructor
             * @param port listening port
             * @param eventHandler EventHandler instance. References to propeller::http::Request and propeller::http::Response objects will be passed to propeller::Server::EventHandler::onRequest method
             */
            Server( unsigned int port, EventHandler& eventHandler )
            : propeller::Server( port, eventHandler )
            {
                
            }
            
        protected:
                virtual propeller::Connection* newConnection( ConnectionThread& thread, sys::Socket* socket );
            
        private:

        };

    }
};

#endif	/* HTTPSERVER_H */
