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


#ifndef _PROPELLER_SERVER_H_
#define _PROPELLER_SERVER_H_

#include "common.h"
#include "event.h"

class Connection;
class Request;
class Response;


class Server : public libevent::Listener
{
public:
    
    Server( unsigned int port );
    virtual ~Server( );

    void start();
    void stop();

    //
    //  libevent::Listener method implementation
    //
    virtual void onAccept();

    struct Callback
    {
        Callback()
        : callback( NULL ), data( NULL )
        {

        }

        void* callback;
        void* data;
    };

    //
    //  Handler callback
    //
    typedef void ( API_CALL *OnRequest ) ( const Request* request, Response* response, void* data, void* threadData );

    //
    //  Thread callbacks
    //
    typedef void ( API_CALL *OnThreadStarted ) ( void** threadData, void* data );

    typedef void ( API_CALL *OnThreadStopped ) ( void* threadData, void* data );

    void setOnRequestCallback( OnRequest callback, void* data = NULL )
    {
        m_onRequestCallback.callback = ( void* ) callback;
        m_onRequestCallback.data = data;
    }

    void setOnThreadStartedCallback( OnThreadStarted callback, void* data )
    {
        m_onThreadStartedCallback.callback = ( void* ) callback;
        m_onThreadStartedCallback.data = data;
    }

    void setOnThreadStoppedCallback( OnThreadStopped callback, void* data )
    {
        m_onThreadStoppedCallback.callback = ( void* ) callback;
        m_onThreadStoppedCallback.data = data;
    }

    const Callback& onRequestCallback() const
    {
        return m_onRequestCallback;
    }

    const Callback& onThreadStartedCallback() const
    {
        return m_onThreadStartedCallback;
    }

    const Callback& onThreadStoppedCallback() const
    {
        return m_onThreadStoppedCallback;
    }

    void process( Request* request, Response* response );

    void remove( Connection* );

     libevent::Base& base()
     {
            return m_base;
     }


    class ConnectionThread : public sys::Thread
    {
    public:
        ConnectionThread( Server& server );
        virtual ~ConnectionThread();
        
        void add( Connection* connection );
        void remove( Connection* connection );
        
        const Server& server()
        {
            return m_server;
        }

        libevent::Base& base()
        {
            return m_base;
        }



    protected:
        virtual void routine();

    private:
        libevent::Base m_base;
        sys::Event m_event;
        typedef std::map< intptr_t, Connection* > ConnectionsMap;
        ConnectionsMap m_connections;
        sys::Lock m_lock;
        Server& m_server;
    };

    void setConnectionThreadCount( unsigned int connectionThreadCount )
    {
        m_connectionThreadCount = connectionThreadCount;
    }

    unsigned int connectionThreadCount() const
    {
        return m_connectionThreadCount;
    }

    void setPoolThreadCount( unsigned int poolThreadCount )
    {
        m_poolThreadCount = poolThreadCount;
    }
    
    unsigned int poolThreadCount()
    {
        return m_poolThreadCount;
    }

    void setConnectionReadTimeout( unsigned int timeout )
    {
        m_connectionReadTimeout = timeout;
    }
    
    void setConnectionWriteTimeout( unsigned int timeout )
    {
        m_connectionWriteTimeout = timeout;
    }

    unsigned int connectionReadTimeout() const
    {
        return m_connectionReadTimeout;
    }
    
    unsigned int connectionWriteTimeout() const
    {
        return m_connectionWriteTimeout;
    }

protected:
    void addConnection( Connection* connection );
    void removeConnection( Connection* connection );
    
    class ProcessPool
    {
        friend class Thread;

    public:
        ProcessPool( Server& server );
        ~ProcessPool();
        void process( Request* request, Response* response );
        
        struct Context
        {
            Context( Request* request, Response* response );
            ~Context();
            Request* request;
            Response* response;
        };

        void start( unsigned int threads );
        void stop();

    private:
        class Thread: public sys::Thread
        {
        public:
            Thread( ProcessPool& pool );
            virtual ~Thread();
            virtual void routine();

            void* data() const
            {
                return m_data;
            }
            
            void setData( void* data )
            {
                m_data = data;
            }
            
        private:
            ProcessPool& m_pool;
            void* m_data;
            
        };

        void wait()
        {
            //m_event.wait();
            m_semaphore.wait();
        }
        
        Context* get();
        bool needStop() const
        {
            return m_stop;
        }
        
        void handle( Context* context, const Thread& thread );

    private:
        std::deque< Context* > m_queue;
        std::list< Thread* > m_threads;
        sys::Lock m_lock;
        sys::Semaphore m_semaphore;
        Server& m_server;
        bool m_stop;
    };

private:
    libevent::Base m_base;
    std::deque< ConnectionThread* > m_connectionThreads;
    unsigned int m_connectionThreadCount;

    unsigned int m_connectionReadTimeout;
    unsigned int m_connectionWriteTimeout;
    
    Callback m_onRequestCallback;
    Callback m_onThreadStartedCallback;
    Callback m_onThreadStoppedCallback;

    unsigned int m_poolThreadCount;
    ProcessPool m_pool;
};

#endif //_PROPELLER_SERVER_H_