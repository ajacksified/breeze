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

#include "system.h"
#include "trace.h"



namespace sys
{

    //
    //	General
    //

    unsigned int General::getLastError ( )
    {
#ifdef WIN32
        return( unsigned int ) GetLastError( );
#else
        return errno;
#endif
    }

    unsigned int General::getTime ( )
    {
        time_t currentTime = 0;
        time( &currentTime );
        return( unsigned int ) currentTime;
    }
    
    unsigned int General::getMillisecondTimestamp()
    {
        timeval time;
        gettimeofday( &time, NULL );
        
        long timestamp = ( ( time.tv_sec ) * 1000 + time.tv_usec / 1000.0 ) + 0.5;
        
        return ( unsigned int ) timestamp;
    }

    void* General::interlockedExchangePointer ( void** target, void* value )
    {
#ifdef WIN32
        return InterlockedExchangePointer( target, value );
#else
        void* oldValue = __sync_fetch_and_add( target, 0 );
        return __sync_val_compare_and_swap( target, oldValue, value );
#endif

    }

    void General::interlockedIncrement( unsigned int* target )
    {
#ifdef WIN32

#else
    __sync_fetch_and_add( target, 1 );
#endif
    }

    unsigned int General::interlockedDecrement( unsigned int* target )
    {
#ifdef WIN32

#else
    return __sync_fetch_and_sub( target, 1 );
    
#endif
    }



    void General::sleep ( unsigned int seconds )
    {
#ifdef WIN32
        Sleep( seconds * 1000 );
#else
        ::sleep( seconds );
#endif
    }

    //
    //	Lock
    //

    Lock::Lock ( )
    {
#ifdef WIN32
        InitializeCriticalSection( &m_lock );
#else
        pthread_mutexattr_t mutexAttributes;
        pthread_mutexattr_init( &mutexAttributes );
        pthread_mutexattr_settype( &mutexAttributes, PTHREAD_MUTEX_ERRORCHECK );
        pthread_mutex_init( &m_lock, &mutexAttributes );
#endif
    }

    Lock::~Lock ( )
    {
#ifdef WIN32
        DeleteCriticalSection( &m_lock );
#else
        pthread_mutex_destroy( &m_lock );
#endif
    }

    LOCK_HANDLE* Lock::handle ( )
    {
        return &m_lock;
    }
    
    void Lock::lock()
    {
 #ifdef WIN32
        EnterCriticalSection( m_lock );
#else
        pthread_mutex_lock( &m_lock );
#endif
    }
    
    
    void Lock::unlock()
    {
 #ifdef WIN32
        EnterCriticalSection( m_lock );
#else
        pthread_mutex_unlock( &m_lock );
#endif
    }
    
    

    //
    //	LockEnterLeave
    //
    LockEnterLeave::LockEnterLeave ( Lock& lock )
    : m_lock ( lock )
    {
        m_lock.lock();
    }

    LockEnterLeave::~LockEnterLeave ( )
    {
        m_lock.unlock();    
    }

    //
    //	Event
    //
    Event::Event ( )
    : m_disabled ( false )
    {
#ifdef WIN32
        m_handle = CreateEvent( 0, FALSE, 0, NULL );
#else
        pthread_mutex_init( &m_lock, NULL );

        pthread_cond_init( &m_condition, NULL );
#endif
    }

    Event::~Event ( )
    {
#ifdef WIN32
        CloseHandle( m_handle );
#else
        pthread_cond_destroy( &m_condition );

        pthread_mutex_destroy( &m_lock );
#endif
    }

    void Event::wait ( )
    {
        if ( m_disabled )
        {
            return;
        }

#ifdef WIN32
        WaitForSingleObject( m_handle, INFINITE );
#else		
        pthread_mutex_lock( &m_lock );

        pthread_cond_wait( &m_condition, &m_lock );

        pthread_mutex_unlock( &m_lock );
#endif
    }

    void Event::set ( )
    {
#ifdef WIN32
        SetEvent( m_handle );
#else
        pthread_cond_signal( &m_condition );
#endif
    }
    
    void Event::broadcast ( )
    {
#ifdef WIN32
        //
        //  whats the windows equivalent?
        //
#else
        pthread_cond_broadcast( &m_condition );
#endif
    }


    Semaphore::Semaphore()
    {
#ifdef __MACH__
        char name[16];
        sprintf( name, "%d", rand() % 1000 );
        m_handle = sem_open( name,  O_CREAT, S_IRUSR | S_IWUSR, 0 );
#else
        sem_init( &m_handle, 0, 0 );
#endif
       

       
    }

    Semaphore::~Semaphore()
    {
#ifdef __MACH__
        sem_destroy( m_handle );
#else
        sem_destroy( &m_handle );
#endif

        
    }

    void Semaphore::post()
    {
#ifdef __MACH__
        sem_post( m_handle );
#else
        sem_post( &m_handle );
#endif
        
    }

    void Semaphore::wait()
    {
#ifdef __MACH__
        sem_wait( m_handle );
#else
        sem_wait( &m_handle );
#endif
        
    }

    void Semaphore::setValue( unsigned int value )
    {
        for ( int i = 0; i < value; i++ )
        {
            post();
        }
    }

    //
    //	Thread
    //

    Thread::Thread ( )
    : m_started ( false ), m_stackSize( 0 )
    {
        
    }

    Thread::~Thread ( )
    {
        stop();
    }

    void Thread::start ( )
    {
        m_started = true;
        
#ifdef WIN32
        m_handle = CreateThread( 0, 0, routineStatic, this, 0, NULL );
#else
        pthread_attr_t attributes;
        
        pthread_attr_init( &attributes );
        
        if ( m_stackSize )
        {
            
            int result = pthread_attr_setstacksize( &attributes, m_stackSize );
        }
        
        pthread_create( &m_handle, &attributes, ( void *( * )( void* ) ) routineStatic, this );

#endif

    }

