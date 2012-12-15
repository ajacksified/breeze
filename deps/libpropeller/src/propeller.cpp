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

#include "propeller.h"
#include "Server.h"
#include "Connection.h"

PROPELLER_APIEXP void* PROPELLER_API propeller_serverCreate( unsigned int port )
{
    return (void*) new propeller::Server( port );
    
}

//PROPELLER_APIEXP void PROPELLER_API propeller_serverDestroy( void* server )
//{
//    delete ( ( Server* ) server );
//}

PROPELLER_APIEXP short PROPELLER_API propeller_serverStart( void* server )
{
    try
    {
        ( ( propeller::Server* ) server )->start( );
        
    }
    catch (...)
    {
        return PROPELLER_STATUS_FAILURE;
    }

    return PROPELLER_STATUS_SUCCESS;
}

//PROPELLER_APIEXP short PROPELLER_API propeller_serverStop( void* server )
//{
//     ( ( Server*  )server )->stop( );
//
//    return PROPELLER_STATUS_SUCCESS;
//}

PROPELLER_APIEXP void PROPELLER_API propeller_serverDestroy( void* server )
{
    delete ( ( propeller::Server*  )server );
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetConnectionThreadCount( void* server, unsigned int threads )
{
    ( ( propeller::Server* )server )->setConnectionThreadCount( threads );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetConnectionThreadCount( void* server )
{
    return ( ( propeller::Server* ) server )->connectionThreadCount();
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetPoolThreadCount( void* server, unsigned int threads )
{
    ( ( propeller::Server* )server )->setPoolThreadCount( threads );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetPoolThreadCount( void* server )
{
    return ( ( propeller::Server* ) server )->poolThreadCount();
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetConnectionReadTimeout( void* server, unsigned int timeout )
{
    ( ( propeller::Server* ) server )->setConnectionReadTimeout( timeout );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetConnectionWriteTimeout( void* server, unsigned int timeout )
{
    ( ( propeller::Server* ) server )->setConnectionWriteTimeout( timeout );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetConnectionReadTimeout( void* server )
{
    return ( ( propeller::Server* ) server )->connectionReadTimeout( );
}

PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetConnectionWriteTimeout( void* server )
{
    return ( ( propeller::Server* ) server )->connectionWriteTimeout( );
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetOnRequestCallback( void* server, OnRequest callback, void* data )
{
    ( ( propeller::Server* ) server )->setOnRequestCallback( ( propeller::Server::OnRequest ) callback, data );

    return PROPELLER_STATUS_SUCCESS;
}

// PROPELLER_APIEXP short PROPELLER_API propeller_serverSetAfterRequestCallback( void* server, AfterRequest callback, void* data )
// {
//     ( ( Server* ) server )->setAfterRequestCallback( ( Server::AfterRequest ) callback, data );
// 
//     return PROPELLER_STATUS_SUCCESS;
// }

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetOnThreadStartedCallback( void* server, OnThreadStarted callback, void* data )
{
    ( ( propeller::Server* ) server )->setOnThreadStartedCallback( ( propeller::Server::OnThreadStarted ) callback, data );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetOnThreadStoppedCallback( void* server, OnThreadStarted callback, void* data )
{
    ( ( propeller::Server* ) server )->setOnThreadStoppedCallback( ( propeller::Server::ThreadCallback ) callback, data );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetTimer( void* server, unsigned int interval, TimerCallback callback, void* data )
{
    ( ( propeller::Server* ) server )->setTimer( interval, ( propeller::Server::TimerCallback ) callback, data );
    
    return PROPELLER_STATUS_SUCCESS;
}


PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetBody( const void* request )
{
    return ( ( propeller::Request* )request )->body( );
}

PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetUri( const void* request )
{
    return ( ( propeller::Request* )request )->uri();
}

PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetHeader( const void* request, const char* name )
{
    return ( ( propeller::Request* )request )->header( name );
}

PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetMethod( const void* request )
{
    return ( ( propeller::Request* )request )->method( );
}


PROPELLER_APIEXP unsigned int PROPELLER_API propeller_responseGetStatus( const void* response )
{
    return ( ( propeller::Response* )response )->status( );
}

PROPELLER_APIEXP void PROPELLER_API propeller_responseSetStatus( void* response, unsigned int status )
{
    ( ( propeller::Response* )response )->setStatus( status );

}

PROPELLER_APIEXP void PROPELLER_API propeller_responseSetHeader( void* response, const char* name, const char* value )
{
    ( ( propeller::Response* )response)->addHeader( name, value );
}

PROPELLER_APIEXP void PROPELLER_API propeller_responseSetBody( void* response, const char* body, unsigned int length )
{
    ( ( propeller::Response* )response )->setBody( body, length );
}

PROPELLER_APIEXP void PROPELLER_API propeller_utilLockEnter( void* lock )
{
    ( ( sys::Lock* ) lock )->lock();
}

PROPELLER_APIEXP void PROPELLER_API propeller_utilLockLeave( void* lock )
{
    ( ( sys::Lock* ) lock )->unlock();
}

