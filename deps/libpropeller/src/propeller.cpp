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
    return (void*) new Server( port );
}

//PROPELLER_APIEXP void PROPELLER_API propeller_serverDestroy( void* server )
//{
//    delete ( ( Server* ) server );
//}

PROPELLER_APIEXP short PROPELLER_API propeller_serverStart( void* server )
{
    try
    {
        ( ( Server* ) server )->start( );
        
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


PROPELLER_APIEXP short PROPELLER_API propeller_serverSetConnectionThreadCount( void* server, unsigned int threads )
{
    ( ( Server* )server )->setConnectionThreadCount( threads );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetConnectionThreadCount( void* server )
{
    return ( ( Server* ) server )->connectionThreadCount();
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetPoolThreadCount( void* server, unsigned int threads )
{
    ( ( Server* )server )->setPoolThreadCount( threads );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetPoolThreadCount( void* server )
{
    return ( ( Server* ) server )->poolThreadCount();
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetConnectionReadTimeout( void* server, unsigned int timeout )
{
    ( ( Server* ) server )->setConnectionReadTimeout( timeout );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetConnectionWriteTimeout( void* server, unsigned int timeout )
{
    ( ( Server* ) server )->setConnectionWriteTimeout( timeout );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetConnectionReadTimeout( void* server )
{
    return ( ( Server* ) server )->connectionReadTimeout( );
}

PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetConnectionWriteTimeout( void* server )
{
    return ( ( Server* ) server )->connectionWriteTimeout( );
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetOnRequestCallback( void* server, OnRequest callback, void* data )
{
    ( ( Server* ) server )->setOnRequestCallback( ( Server::OnRequest ) callback, data );

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
    ( ( Server* ) server )->setOnThreadStartedCallback( ( Server::OnThreadStarted ) callback, data );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP short PROPELLER_API propeller_serverSetOnThreadStoppedCallback( void* server, OnThreadStarted callback, void* data )
{
    ( ( Server* ) server )->setOnThreadStoppedCallback( ( Server::OnThreadStopped ) callback, data );

    return PROPELLER_STATUS_SUCCESS;
}

PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetBody( const void* request )
{
    return ( ( Request* )request )->body( );
}

PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetUri( const void* request )
{
    return ( ( Request* )request )->uri();
}

PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetHeader( const void* request, const char* name )
{
    return ( ( Request* )request )->header( name );
}

PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetMethod( const void* request )
{
    return ( ( Request* )request )->method( );
}


PROPELLER_APIEXP unsigned int PROPELLER_API propeller_responseGetStatus( const void* response )
{
    return ( ( Response* )response )->status( );
}

PROPELLER_APIEXP void PROPELLER_API propeller_responseSetStatus( void* response, unsigned int status )
{
    ( ( Response* )response )->setStatus( status );

}

PROPELLER_APIEXP void PROPELLER_API propeller_responseSetHeader( void* response, const char* name, const char* value )
{
    ( ( Response* )response)->addHeader( name, value );
}

PROPELLER_APIEXP void PROPELLER_API propeller_responseSetBody( void* response, const char* body )
{
    ( ( Response* )response )->setBody( body );

}

