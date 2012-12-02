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

#ifndef _EVENT_H
#define	_EVENT_H

//
//  libevent headers
//
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include "system.h"
#include "common.h"

//
//  libevent wrappers
//
namespace libevent
{
    class General
    {
    public:
        static void setSocketNonBlocking( sys::Socket& socket );
        static void initThreads();
    };
    

    enum Error
    {
        BindError,
        ConnectionError,
        AcceptError,
        GeneralError,
        ListenError

    };
    
    class Base
    {

    public:

        Base()
        : m_base( NULL ), m_started( false )
        {
            m_base = event_base_new();
        }


        void start( bool block = true )
        {
            m_started = true;
            event_base_dispatch( m_base );
        }

        void stop()
        {
            TRACE_ENTERLEAVE();
            
            if ( m_base && m_started )
            {
                event_base_loopexit( m_base, NULL );
            }

            m_started = false;
        }

        bool started() const
        {
            return m_started;
        }

        operator event_base*() const
        {
            return m_base;
        }

        ~Base()
        {
            stop();

            if ( m_base )
            {
                event_base_free( m_base );
            }
        }

        sys::Lock& lock()
        {
            return m_lock;
        }
        
        
    private:
         event_base* m_base;
         sys::Lock m_lock;
         bool m_started;
    };

    class Listener
    {
    public:
        Listener( unsigned  int port );
        void listen( const Base& base );
        
        virtual void onAccept() = 0;
        virtual ~Listener();

    protected:
        sys::Socket m_socket;
        
    private:
        static void onAcceptStatic( evutil_socket_t listener, short event, void *arg );
        
    private:
        struct event* m_listenerEvent;
        unsigned int m_port;
    };

    class Connection
    {
    public:


        Connection( sys::Socket* socket );
        virtual ~Connection();

        void assign( const Base& base );
        virtual void onRead();
        virtual void onWrite();
        virtual void onError( short error );

        
        unsigned int read( char* data, unsigned int length );
        

        void write( const char* data, unsigned int length );
        void writeFormat( const char* format, ... );
        void send();

        void close();
        virtual void onClose() = 0;
        
        char* readLine();
        unsigned int inputLength();
        unsigned int searchInput( const char* what, unsigned int length );

        void setReadTimeout( unsigned int value );
        void setWriteTimeout( unsigned int value );

        void flush();

        intptr_t id() const
        {
            return m_id;
        }


    protected:
        bool m_close;
        evbuffer* m_input;
        evbuffer* m_output;
        
    private:
        static void onReadStatic( bufferevent* bev, void* ctx );
        static void onWriteStatic( bufferevent* bev, void* ctx );
        static void onErrorStatic( bufferevent* bev, short error, void* ctx );


    private:
        bufferevent* m_handle;
        sys::Socket* m_socket;
        intptr_t m_id;

    };

   


}
#endif	/* _EVENT_H */

