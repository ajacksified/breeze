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

#ifndef _CONNECTION_H
#define	_CONNECTION_H

#include "Server.h"
#include "HttpProtocol.h"

//
//	HTTP request
//

class Connection : public libevent::Connection
{
    friend class Request;
    friend class Response;

public:

    Connection( const Server::ConnectionThread& thread, Server::ProcessPool& pool, sys::Socket* socket );
    virtual ~Connection( );
    virtual void onRead( );
    virtual void onClose( );
    
    Request* request( )
    {
        return m_request;
    }

    //
    //  increase reference count
    //
    void ref()
    {
        TRACE_ENTERLEAVE();
                
        sys::General::interlockedIncrement( &m_ref );
    }

    //
    //  decrease reference count
    //
    void deref()
    {
        TRACE_ENTERLEAVE();
        
        unsigned int count = sys::General::interlockedDecrement( &m_ref );
                
        if ( count == 1 ) 
        {
            //
            //  schedule deletion
            //  
            scheduleDelete();
        }
        
    }
    
    bool needClose() const
    {
        return m_needClose;
    }


private:
    Request* m_request;
    const Server::ConnectionThread& m_thread;
    Server::ProcessPool& m_pool;
    
    unsigned int m_ref;
    bool m_needClose;
    
};

class Request
{
public:

    Request( Connection& connection );
    ~Request( );

    void parse( );

    const char* method( ) const
    {
        return m_method;
    }

    const char* uri( ) const
    {
        return m_uri;
    }

    const char* body( ) const
    {
        if ( !m_bodyLength )
        {
            return NULL;
        }

        return m_body;
    }

    const char* protocol( ) const
    {
        return m_protocol;
    }

    const char* header( const char* name ) const;

    typedef std::map< std::string, std::string > HeaderMap;

    const HeaderMap& headers( ) const
    {
        return m_headers;
    }
    
    


private:
    char m_method[64];
    char m_uri[128];
    char m_protocol[64];
    char m_header[2048];

    HeaderMap m_headers;
    unsigned int m_length;
    char* m_body;
    unsigned int m_bodyLength;
    bool m_parsedHeader;
    unsigned int m_read;
    Connection& m_connection;
};

//
//	HTTP response
//

class Response
{
    friend class Connection;

public:

    unsigned int status( ) const
    {
        return m_status;
    }

    void setStatus( unsigned int status )
    {
        m_status = status;
    }

    void addHeader( const char* name, const char* value );
    void setBody( const char* body = NULL, unsigned int length = 0 );
    
    void complete();
    
    ~Response( );
private:

    Response( Connection& connection, unsigned int status = HttpProtocol::Ok );
    void init( );

private:
    unsigned int m_status;
    Connection& m_connection;
    bool m_init;
};


#endif	/* _CONNECTION_H */

