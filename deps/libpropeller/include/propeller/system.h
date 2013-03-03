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

#include "common.h"

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
#define MAX_SEMAPHORE_COUNT 10000
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

    /**
     * Platform independent utility functions
     */
    class General
    {
    public:
        /**
         * @return Last error code
         */
        static unsigned int getLastError( );
        /**
         * @return Current time in seconds since epoch
         */
        static unsigned int getTime( );
        /**
         * 
         * @return MIllisecond timestamp
         */
        static unsigned int getMillisecondTimestamp( );

        /**
         * Atomic exchange of op pointers
         * @param target pointer to target pointer
         * @param value pointer to exchange with
         * @return old value of target
         */
        static void* interlockedExchangePointer( void** target, void* value );
        /**
         * Atomic increment
         * @param target pointer to value that has be incremented
         */
        static unsigned int interlockedIncrement( unsigned int* target );
        /**
         * Atomic decrement
         * @param target pointer to value that has be decremented
         */
        static unsigned int interlockedDecrement( unsigned int* target );
        /**
         * Sleep current thread
         * @param seconds interval in seconds
         */
        static void sleep( unsigned int seconds );

    };

    /**
     * Platform independent critical section class
     */
    class Lock
    {
        friend class LockEnterLeave;

    public:
        Lock( );
        ~Lock( );

        void lock( );
        void unlock( );

    private:
        LOCK_HANDLE* handle( );

    private:
        LOCK_HANDLE m_lock;
    };

    /**
     *  Lock utility class
     */
    class LockEnterLeave
    {
    public:
        LockEnterLeave( Lock& lock );
        ~LockEnterLeave( );

    private:
        Lock& m_lock;
    };

    /**
     * PLatform independent event 
     */
    class Event
    {
    public:
        Event( );
        ~Event( );

        /**
         * wait on event 
         */
        void wait( );

        /**
         * set event
         */
        void set( );

        
        void disable( )
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

    /**
     * PLatform independet semaphore class
     */
    class Semaphore
    {
    public:
        Semaphore( );
        ~Semaphore( );

        /**
         * increase semaphore count
         */
        void post( );
        /**
         * wait till semaphore value becomes greater than zero
         */
        void wait( );

        
        void setValue( unsigned int value );

    private:
        SEMAPHORE_HANDLE m_handle;

    };


    /**
     * Platform independent thread class
     */
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
        /**
         * block calling thread till thread finishes
         */
        void join( ) const;
#ifdef WIN32
        static DWORD WINAPI routineStatic( void* data );
#else
        static void routineStatic( void* data );
#endif
        /**
         * routine (to be implemented by deriving classes)
         */
        virtual void routine( ) = 0;
        
        /**
         * starts thread
         */
        void start( );
        /**
         * stops thread
         */
        virtual void stop( );

        /**
         * 
         * @return true if thread has been started, fale otherwise
         */
        bool started( ) const
        {
            return m_started;
        }

        /**
         * 
         * @return current threads id
         */
        static intptr_t currentId( );

    protected:
        size_t m_stackSize;

    private:
        void cleanup( );

    private:
        THREAD_HANDLE m_handle;
        bool m_started;

    };

    
    /**
     * platform independent socket wrapper class
     */
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

    /**
     * Thread pool class
     */
    class ThreadPool
    {
        friend class Thread;

    public:
        ThreadPool( );
        virtual ~ThreadPool( );
        struct Task
        {
            Task( )
            {
                
            }
            
            virtual ~Task( )
            {
            }
        };
        
        void queue( Task* task );

        
        void start( unsigned int threads );
        void stop( );

        /**
         * Thread pool worker thread 
         */
        class Worker : public sys::Thread
        {
        public:
            Worker( ThreadPool& pool );
            virtual ~Worker( );
            virtual void routine( );

            void* data( ) const
            {
                return m_data;
            }

            void setData( void* data )
            {
                m_data = data;
            }

            sys::Lock& lock( ) 
            {
                return m_lock;
            }

        private:
            ThreadPool& m_pool;
            void* m_data;
            sys::Lock m_lock;
        };
        
        virtual void onTaskProcess( Task* task, Worker& thread ) = 0;
        virtual void onThreadStart( Worker& thread )
        {
            
        }
        typedef std::list< Worker* > WorkerList;
        
        WorkerList& threads()
        {
            return m_threads;
        }


    private:

        Task* get( );

        bool needStop( ) const
        {
            return m_stop;
            
            
        }


    private:
        std::list< Task* > m_queue;
        WorkerList m_threads;
        Lock m_lock;
        Semaphore m_semaphore;
        bool m_stop;
    };
}



#endif //_SYSTEM_H_




