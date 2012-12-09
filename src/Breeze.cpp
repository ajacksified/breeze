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

#include "Breeze.h"
#include <propeller.h>
#include <lua_cjson.h>


Breeze* Breeze::m_instance = NULL;

//
//  lua api functions
//
int setOnRequest( lua_State* lua )
{
    TRACE_ENTERLEAVE();
    
    lua_setglobal( lua, "__onRequest" );
}

int requestGetBody( lua_State* lua )
{
    void* request = lua_touserdata( lua, -1 );
    lua_pop( lua, 1 );
    
    lua_pushstring( lua, propeller_requestGetBody( request ) );
    
    return 1;
}

int requestGetUrl( lua_State* lua )
{
    void* request = lua_touserdata( lua, -1 );
    lua_pop( lua, 1);

    lua_pushstring( lua, propeller_requestGetUri( request ) );

    return 1;
}

int requestGetMethod( lua_State* lua )
{
    void* request = lua_touserdata( lua, -1 );
    lua_pop( lua, 1 );

    lua_pushstring( lua, propeller_requestGetMethod( request ) );

    return 1;
}

int requestGetHeader( lua_State* lua )
{
    void* request = lua_touserdata( lua, -2 );
    lua_remove( lua, -2 );

    const char* header = propeller_requestGetHeader( request, lua_tostring( lua, -1 ) );
    lua_remove( lua, -1 );

    lua_pushstring( lua, header );
    return 1;
}

int responseSetBody( lua_State* lua )
{
    void* response = lua_touserdata( lua, -2 );
    lua_remove( lua, -2 );
    
    size_t length = 0;
    const char* body = luaL_tolstring( lua, -1, &length );

    propeller_responseSetBody( response, body, length );
    lua_remove( lua, -1 );

    return 0;
}

int responseSetHeader( lua_State* lua )
{
    void* response = lua_touserdata( lua, -3 );
    lua_remove( lua, -3 );

    const char* name = lua_tostring( lua, -2 );
    const char* value = lua_tostring( lua, -1 );

    propeller_responseSetHeader( response, name, value );
    lua_remove( lua, -1 );
    lua_remove( lua, -2 );

    return 0;
}

int responseSetStatus( lua_State* lua )
{
    void* response = lua_touserdata( lua, -2 );
    lua_remove( lua, -2 );

    unsigned int status = lua_tonumber( lua, -1 );
    lua_remove( lua, -1 );

    propeller_responseSetStatus( response, status );
    
    return 0;
}

Breeze::Breeze( unsigned int port )
: m_development( false ), m_dataCollectTimeout( 20 )
{
    m_server = propeller_serverCreate( port );

    propeller_serverSetOnRequestCallback( m_server, onRequestStatic, this );
    propeller_serverSetOnThreadStartedCallback( m_server, onThreadStartedStatic, this );
}

Breeze::~Breeze( )
{
    m_instance = NULL;
}

Breeze* Breeze::instance()
{
    return m_instance;
}

Breeze* Breeze::create( unsigned int port )
{
    if ( !m_instance )
    {
        m_instance = new Breeze( port );
    }

    return m_instance;
}

void Breeze::onRequestStatic( const void* request, void* response, void* data, void* threadData )
{
   ( ( Breeze* ) data )->onRequest( request, response, threadData );
}

void Breeze::onRequest( const void* request, void* response, void* threadData )
{
    ThreadState* state = ( ThreadState* ) threadData;
    lua_State* lua = state->lua;
    
    propeller_utilLockEnter( state->lock );

    //
    //  reload script in dewvelopment mode
    //
    if ( m_development )
    {

        //
        //  reload libraries
        //
        luaL_dostring( lua, "for key, value in pairs(package.loaded) do package.loaded[key] = nil end" );
        loadLibraries( lua );
        if ( !loadScript( lua ) )
        {
            //
            //  error executing lua
            //
            propeller_responseSetStatus( response, 500 );
            propeller_responseSetBody( response, lua_tostring( lua, -1 ), 0 );
            lua_pop( lua, 1 );
            return;
        }
    }

    //
    //  invoke lua callback function
    //
    lua_getglobal( lua, "debug");
    lua_getfield( lua, -1, "traceback");
    
    lua_getglobal( lua, "__onRequest" );
    lua_pushlightuserdata( lua, ( void* ) request );
    lua_pushlightuserdata( lua, response );
    
    //
    //  measure how long did the request take
    //
    timeval start;
    gettimeofday( &start, NULL );

    
    
    int result = lua_pcall( lua, 2, 0, -4 );

    if (result > 0)
    {
        //
        //  error executing lua
        //
        TRACE_ERROR("%s", lua_tostring( lua, -1 ));
        propeller_responseSetStatus( response, 500 );
        if ( m_development )
        {
            propeller_responseSetBody( response, lua_tostring( lua, -1 ), 0 );
        }
         
        lua_pop( lua, 1 );
    }
    
    timeval end;
    gettimeofday( &end, NULL );
    
    long mtime, seconds, useconds;
    seconds  = end.tv_sec  - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;

    mtime = ( ( seconds) * 1000 + useconds / 1000.0 ) + 0.5;
    
    
    
    //
    //  collect high level metrics
    //
    state->metrics.collect( 
        mtime, 
        propeller_responseGetStatus( response ) > 400,
        propeller_requestGetUri( request ) 
    );
    
    //
    //  remove debug.traceback from stack
    //
    lua_remove( lua, -1 );
    lua_remove( lua, -2 );
    
    propeller_utilLockLeave( state->lock );
 }

