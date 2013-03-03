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

#include <event2/thread.h>

#include "event.h"
#include "common.h"

//
//	Trace function
//
#include "trace.h"
#include "event.h"


namespace libevent
{
    static bool s_initThreads = false;

    void General::setSocketNonBlocking( sys::Socket& socket )
    {
        evutil_make_socket_nonblocking( socket.s() );
    }

    void General::initThreads()
    {
        if ( s_initThreads )
        {
            return;
        }

        s_initThreads = true;
            
#ifdef WIN32
     evthread_use_windows_threads();
#else
     evthread_use_pthreads();
#endif
     
     //event_enable_debug_logging(EVENT_DBG_ALL);
    }
    
    void General::shutdown()
    {
        libevent_global_shutdown();
    }

    Base::Base( )
    : m_base( NULL ), m_started( false )
    {
        m_base = event_base_new( );

        evthread_make_base_notifiable( m_base );
    }
    void Base::start( bool )
    {
        m_started = true;

        event_base_loop( m_base, EVLOOP_NO_EXIT_ON_EMPTY );
    }
    void Base::stop( ) const
    {
        TRACE_ENTERLEAVE( );

        if ( m_base )
        {
            event_base_loopexit( m_base, NULL );
        }

    }
    
    Base::~Base( )
    {
        stop( );

        if ( m_base )
        {
            event_base_free( m_base );
        }
    }

    Listener::Listener( unsigned int port )
    : m_listenerEvent( NULL ), m_port( port )
    {
        
    }

    void Listener::listen( const Base& base)
    {
        TRACE_ENTERLEAVE();

        General::setSocketNonBlocking( m_socket );

        if ( m_socket.bind( m_port ) == sys::Socket::StatusFailed )
        {
            TRACE_ERROR( "cannot bind to socket, error %d", sys::General::getLastError() );
            throw BindError;
        }
        
        if ( m_socket.listen() == sys::Socket::StatusFailed )
        {
            throw ListenError;
        }

        m_listenerEvent = event_new( base, m_socket.s(), EV_READ | EV_PERSIST, onAcceptStatic,  this );

        if ( !m_listenerEvent )
        {
            throw GeneralError;
        }

        event_add( m_listenerEvent, NULL );
    }

    Listener::~Listener()
    {
        if ( m_listenerEvent )
        {
            event_free( m_listenerEvent );
        }
    }

    void Listener::onAcceptStatic( int listener, short event, void* arg )
    {
        (( Listener* ) arg)->onAccept();
    }

    Connection::Connection( sys::Socket* socket, const Base& base )
    : m_socket( socket ), m_handle( NULL ), m_close( false ), m_id( ( intptr_t ) this ),  m_deleteEvent( NULL ), m_base( base )
    {
        TRACE_ENTERLEAVE();
        General::setSocketNonBlocking( *socket );
        
        m_handle = bufferevent_socket_new( m_base, m_socket->s(), BEV_OPT_THREADSAFE );

        if ( !m_handle )
        {
            TRACE_ERROR( "failed to create buffered event", "" );
            throw GeneralError;
        }
        
        bufferevent_setcb( m_handle, onReadStatic, onWriteStatic, onErrorStatic, this );
        
        m_input = bufferevent_get_input( m_handle );
        m_output = bufferevent_get_output( m_handle );
     }

    Connection::~Connection()
    {
        TRACE_ENTERLEAVE();
        
        if ( m_handle )
        {
            bufferevent_free( m_handle );
        }
        
        if ( m_socket )
        {
            delete m_socket;
        }
        
        if ( m_deleteEvent )
        {
            event_free( m_deleteEvent );
        }
    }
    
    void Connection::enable()
    {
        if ( bufferevent_enable( m_handle, EV_READ | EV_WRITE  ) != 0 ) 
        {
            TRACE_ERROR( "bufferevent_enable failed", "" );
            throw GeneralError;
        }
    }
   

    unsigned int Connection::read( char* data, unsigned int length )
    {
        return bufferevent_read( m_handle, data, length );
    }
    
    void Connection::write( const char* data, unsigned int length, bool close )
    {
        TRACE("%s", data );
        bufferevent_write( m_handle, data, length );
    
        m_close = close;
    }

