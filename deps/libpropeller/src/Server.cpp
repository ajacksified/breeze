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
#include "Connection.h"
#include "HttpProtocol.h"

namespace propeller
{

Server::Server( unsigned int port )
: libevent::Listener( port ), m_connectionThreadCount( 10 ), m_connectionReadTimeout( 0 ), m_connectionWriteTimeout( 0 ),  m_pool( *this ), m_poolThreadCount( 25 ), m_timerThread( NULL )
{
    TRACE_ENTERLEAVE( );

    libevent::General::initThreads( );

    HttpProtocol::initialize( );
}

Server::~Server( )
{
    TRACE_ENTERLEAVE( );

    stop( );
    
    libevent::General::shutdown();
}

void Server::stop( )
{
    TRACE_ENTERLEAVE( );

    if ( !m_base.started() )
    {
        return;
    }

    m_base.stop( );

    for ( std::list< ConnectionThread* >::iterator i = m_connectionThreads.begin(); i != m_connectionThreads.end(); i++ )
    {
        (*i)->base().stop();
    }
    
    m_pool.stop();

    while ( !m_connectionThreads.empty() )
    {
        ConnectionThread* thread = m_connectionThreads.front();

        delete thread;

        m_connectionThreads.pop_front();
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
    //  start connection threads
    //
    for ( int count = 0; count < m_connectionThreadCount; count++ )
    {
        m_connectionThreads.push_back( new ConnectionThread( *this ) );
    }

    //
    //  start process pool
    //
    m_pool.start( m_poolThreadCount );
    
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
    ConnectionThread* thread = m_connectionThreads.front( );
    m_connectionThreads.pop_front( );
    
    //
    //  create new connection  and assign it to connection thread. it mantains referencre count and will delete itself when no longer referenced
    //
    try
    {
        Connection* connection = new Connection( *thread, m_pool, socket );
        connection->enable();
        
        #ifdef _PROPELLER_DEBUG
            thread->add( connection );
        #endif            
    }
    catch (...)
    {
        
    }
    
    //
    //  add connection thread to back
    //
    m_connectionThreads.push_back( thread );
}

void Server::process( Request* request, Response* response )
{
    TRACE_ENTERLEAVE();

    m_pool.process( request, response );
}

void Server::setTimer( unsigned int interval, TimerCallback callback, void* data )
{
    TRACE_ENTERLEAVE();
    
    m_timerThread = new TimerThread( interval, callback, data );
}


Server::ConnectionThread::ConnectionThread( Server& server )
: m_server( server )
{
    TRACE_ENTERLEAVE();
    
    m_stackSize = 1024 * 1024 * 2;
    
    start();
}

void Server::ConnectionThread::add( Connection* connection )
{
    TRACE_ENTERLEAVE();
    
    sys::LockEnterLeave lock( m_lock );
    
    m_connections[ connection->id() ] = connection;
}

void Server::ConnectionThread::remove( Connection* connection, bool needDelete )
{
    TRACE_ENTERLEAVE();
    
    sys::LockEnterLeave lock( m_lock );
    
    std::map< intptr_t, Connection* >::iterator found = m_connections.find( connection->id() );
    if ( found != m_connections.end() )
    {
        m_connections.erase( found );
        
        if ( needDelete )
        {
            delete connection;
        }
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

    //m_base.stop();
    
    while ( !m_connections.empty() )
    {
        remove( m_connections.begin()->second, true );
    }
}

Server::ProcessPool::ProcessPool( Server& server )
    : m_server( server ), m_stop( false )
{
    TRACE_ENTERLEAVE();
}

Server::ProcessPool::~ProcessPool()
{
    TRACE_ENTERLEAVE();

    stop();

}

void Server::ProcessPool::process( Request* request, Response* response )
{
    TRACE_ENTERLEAVE();

    if ( m_stop )
    {
        return;
    }

    {
        sys::LockEnterLeave lock( m_lock );
        m_queue.push_back( new Context( request, response ) );
    }

    m_semaphore.post();
}

void Server::ProcessPool::start( unsigned int threads )
{
    TRACE_ENTERLEAVE();
    int i = 0;

    while ( i < threads )
    {
        Thread* thread = new Thread( *this );

        //
        //  invoke callback
        //
        const Server::Callback& callback = m_server.onThreadStartedCallback();

        if ( callback.callback )
        {
            void* threadData = NULL;

            ( ( Server::OnThreadStarted ) callback.callback )( &threadData, callback.data, thread->lock() );
            thread->setData( threadData );
        }

        m_threads.push_back( thread );
        i++;
        thread->start();
    }
}

void Server::ProcessPool::stop()
{
    TRACE_ENTERLEAVE();

    if ( m_stop )
    {
        return;
    }
    
    m_stop = true;
    
    {
        sys::LockEnterLeave lock( m_lock );

        while ( !m_queue.empty() )
        {
            Context* context = m_queue.front();
            delete context;
            m_queue.pop_front();
        }
    }

    m_semaphore.setValue( m_threads.size() );

    while ( !m_threads.empty() )
    {
        Thread* thread = m_threads.front();

        //
        //  invoke callback
        //
        const Server::Callback& callback = m_server.onThreadStoppedCallback();

        if ( callback.callback )
        {
            ( ( Server::ThreadCallback ) callback.callback )( thread->data(), callback.data );
        }

        delete thread;
        m_threads.pop_front();
    }
}

Server::ProcessPool::Context* Server::ProcessPool::get()
{
    TRACE_ENTERLEAVE();
    
    //
    //  wait for semaphore
    //
    m_semaphore.wait();
    
    if ( m_stop )
    {
        return NULL;
    }

    //
    //  return next context to process or NULL if the queue is empty
    //
    sys::LockEnterLeave lock( m_lock );
    
    if ( m_queue.empty() )
    {
        return NULL;
    }


    Context* context = m_queue.front();
    m_queue.pop_front();

    return context;
}

Server::ProcessPool::Context::Context( Request* _request, Response* _response )
: request( _request ), response( _response )
{
}

Server::ProcessPool::Context::~Context( )
{
    delete request;
    delete response;
}

void Server::ProcessPool::handle( Context* context, const Thread& thread )
{
    TRACE_ENTERLEAVE();

    const Server::Callback& callback = m_server.onRequestCallback();
    
    //
    //  invoke callback
    //
    ( ( Server::OnRequest ) callback.callback )( context->request, context->response, callback.data, thread.data() );

    delete context;

    const Server::Callback& afterCallback = m_server.afterRequestCallback();

    if ( afterCallback.callback )
    {
        ( ( Server::ThreadCallback ) afterCallback.callback )( callback.data, thread.data( ));
    }
}

Server::ProcessPool::Thread::Thread( ProcessPool& pool )
    : m_pool( pool ), m_data( NULL )
{
    TRACE_ENTERLEAVE();
    
    m_stackSize = 1024 * 1024 * 4;
}

Server::ProcessPool::Thread::~Thread( )
{
    TRACE_ENTERLEAVE();
}

void Server::ProcessPool::Thread::routine()
{
    TRACE_ENTERLEAVE();

    for ( ;; )
    {
        Context* context = m_pool.get( );

        if ( context )
        {
            //
            //  process data
            //
            m_pool.handle( context, *this );
        }
        else
        {
            return;
        }
    }
}

Server::TimerThread::TimerThread( unsigned int interval, TimerCallback callback, void* data )
: Timer( interval ), m_callback( ( void* ) callback, data )
{
    assign( m_base );
}

void Server::TimerThread::routine()
{
    TRACE_ENTERLEAVE();
    
    m_base.start();
}

void Server::TimerThread::onTimer()
{
    TRACE_ENTERLEAVE();
    
    ( ( TimerCallback ) m_callback.callback )( m_callback.data );
}

}