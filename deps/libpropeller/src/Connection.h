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

class Connection;

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
            return m_empty;
        }

        return m_body;
    }

    const char* protocol( ) const
    {
        return m_protocol;
    }

    const std::string& header( const std::string& name ) const;

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
    static std::string s_empty;
    char m_empty[2];
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
    void setBody( const char* body = NULL );

    ~Response( );
    
    void complete();

private:

    Response( Connection& connection, unsigned int status = HttpProtocol::Ok );
    void init( );

private:
    unsigned int m_status;
    Connection& m_connection;
    bool m_wroteBody;
    bool m_init;

};

class Connection : public libevent::Connection
{
    friend class Request;
    friend class Response;

public:

    Connection( Server::ConnectionThread& thread, Server& server, sys::Socket* socket );
    virtual ~Connection( );
    virtual void onRead( );
    virtual void onClose( );

    

    Request* request( )
    {
        return m_request;
    }

    void ref()
    {
        sys::General::interlockedIncrement( &m_ref );
    }

    void setDelete()
    {
        m_delete = true;
    }

    void tryDelete()
    {
        TRACE("%d %d" , m_delete , m_ref);
        
        if ( m_delete && !m_ref )
        {
            delete this;
        }
    }

    void deref()
    {
        sys::General::interlockedDecrement( &m_ref );
        tryDelete();
    }




private:
    Request* m_request;
    Server::ConnectionThread& m_thread;
    Server& m_server;
    unsigned int m_ref;
    bool m_delete;

};
#endif	/* _CONNECTION_H */

