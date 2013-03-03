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
        static void shutdown();
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

        Base();
        ~Base();
                
        void start( bool block = true );
        void stop() const;
        
        bool started() const
        {
            return m_started;
        }

        operator event_base*() const
        {
            return m_base;
        }

    private:
         event_base* m_base;
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
        Timer( const Base& base );
        void add( unsigned int seconds, void* data );
        ~Timer();
        static void onTimerStatic( evutil_socket_t fd, short what, void *arg );
        virtual void onTimer( unsigned int seconds, void* data = NULL ) = 0;
        
    private:
        struct Data
        {
            Data( Timer* _instance, void* _arg, unsigned int _seconds )
            : instance( _instance), arg(_arg), seconds( _seconds ) 
            {
            }
            
            Timer* instance;
            void* arg;
            unsigned int seconds;
        };
      
    private: 
        const Base& m_base;
        std::list< event* > m_timers;
    };

    class Connection
    {
    public:
        Connection( sys::Socket* socket, const Base& base );
        virtual ~Connection();

        void enable( );
        virtual void onRead()
        {
            
        }
        
        virtual void onWrite()
        {
            
        }
        
        virtual void onError( short error );

        unsigned int read( char* data, unsigned int length );
        

        void write( const char* data, unsigned int length, bool close = false );
        void writeFormat( const char* format, ... );
        void send();
        void close();
        
        virtual void onClose()
        {
            
        }
        
        char* readLine();
        unsigned int inputLength();
        unsigned int searchInput( const char* what, unsigned int length );

        void setReadTimeout( unsigned int value );
        void setWriteTimeout( unsigned int value );
        
        void scheduleDelete();
        bool enabled();
        void disable();
        
        void flush();

        intptr_t id() const
        {
            return m_id;
        }


    protected:
        
        evbuffer* m_input;
        evbuffer* m_output;
        bool m_close;
        
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
        event* m_deleteEvent;
        const Base& m_base;
    };

   


}
#endif	/* _EVENT_H */

