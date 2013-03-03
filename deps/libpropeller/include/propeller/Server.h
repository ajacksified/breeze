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

namespace propeller
{
    class Connection;

    /**
     * Simple message struct
     */    
    struct Message
    {
    public: 
        Message( const char* _contents, unsigned int _length = 0 )
            : contents( _contents ), length( _length > 0 ? _length : strlen( _contents ) ) 
        {
            
        }
        
        const char* contents;
        unsigned int length;
    };
    
    /**
     * Generic request class
     */
    class Request
    {
    public:

        friend class Connection;

        virtual ~Request( );
        
        /**
         * Get request contents
         * @return pointer to buffer containing request contents
         */
        const char* body( ) const
        {
            return m_body;
        }

        /**
         * Get request timestamp 
         * @return request timestamp in milliseconds
         */
        unsigned int timestamp( ) const
        {
            return m_timestamp;
        }
        
    protected:
        Request( Connection& connection );
        virtual void parse( );
        
    protected:
        Connection& m_connection;
        char* m_body;
        
    private:
        unsigned int m_timestamp;
    };

    
    /**
     * Generic response class
     */
    class Response
    {
        friend class Connection;

    public:

        /**
         * Send character buffer 
         * @param body pointer character buffer to send
         * @param length length of the buffer pointed at by first parameter, or 0 if buffer contains zero terminated string
         */
        void write( const char* body = NULL, unsigned int length = 0 );
        
        virtual ~Response( );

    protected:
        Response( Connection& connection );

    protected:
        Connection& m_connection;
    };
    
        
    /**
     * Main server class that implements basic TCP server functionality.
     */
    class PROPELLER_API Server : public libevent::Listener, public sys::ThreadPool
    {
    public:
        
        /**
         * Event handler class
         */
        class EventHandler
        {
        public:

            EventHandler( )
            {
            }

            virtual ~EventHandler( )
            {
            }
            
            /**
             * Invoked when request needs to be processsed
             * @param request request object 
             * @param response response object
             * @param thread thread that is handling this request 
             */
            virtual void onRequest( const Request& request, Response& response, sys::ThreadPool::Worker& thread )
            {

            }
            
            /**
             * Invoked when the new thread is started
             * @param thread thread that has been started
             */
            virtual void onThreadStarted( sys::ThreadPool::Worker& thread )
            {

            }

            /**
             * Invoked when interval specified on one of added timers passes
             * @param interval interval set when creating timer (in seconds)
             * @param data argument passed to the timer during creation
             */
            virtual void onTimer( unsigned int interval, void* data )
            {

            }

        private:
            
        };

        /**
         * Server  constructor
         * @param port port to listen on
         * @param handler EventHandler object that will receive events
         */
        Server( unsigned int port, EventHandler& handler );
        virtual ~Server( );

        /**
         * Start server (blocking)
         */
        void start( );
        /**
         * Stop server
         */
        void stop( );
        /**
         * Add a timer
         * @param interval timer interval (in seconds)
         * @param data arbitrary data to pass to timer
         */
        void addTimer( unsigned int interval, void* data = NULL );

        
        /**
         * Send message to all clients connected
         * @param message message to send
         */
        void broadcast( const Message& message );
        
        /**
         * Connection thread
         */
        class ConnectionThread : public sys::Thread
        {
            friend class Server;
            friend class Connection;
            
        public:
            Server& server( )
            {
                return m_server;
            }
            
        private:
            ConnectionThread( Server& server );
            virtual ~ConnectionThread( );
            
            void add( Connection* connection );
            void remove( Connection* connection, bool needDelete = false );
            void broadcast( const Message& message );
            
            

            const libevent::Base& base( ) const
            {
                return m_base;
            }
            
        protected:
            virtual void routine( );

        private:
            libevent::Base m_base;
            Server& m_server;
            std::map< intptr_t, Connection* > m_connections;
            sys::Lock m_lock;

        };

        /**
         * Set connection thread count
         * @param connectionThreadCount number of connection threads to serve all connections (at least one connection thread is started)
         */
        void setConnectionThreadCount( unsigned int connectionThreadCount )
        {
            m_connectionThreadCount = connectionThreadCount;
        }

        /**
         * Get number of connection threads
         * @return number of connection threads
         */
        unsigned int getConnectionThreadCount( ) const
        {
            return m_connectionThreadCount;
        }
        
        /**
         * Set number of threads in the pool 
         * @param poolThreadCount number of threads in the pool that processes requests (set to 0 to process requests synchronously, i.e in connection threads)
         */
        void setPoolThreadCount( unsigned int poolThreadCount )
        {
            m_poolThreadCount = poolThreadCount;
        }