    intptr_t Thread::currentId()
    {
        return ( intptr_t ) pthread_self();
        
    }
    void Thread::stop ( )
    {

        
        
        if ( !m_started )
        {
            return;
        }
        
//#ifdef WIN32
//        TerminateThread( m_handle, 1 );
//#else
//        pthread_cancel( m_handle );
//#endif

        join();
        cleanup();

        m_started = false;
    }


#ifdef WIN32
    DWORD WINAPI Thread::routineStatic ( void* data )
#else

    void Thread::routineStatic ( void* data )
#endif
    {
        Thread* instance = reinterpret_cast < Thread* > ( data );

        if ( instance )
        {
            instance->routine( );
            instance->stop();
        }

#ifdef WIN32
        return 0;
#endif
    }

    void Thread::cleanup( )
    {

#ifdef WIN32
        CloseHandle( m_handle );
#else
        pthread_detach( m_handle );
#endif
    }

    void Thread::join ( ) const
    {

#ifdef WIN32

        WaitForSingleObject( m_handle, INFINITE );
#else
        pthread_join( m_handle, NULL );
#endif
    }

    void Socket::startup ( )
    {
#ifdef	WIN32
        WORD versionRequested;
        WSADATA wsaData;
        versionRequested = MAKEWORD( 2, 2 );
        WSAStartup( versionRequested, &wsaData );
#endif
    }

    void Socket::cleanup ( )
    {
#ifdef	WIN32
        WSACleanup( );
#endif
    }

    Socket::Socket ( )
    {
#ifdef WIN32
        m_socket = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
#else
        m_socket = ::socket( PF_INET, SOCK_STREAM, 0 );
#endif	
    }

    Socket::Socket ( const Socket& socket )
    {
        m_socket = socket.m_socket;
    }

    Socket::Socket ( const SOCKET& socket )
    : m_socket ( socket )
    {
    }

    Socket::~Socket ( )
    {
#ifdef  WIN32
        ::closesocket( m_socket );

        ::shutdown( m_socket, SD_BOTH );

#else
        ::close( m_socket );
#endif
    }

    Socket::Status Socket::receive ( char* buffer, unsigned int bufferSize, unsigned int& bytesReceived )
    {
        Status status = StatusFailed;

        int received = ::recv( m_socket, buffer, bufferSize, 0 );

        if ( received && received != SOCKET_ERROR )
        {
            bytesReceived = received;
            status = StatusSuccess;
        }

        return status;
    }

    Socket::Status Socket::shutdown ( )
    {
#ifdef WIN32
        ::shutdown( m_socket, SD_BOTH );
#else
        ::shutdown( m_socket, SHUT_WR );
#endif
        return StatusSuccess;

    }

    Socket::Status Socket::send ( const char* buffer, unsigned int length, unsigned int& bytesSent )
    {
        Status status = StatusFailed;

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif
        int sent = ::send( m_socket, buffer, length, MSG_DONTWAIT );

        if ( sent != SOCKET_ERROR )
        {
            bytesSent = sent;
            status = StatusSuccess;
        }

        return status;
    }

    Socket::Status Socket::listen ( )
    {
        int result = ::listen( m_socket, SOMAXCONN );

        return( result == 0 ) ? StatusSuccess : StatusFailed;
    }

    Socket* Socket::accept ( )
    {
        SOCKET connection = ::accept( m_socket, NULL, NULL );

        if ( connection != INVALID_SOCKET )
        {
            return new Socket( connection );
        }



        return NULL;
    }

    Socket::Status Socket::bind ( unsigned int port )
    {
        struct sockaddr_in service;
        memset( &service, 0, sizeof(service) );
        
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = htonl( INADDR_ANY );
        service.sin_port = htons( port );

        //
        //	set reuse address flag, workaround for address in use issue on linux
        //
        int on = 1;
        int result = ::setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, ( const char* ) & on, sizeof(on ) );

        result = ::bind( m_socket, ( struct sockaddr* ) & service, sizeof( service ) );

        return( result == 0 ) ? StatusSuccess : StatusFailed;
    }

    unsigned int Socket::getLastError ( )
    {
#ifdef WIN32
        return( unsigned int ) WSAGetLastError( );
#else
        return errno;
#endif
    }

    SOCKET Socket::s ( )
    {
        return m_socket;
    }

    SocketSet::SocketSet ( )
    : m_max ( 0 )
    {
        FD_ZERO( &m_set );

    }

    SocketSet::SocketSet ( Socket& socket )
    : m_max ( 0 )
    {
        FD_ZERO( &m_set );
        add( socket );

    }

    void SocketSet::add ( Socket& socket )
    {
        if ( socket.s( ) > m_max )
        {
            m_max = socket.s( );
        }

        FD_SET( socket.s( ), &m_set );
    }

    unsigned int SocketSet::select ( SocketSet::ReadyType type, unsigned int _timeout )
    {
        int count = 0;

        struct timeval timeout;

        memset( &timeout, 0, sizeof( timeout ) );

        timeout.tv_sec = _timeout;

        if ( type == ReadyToRead )
        {
            count = ::select( m_max + 1, &m_set, NULL, NULL, &timeout );
        }
        else
        {
            count = ::select( m_max + 1, NULL, &m_set, NULL, &timeout );
        }

        return count == SOCKET_ERROR ? 0 : count;
    }

    bool SocketSet::isSelected ( Socket& socket )
    {
        return FD_ISSET( socket.s( ), &m_set ) ? true : false;
    }
}


