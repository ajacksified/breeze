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
#include <event2/thread.h>


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
            
//             event_config* cfg = event_config_new();
//
//            event_config_require_features(cfg, EV_FEATURE_O1);
//
//            m_base = event_base_new_with_config(cfg);
//            
//            event_config_free(cfg);
            
            evthread_make_base_notifiable( m_base );
        }


        void start( bool block = true )
        {
            m_started = true;
            
            event_base_loop( m_base, EVLOOP_NO_EXIT_ON_EMPTY );
        }
        
        void rescan() const
        {
            event_base_loopcontinue( m_base );
        }

        void stop() const
        {
            TRACE_ENTERLEAVE();
            
            if ( m_base )
            {
                event_base_loopexit( m_base, NULL );
            }

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
    
    class Timer
    {
    public:
        Timer( unsigned int seconds );
        void assign( const Base& base );
        ~Timer();
        static void onTimerStatic( evutil_socket_t fd, short what, void *arg );
        virtual void onTimer() = 0;
        
    private: 
        struct event* m_timer;
        unsigned int m_seconds;
    };

    class Connection
    {
    public:


        Connection( sys::Socket* socket, const Base& base );
        virtual ~Connection();

        void assign( const Base& base );
        virtual void onRead();
        virtual void onWrite();
        virtual void onError( short error );

        
        unsigned int read( char* data, unsigned int length );
        

        void write( const char* data, unsigned int length, bool close = false );
        void writeFormat( const char* format, ... );
        void send();

        void close();
        virtual void onClose();
        
        char* readLine();
        unsigned int inputLength();
        unsigned int searchInput( const char* what, unsigned int length );

        void setReadTimeout( unsigned int value );
        void setWriteTimeout( unsigned int value );
        
        void scheduleDelete();
        
        void flush();

        intptr_t id() const
        {
            return m_id;
        }


    protected:
        evbuffer* m_input;
        evbuffer* m_output;
        
    private:
        static void onReadStatic( bufferevent* bev, void* ctx );
        static void onWriteStatic( bufferevent* bev, void* ctx );
        static void onErrorStatic( bufferevent* bev, short error, void* ctx );
        static void onDeleteStatic( evutil_socket_t fd, short what, void *arg );
        void onDelete();
                
    private:
        bufferevent* m_handle;
        sys::Socket* m_socket;
        intptr_t m_id;
        bool m_closed;
        event* m_deleteEvent;
        const Base& m_base;
        bool m_close;
    };

   


}
#endif	/* _EVENT_H */

