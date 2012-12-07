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

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

//
//	Needed includes
//
#ifdef WIN32
#define FD_SETSIZE 1024
#include <Winsock2.h>
#include <windows.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <memory.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#endif



//
//	Generic system independent wrappers
//

namespace sys
{

    //
    //	Type defines
    //
#ifdef WIN32
#define LOCK_HANDLE CRITICAL_SECTION
#define THREAD_HANDLE HANDLE
#define SEMAPHORE_HANDLE HANDLE
#define PIPE_HANDLE HANDLE
#define API_CALL __stdcall
#define fd_set FD_SET
#else
#define LOCK_HANDLE pthread_mutex_t
#define THREAD_HANDLE pthread_t
#ifdef __MACH__
    #define SEMAPHORE_HANDLE sem_t*
#else
    #define SEMAPHORE_HANDLE sem_t
#endif

#define PIPE_HANDLE int
#define API_CALL
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SOCKET int
#endif

    class General
    {
    public:
        static unsigned int getLastError( );
        static unsigned int getTime( );

        static void* interlockedExchangePointer( void** target, void* value );
        static void interlockedIncrement( unsigned int* target );
        static unsigned int interlockedDecrement( unsigned int* target );
        static void sleep( unsigned int seconds );

    };


    //
    //	Lock (critical section)
    //

    class Lock
    {
        friend class LockEnterLeave;

    public:
        Lock( );
        ~Lock( );

    private:
        LOCK_HANDLE* handle( );

    private:
        LOCK_HANDLE m_lock;
    };

    //
    //	Lock utility class
    //

    class LockEnterLeave
    {
    public:
        LockEnterLeave( Lock& lock );
        ~LockEnterLeave( );

    private:
        Lock& m_lock;
    };

    //
    //	Generic event class
    //

    class Event
    {
    public:
        Event( );
        ~Event( );

        //
        //	wait on event
        //
        void wait( );

        //
        //	set event
        //
        void set( );

        void broadcast();
        void disable()
        {
            m_disabled = true;
        }

    private:
#ifdef WIN32
        HANDLE m_handle;
#else
        pthread_mutex_t m_lock;
        pthread_cond_t m_condition;
#endif
        bool m_disabled;
    };

    class Semaphore
    {
    public:
        Semaphore();
        ~Semaphore();

        void post();
        void wait();
        void setValue( unsigned int value );

    

    private:
        SEMAPHORE_HANDLE m_handle;

    };


    //
    //	Generic thread class
    //

    class Thread
    {
    public:
        Thread( );
        Thread( const Thread& thread )
        {
            m_handle = thread.m_handle;
            m_started = thread.m_started;
        }

        virtual ~Thread( );
        void join( ) const;
#ifdef WIN32
        static DWORD WINAPI routineStatic( void* data );
#else
        static void routineStatic( void* data );
#endif
        virtual void routine( ) = 0;
        void start( );
        virtual void stop( );

        bool started( ) const
        {
            return m_started;
        }
        
        static intptr_t currentId();

    private:
        void cleanup();

    private:
        THREAD_HANDLE m_handle;
        bool m_started;
    };

    //
    //	TCP socket
    //

    class Socket
    {
        friend class SocketSet;

    public:

        enum Status
        {
            StatusSuccess,
            StatusFailed
        };

        static void startup( );
        static void cleanup( );

        Socket( );
        Socket( const Socket& socket );
        Socket( const SOCKET& socket );

        ~Socket( );

        Status receive( char* buffer, unsigned int bufferSize, unsigned int& bytesReceived );
        Status send( const char* buffer, unsigned int length, unsigned int& bytesSent );
        Status listen( );
        Socket* accept( );
        Status bind( unsigned int port );

        Status shutdown( );

        static unsigned int getLastError( );

        SOCKET s( );

    private:
        

    private:
        SOCKET m_socket;
    };

    //
    //	wrapper around FD_SET
    //

    class SocketSet
    {
    public:

        enum ReadyType
        {
            ReadyToRead,
            ReadyToWrite
        };

        SocketSet( );

        SocketSet( Socket& socket );

        //
        //	add socket to set
        //
        void add( Socket& socket );
        //
        //	select sockets in given state with timeout in microseconds. returns number of sockets selected
        //
        unsigned int select( ReadyType type, unsigned int timeout = 0 );
        //
        //	check whether socket has been selected
        //
        bool isSelected( Socket& socket );

    private:
        fd_set m_set;
        SOCKET m_max;
    };
}



#endif //_SYSTEM_H_




