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
     TRACE("init threads", "");
     evthread_use_pthreads();
#endif
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

    Connection::Connection( sys::Socket* socket )
    : m_socket( socket ), m_handle( NULL ), m_close( false ), m_id( ( intptr_t ) this )
    {
        TRACE_ENTERLEAVE();
        General::setSocketNonBlocking( *socket );
     }


    void Connection::assign( const Base& base )
    {
        m_handle = bufferevent_socket_new( base, m_socket->s(), BEV_OPT_THREADSAFE | BEV_OPT_CLOSE_ON_FREE );

        if ( !m_handle )
        {
            TRACE_ERROR( "failed to create buffered event", "" );
            throw GeneralError;
        }

        bufferevent_setcb( m_handle, onReadStatic, onWriteStatic, onErrorStatic, this );
        bufferevent_enable( m_handle, EV_READ | EV_WRITE );
        m_input = bufferevent_get_input( m_handle );
        //m_output = evbuffer_new();
        m_output = bufferevent_get_output( m_handle );
     }

    Connection::~Connection()
    {
        TRACE_ENTERLEAVE();
        
        if ( m_handle )
        {
            bufferevent_disable( m_handle, EV_READ | EV_WRITE );
            bufferevent_free( m_handle );
        }

        if ( m_socket )
        {
            delete m_socket;
        }
    }

    unsigned int Connection::read( char* data, unsigned int length )
    {
        return bufferevent_read( m_handle, data, length );
    }
    
    void Connection::write( const char* data, unsigned int length )
    {
#ifdef _DEBUG
        std::string s;
        s.append( data, length );
        TRACE( "%s", s.c_str() );
#endif
        bufferevent_write( m_handle, data, length );
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
        m_socket->shutdown();
        
        onClose();
    }

    void Connection::onRead()
    {
        
    }

    void Connection::onWrite()
    {
        TRACE_ENTERLEAVE();
        
        if ( m_close )
        {
            close();
        }
    }

    void Connection::onError( short error )
    {
        TRACE_ENTERLEAVE();

        TRACE("%d, eof: %d, error %d", error, error & BEV_EVENT_EOF, error & BEV_EVENT_ERROR );

        if ( error & BEV_EVENT_EOF || error & BEV_EVENT_ERROR)
        {
            close();
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

        TRACE("setting timeouts", "");
        
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




}
