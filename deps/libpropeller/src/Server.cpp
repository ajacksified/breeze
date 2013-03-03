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

#include "Server.h"

//
//	Trace function
//
#include "trace.h"

namespace propeller
{
    Connection::Connection( Server::ConnectionThread& thread, sys::Socket* socket )
    : libevent::Connection( socket, thread.base( ) ), m_request( NULL ), m_thread( thread ), m_needClose( false ), m_initialized( false ), m_ref( 0 )
    {
        TRACE_ENTERLEAVE( );
        ref( );
    }

    Connection::~Connection( )
    {
        TRACE_ENTERLEAVE( );

        
        m_thread.remove( this );
        
        if ( m_request )
        {
            delete m_request;
        }
        
        
    }
    
    

    void Connection::onWrite( )
    {
        TRACE_ENTERLEAVE( );

        if ( !m_initialized )
        {
            //
            //  set timeouts if needed
            //
            if ( m_thread.server( ).getConnectionReadTimeout( ) > 0 )
            {
                setReadTimeout( m_thread.server( ).getConnectionReadTimeout( ) );
            }

            if ( m_thread.server( ).getConnectionWriteTimeout( ) > 0 )
            {
                setWriteTimeout( m_thread.server( ).getConnectionWriteTimeout( ) );
            }
        }

        if ( m_close )
        {
            disable( );
            onClose( );
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
                 m_request = createRequest();
            }
               

            //
            //  try to parse request
            //
            m_request->parse( );

            //
            //  dispatch request for processing
            //
            ref( );

            Response* response = createResponse();

            process( m_request, response );

            m_request = NULL;
        }
        catch ( const Exception& exception )
        {
            if ( exception.type() == Exception::RequestError )
            {
                delete m_request;
                m_request = NULL;

                Response* response = createResponse( &exception );
                delete response;
            }
        }
    }
    
                
    void Connection::process( Request* request, Response* response )
    {
        m_thread.server().process( request, response );
    }
    
    void Connection::ref( )
    {
        TRACE_ENTERLEAVE( );

        sys::General::interlockedIncrement( &m_ref );
    }
    
    void Connection::deref( bool threadsafe )
    {
        TRACE_ENTERLEAVE( );

        unsigned int count = sys::General::interlockedDecrement( &m_ref );

        if ( count == 1 )
        {
            //
            //  schedule deletion
            //  
            if ( threadsafe )
            {
                scheduleDelete( );
            }
            else
            {
                delete this;
            }
        }
    }
    
    Response* Connection::createResponse( const Exception* exception )
    {
        TRACE_ENTERLEAVE();
        
        return new Response( *this );
    }
    
    Request* Connection::createRequest( )
    {
        return new Request( *this );
    }

    void Connection::checkDelete( )
    {
        TRACE_ENTERLEAVE( );

        deref( true );
    }

    void Connection::onClose( )
    {
        TRACE_ENTERLEAVE( );

        deref( );
    }

    Request::~Request( )
    {
        TRACE_ENTERLEAVE( );

        if ( m_body )
        {
            delete[] m_body;
        }
    }

    void Request::parse( )
    {

        m_body = m_connection.readLine( );

        if ( !m_body )
        {
            throw Connection::Exception( Connection::Exception::RequestError );
        }
    }

    Request::Request( Connection& connection )
    : m_body( NULL ), m_connection( connection )
    {
        TRACE_ENTERLEAVE( );

        m_timestamp = sys::General::getMillisecondTimestamp( );
    }

    Response::Response( Connection& connection )
    : m_connection( connection )
    {
        TRACE_ENTERLEAVE( );
    }

    Response::~Response( )
    {
        TRACE_ENTERLEAVE( );

        m_connection.checkDelete( );
    }

    void Response::write( const char* body, unsigned int length )
    {
        m_connection.write( body, strlen( body ) );
    }

    Server::Server( unsigned int port, EventHandler& handler )
    : libevent::Listener( port ), m_connectionThreadCount( 10 ), m_connectionReadTimeout( 0 ), 
      m_connectionWriteTimeout( 0 ),  m_poolThreadCount( 25 ),  m_eventHandler( handler ), m_timerThread( NULL ), m_storeConnections( false )
    {
        TRACE_ENTERLEAVE( );

        libevent::General::initThreads( );
    }

    Server::~Server( )
    {
        TRACE_ENTERLEAVE( );

        stop( );

        libevent::General::shutdown( );
    }

    void Server::stop( )
    {
        TRACE_ENTERLEAVE( );

        if ( !m_base.started( ) )
        {
            return;
        }

        m_base.stop( );

        for ( std::list< ConnectionThread* >::iterator i = m_connectionThreads.begin( ); i != m_connectionThreads.end( ); i++ )
        {
            ( *i )->base( ).stop( );
        }

        //
        //      stop thread pool
        //
        sys::ThreadPool::stop();
        

        while ( !m_connectionThreads.empty( ) )
        {
            ConnectionThread* thread = m_connectionThreads.front( );

            delete thread;

            m_connectionThreads.pop_front( );
        }

        if ( m_timerThread )
        {
            delete m_timerThread;
        }
    }

    void Server::start( )
    {
        TRACE_ENTERLEAVE( );

        //
        //  bind to port listening 
        //
        listen( m_base );

        //
        //  start thread pool
        //
        sys::ThreadPool::start( m_poolThreadCount );

        if ( m_timerThread )
        {
            m_timerThread->start();
        }

        //
        //  start event base 
        //
        m_base.start( );
    }

    void Server::onAccept( )
    {
        TRACE_ENTERLEAVE( );

        //
        //  accept new connection and add it to connection thread
        //
        sys::Socket* socket = m_socket.accept( );
        
        if ( !socket )
        {
            TRACE_ERROR( "accept failed, error %d", sys::Socket::getLastError( ) );
            return;
        }

        //
        //  get first connection thread
        //
        ConnectionThread* thread;
        
        {
            sys::LockEnterLeave lock( m_lock );
            
            if ( m_connectionThreads.size() < m_connectionThreadCount )
            {
                thread = new ConnectionThread( *this );
            }
            else
            {
                thread = m_connectionThreads.front( );
                m_connectionThreads.pop_front( );
            }
        }
        

        //
        //  create new connection  and assign it to connection thread. it mantains referencre count and will delete itself when no longer referenced
        //

        Connection* connection = newConnection( *thread, socket );

        if ( m_storeConnections )
        {
            thread->add( connection );
        }

        //
        //  enable events processing on this connection
        //  
        connection->enable( );
        
        //
        //  add connection thread to back
        //
        {
            sys::LockEnterLeave lock( m_lock );
            m_connectionThreads.push_back( thread );
        }
        
    }

    void Server::broadcast( const Message& message )
    {
        TRACE_ENTERLEAVE();
        
        if ( !m_storeConnections || m_connectionThreads.empty() )
        {
            return;
        }
        
        sys::LockEnterLeave lock( m_lock ); 
    
        for ( std::list< ConnectionThread* >::iterator i = m_connectionThreads.begin( ); i != m_connectionThreads.end( ); i++ )
        {
            ( *i )->broadcast( message );
        }
        
    }
    
    void Server::process( Request* request, Response* response )
    {
        TRACE_ENTERLEAVE( );

        queue( new Task( request, response ) );
    }

    void Server::addTimer( unsigned int interval, void* data )
    {
        TRACE_ENTERLEAVE( );
        
        //
        //  create timer thread
        //
        if ( !m_timerThread )
        {
            m_timerThread = new TimerThread( *this );
        }

        m_timerThread->add( interval, data );
    }

    Server::ConnectionThread::ConnectionThread( Server& server )
    : m_server( server )
    {
        TRACE_ENTERLEAVE( );

        m_stackSize = 1024 * 1024 * 2;

        start( );
    }

    void Server::ConnectionThread::add( Connection* connection )
    {
        TRACE_ENTERLEAVE( );

        sys::LockEnterLeave lock( m_lock );

        m_connections[ connection->id( ) ] = connection;
    }

    void Server::ConnectionThread::remove( Connection* connection, bool needDelete )
    {
        TRACE_ENTERLEAVE( );
        
        if ( !m_server.getStoreConnections() )
        {
            return;
        }

        sys::LockEnterLeave lock( m_lock );

        std::map< intptr_t, Connection* >::iterator found = m_connections.find( connection->id( ) );
        if ( found != m_connections.end( ) )
        {
            m_connections.erase( found );

            if ( needDelete )
            {
                delete connection;
            }
        }
    }

    void Server::ConnectionThread::broadcast( const Message& message )
    {
        TRACE_ENTERLEAVE();
        
        sys::LockEnterLeave lock( m_lock );
        
        TRACE( "%d", m_connections.size() );
        
        for ( std::map< intptr_t, Connection* >::iterator i = m_connections.begin(); i != m_connections.end(); i++ )
        {
            i->second->write( message.contents, message.length );
        }
    }
    
    void Server::ConnectionThread::routine( )
    {
        TRACE_ENTERLEAVE( );

        //
        //  start event loop
        //
        m_base.start( );
    }

    Server::ConnectionThread::~ConnectionThread( )
    {
        TRACE_ENTERLEAVE( );

        m_base.stop();

        while ( !m_connections.empty( ) )
        {
            remove( m_connections.begin( )->second, true );
        }
    }

    Server::Task::Task( Request* _request, Response* _response )
    : request( _request ), response( _response )
    {
    }

    Server::Task::~Task( )
    {
        delete request;
        delete response;
    }

    void Server::onTaskProcess( sys::ThreadPool::Task* task, sys::ThreadPool::Worker& thread )
    {
        TRACE_ENTERLEAVE( );

        Task* serverTask = ( Task* ) task; 

        //
        //  invoke callback
        //
        eventHandler().onRequest( *serverTask->request, *serverTask->response, thread );      
        delete serverTask;
    }
    
    void Server::onThreadStart( sys::ThreadPool::Worker& thread )
    {
        eventHandler().onThreadStarted( thread );
    }

    Server::TimerThread::TimerThread( Server& server )
    : Timer( m_base ), m_server( server )
    {
        
    }

    void Server::TimerThread::routine( )
    {
        TRACE_ENTERLEAVE( );

        m_base.start( );
    }

    void Server::TimerThread::onTimer( unsigned int interval, void* data )
    {
        TRACE_ENTERLEAVE( );

        //
        //      call back
        //
        m_server.eventHandler().onTimer( interval, data );
    }
    
    Connection* Server::newConnection( ConnectionThread& thread, sys::Socket* socket )
    {
        return new Connection( thread, socket );
    }
}