void Breeze::onThreadStartedStatic( void** threadData, void* data, void* lock )
{
   ( ( Breeze* ) data )->onThreadStarted( threadData, lock );
}

 void Breeze::onThreadStarted( void** threadData, void* lock )
 {
     TRACE_ENTERLEAVE();

     //
     // create new lua environment
     //
     lua_State* lua = luaL_newstate();


     loadLibraries( lua );

     //
     // register environment
     //
     lua_getglobal( lua, "breezeApi" );
     lua_pushstring( lua, m_environment.c_str() );
     lua_setfield( lua, -2, "environment" );
     lua_pop( lua, 1 );

     ThreadState* state = new ThreadState( lua, lock );
     *threadData = state;
     
     m_threadStates.push_back( state );
     
     if ( !loadScript( lua ) )
     {
         throw std::exception();
     }
 }

 void Breeze::setEnvironment( const char* environment )
 {
     if ( environment )
     {
        m_environment = environment;
     }

     if ( !m_environment.size() )
     {
         m_environment = "development";
     }

     if ( m_environment == "development")
     {
         m_development = true;
     }

 }
 
void Breeze::loadLibraries( lua_State* lua )
{
      luaL_openlibs( lua );

     //
     // register lua api functions
     //
     static const luaL_Reg functions[] = {
        {"setOnRequest", setOnRequest},
        {"requestGetBody", requestGetBody},
        {"requestGetUrl", requestGetUrl},
        {"requestGetHeader", requestGetHeader},
        {"requestGetMethod", requestGetMethod},
        {"responseSetStatus", responseSetStatus},
        {"responseSetBody", responseSetBody},
        {"responseSetHeader", responseSetHeader},

        {NULL, NULL}
    };

    luaL_register( lua, "breezeApi", functions );

    //
    //  cjson library
    //
    luaopen_cjson( lua );

    //
    // modify package search path
    //
    lua_getglobal( lua, "package" );
    lua_getfield( lua, -1, "path" );
    std::string path = lua_tostring( lua, -1 );
    
    for ( std::list< std::string >::iterator i = m_paths.begin(); i != m_paths.end(); i++ )
    {
        path.append( ";" );
        path.append( i->c_str() );
        path.append( "/?.lua" );
    }
    
    TRACE( "%s", path.c_str() );
    
    lua_pop( lua, 1 );
    lua_pushstring( lua, path.c_str( ) );
    lua_setfield( lua, -2, "path" );
    lua_pop( lua, 1 );
 }


 bool Breeze::loadScript( lua_State* lua )
 {
    TRACE_ENTERLEAVE();
    
    int res = luaL_loadfile( lua, m_script.c_str() );

    if ( res == 0 )
    {
        res = lua_pcall( lua, 0, LUA_MULTRET, 0 );

        if ( res )
        {
            TRACE_ERROR( "%s", lua_tostring( lua, -1 ) );
            return false;
        }
    }
    else
    {
        TRACE_ERROR("%s", lua_tostring( lua, -1) );
        return false;
    }

    return true;
 }

 void Breeze::run()
 {
     //
     // hardcode 30 seconds read and write timeouts
     //
     propeller_serverSetConnectionReadTimeout( m_server, 30 );
     propeller_serverSetConnectionWriteTimeout( m_server, 30 );
     
     if ( m_development )
     {
         //
         // limit number of threads in development mode
         //
         propeller_serverSetPoolThreadCount( m_server, 1 );
         propeller_serverSetConnectionThreadCount( m_server, 1 );
     }
     
      
    
     propeller_serverSetTimer( m_server, m_dataCollectTimeout, onTimerStatic, ( void* ) this );
          
     propeller_serverStart( m_server );
     
 }
 
 void Breeze::onTimerStatic( void* data )
 {
    ( ( Breeze* ) data )->onTimer();
 }
 
 void Breeze::onTimer()
 {
     TRACE_ENTERLEAVE();
     
     //
     // collect statistics from all threads
     //
     for ( std::list< ThreadState* >::iterator i = m_threadStates.begin(); i != m_threadStates.end(); i++ )
     {
         ThreadState* state = *i;
         propeller_utilLockEnter( state->lock );
         m_metrics.add( state->metrics, i == m_threadStates.begin() ? m_dataCollectTimeout : 0 );
         state->metrics.reset();
         
         propeller_utilLockLeave( state->lock );
     }
     
     //
     // export stats to lua
     //
     for ( std::list< ThreadState* >::iterator i = m_threadStates.begin(); i != m_threadStates.end(); i++ )
     {
         ThreadState* state = *i;
         propeller_utilLockEnter( state->lock );
         
         lua_getglobal( state->lua, "breezeApi" );
         
         lua_newtable( state->lua );
         
         lua_pushnumber( state->lua, m_metrics.averageResponseTime() );
         lua_setfield( state->lua, -2, "averageResponseTime" );
         lua_pushnumber( state->lua, m_metrics.throughput() );
         lua_setfield( state->lua, -2, "throughput" );
         lua_pushnumber( state->lua, m_metrics.errorRate() );
         lua_setfield( state->lua, -2, "errorRate" );
         lua_setfield( state->lua, -2, "metrics" );
         
         lua_pop( state->lua, 1 );
         
         propeller_utilLockLeave( state->lock );
     }
     
 }
 