        /**
         * Get number of threads in the pool
         * @return  number of threads in the pool
         */
        unsigned int getPoolThreadCount( ) const
        {
            return m_poolThreadCount;
        }

        /**
         * Set connection read timeout (if connection is not available for reading after timeout elapses, it is closed)
         * @param timeout read timeout in seconds
         */
        void setConnectionReadTimeout( unsigned int timeout )
        {
            m_connectionReadTimeout = timeout;
        }

        /**
         * Set connection write timeout (if connection is not available for writing after timeout elapses, it is closed)
         * @param timeout write timeout in seconds
         */        
        void setConnectionWriteTimeout( unsigned int timeout )
        {
            m_connectionWriteTimeout = timeout;
        }

        /**
         * Get connection read timeout
         * @return read timeout in seconds
         */
        unsigned int getConnectionReadTimeout( ) const
        {
            return m_connectionReadTimeout;
        }
        
        /**
         * Get connection write timeout
         * @return write timeout in seconds
         */
        unsigned int getConnectionWriteTimeout( ) const
        {
            return m_connectionWriteTimeout;
        }
        
        /**
         * Sets whether to keep the references to all connections (this may affect perfomance for large n umber of connections)
         * @param storeConnections flag to store connections (default false)
         */
        void setStoreConnections( bool storeConnections )
        {
            m_storeConnections = storeConnections;
        }
        
        /**
         * Get store connections setting
         * @return store connections setting
         */
        unsigned int getStoreConnections() const
        {
            return m_storeConnections;
        }
        
        /**
         * Get associated EventHandler
         * @return EventHandler reference
         */
        EventHandler& eventHandler()
        {
            return m_eventHandler;
        }
        
        /**
         * Process request
         * @param request
         * @param response
         */
        void process( Request* request, Response* response );
        
    protected:
        
        struct Task : public sys::ThreadPool::Task
        {
            Task( Request* request, Response * response );
            virtual ~Task( );
            Request* request;
            Response* response;
        };

        //
        //  sys::ThreadPool methods implementation
        //
        virtual void onTaskProcess( sys::ThreadPool::Task* task, sys::ThreadPool::Worker& thread );
        virtual void onThreadStart( sys::ThreadPool::Worker& thread );
     
        virtual Connection* newConnection( ConnectionThread& thread, sys::Socket* socket ); 
        
    private:
        //
        //  libevent::Listener method implementation
        //
        virtual void onAccept( );
        
        class TimerThread : public sys::Thread, public libevent::Timer
        {
            friend class Server;
        private:

            TimerThread( Server& server );
            
            virtual void routine( );
            virtual void onTimer( unsigned int interval, void* data );

        private:
            libevent::Base m_base;
            Server& m_server;
        };
        

    private:
        libevent::Base m_base;
        std::list< ConnectionThread* > m_connectionThreads;
        TimerThread*  m_timerThread;
        
        unsigned int m_connectionThreadCount;
        unsigned int m_connectionReadTimeout;
        unsigned int m_connectionWriteTimeout;
        unsigned int m_poolThreadCount;
        
        EventHandler& m_eventHandler;
        bool m_storeConnections;
        sys::Lock m_lock;
    };
    
    /**
     * Class that represent the client connection
     */
    class Connection : public libevent::Connection
    { 
       friend class Request;
       friend class Response;
       
    public:
        
        /**
         * Exception class
         */
        class Exception
        {
        public: 
            enum Type
            {
                RequestError,
                RequestNotComplete
            };
            
            Exception( Type type )
            : m_type( type )
            {
                
            }
            
            const Type& type() const
            {
                return m_type;
            }
            
        private:
            Type m_type;
        };
        
        Connection( Server::ConnectionThread& thread, sys::Socket* socket );
        virtual ~Connection( );
        virtual void onRead( );
        virtual void onClose( );
        virtual void onWrite( );

        /**
         * Sets flag to close the connection
         */
        void setClose( )
        {
            m_needClose = true;
        }

        /**
         * Get connection close flag
         * @return connection close flag
         */
        bool getClose( ) const
        {
            return m_needClose;
        }

        virtual Response* createResponse( const Exception* exception = NULL );
        virtual Request* createRequest( );
        virtual void process( Request* request, Response* response );
        
    private:
        void checkDelete( );
        
        //
        //  increase reference count
        //

        void ref( );
        
        //
        //  decrease reference count
        //
        void deref( bool threadsafe = false );
        
    protected:
        Request* m_request;
        Server::ConnectionThread& m_thread;

    private:
        
        bool m_needClose;
        bool m_initialized;
        unsigned int m_ref;
    };

}
#endif //_PROPELLER_SERVER_H_