    void Connection::send()
    {
        evbuffer_add_buffer( bufferevent_get_output( m_handle ), m_output );
    }

    void Connection::writeFormat( const char* format, ... )
    {
        TRACE_ENTERLEAVE();
        
        va_list args;
        va_start(args, format);

        vprintf( format, args );

        int written = evbuffer_add_vprintf( m_output, format, args );

        va_end(args);
    }

    void Connection::close()
    {
        TRACE_ENTERLEAVE();
        disable();
        
        onClose();
    }   
    
    void Connection::onError( short error )
    {
        TRACE_ENTERLEAVE();

        if ( error & BEV_EVENT_EOF || error & BEV_EVENT_ERROR)
        {
            close();
            return;
        }

        if ( error & BEV_EVENT_TIMEOUT )
        {
            close();
        }
    }
    

    void Connection::onReadStatic( bufferevent* bev, void* ctx )
    {
        (( Connection* ) ctx)->onRead();
    }

    void Connection::onWriteStatic( bufferevent* bev, void* ctx )
    {
        (( Connection* ) ctx)->onWrite();
    }

    void Connection::onErrorStatic( bufferevent* bev, short error, void* ctx )
    {
        if ( !bufferevent_get_enabled( bev ) )
        {
            return;
        }

        (( Connection* ) ctx)->onError( error );
    }

    char* Connection::readLine()
    {
        return evbuffer_readln( m_input, NULL, EVBUFFER_EOL_CRLF );
    }

    unsigned int Connection::inputLength()
    {
        return evbuffer_get_length( m_input );
    }

    unsigned int Connection::searchInput( const char* what, unsigned int length )
    {
        evbuffer_ptr found = evbuffer_search( m_input, what, length, NULL );

        return found.pos;
    }

    void Connection::setReadTimeout( unsigned int value )
    {
        TRACE_ENTERLEAVE();

        timeval timeout;
        timeout.tv_sec = ( long ) value;
        timeout.tv_usec = 0;

        bufferevent_set_timeouts( m_handle, &timeout, NULL );
    }

    void Connection::flush()
    {
       bufferevent_flush( m_handle, EV_WRITE, BEV_FLUSH );
    }

    void Connection::setWriteTimeout( unsigned int value )
    {
        timeval timeout;
        timeout.tv_sec = ( long ) value;
        timeout.tv_usec = 0;

        bufferevent_set_timeouts( m_handle, NULL, &timeout );
    }
    
    
    void Connection::scheduleDelete()
    {
        if ( !m_deleteEvent )
        {
            //
            //  schedule deletion event
            //
            m_deleteEvent = evtimer_new( m_base, onDeleteStatic, this );
            struct timeval timeout = { 0, 0 };
            evtimer_add( m_deleteEvent, &timeout );
        }
    }
    
    void Connection::onDeleteStatic( evutil_socket_t fd, short what, void *arg )
    {
        ( ( Connection* ) arg )->onDelete();
    }
    
    bool Connection::enabled()
    {
        if ( bufferevent_get_enabled( m_handle ) == 0 )
        {
            return false;
        }
        
        return true;
        
    }
    
    void Connection::disable()
    {
        m_socket->shutdown();
        bufferevent_disable( m_handle, EV_READ | EV_WRITE );
    }
    
    void Connection::onDelete()
    {
        TRACE_ENTERLEAVE();
     
        delete this;
    }
    
    Timer::Timer( const Base& base )
    :  m_base( base )
    {
    }
    
    void Timer::add( unsigned int seconds, void* data )
    {
        struct timeval timeout = { seconds, 0 };
        
        event* timer = event_new( m_base, -1, EV_PERSIST, onTimerStatic, new Data( this, data, seconds ) );
        m_timers.push_back( timer );
        event_add( timer, &timeout );
    }
    
    Timer::~Timer()
    {
        while ( !m_timers.empty() )
        {
            event* timer = m_timers.front();
            Data* data = ( Data* ) event_get_callback_arg( timer );
            delete data;
            event_free( timer );
            m_timers.pop_front();
        }
    }
    
    void Timer::onTimerStatic( evutil_socket_t fd, short what, void *arg )
    {
        Data* data = ( Data* )arg;
        data->instance->onTimer( data->seconds, data->arg );
    }
}
