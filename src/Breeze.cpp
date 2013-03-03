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
#include "trace.h"
#include <lua_cjson.h>

Breeze* Breeze::m_instance = NULL;

Breeze::Breeze( unsigned int port )
: m_development( false ), m_dataCollectTimeout( 5 ), m_server( port, ( propeller::Server::EventHandler& ) *this ), m_connectionThreads( 10 ), m_poolThreads( 30 )
{
    
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

void Breeze::onRequest( const propeller::Request& req, propeller::Response& res, sys::ThreadPool::Worker& thread )
{
    ThreadState* state = ( ThreadState* ) thread.data();
    
    lua_State* lua = state->lua;
    
    //
    //  obtain the lock (since metrics are collected from the other thread)
    //
    sys::LockEnterLeave lock( thread.lock() );
    
    const propeller::http::Request& request = ( const propeller::http::Request& ) req;
    propeller::http::Response& response = ( propeller::http::Response& ) res;
    
    //
    //  reload script in development mode
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
            response.setStatus( 500 );
            response.setBody( lua_tostring( lua, -1 ) );
            lua_pop( lua, 1 );
            return;
        }
    }

    
    lua_getglobal( lua, "debug");
    lua_getfield( lua, -1, "traceback");
    
    lua_getglobal( lua, "__onRequest" );
    
    //
    //  export request to lua
    //
    lua_newtable( lua );
         
    lua_pushstring( lua, request.body() );
    lua_setfield( lua, -2, "body" );
    
    lua_pushstring( lua, request.uri() );
    lua_setfield( lua, -2, "url" );
    
    lua_pushstring( lua, request.method() );
    lua_setfield( lua, -2, "method" );
    
    lua_newtable( lua );
    
    for ( propeller::http::Request::HeaderMap::const_iterator i = request.headers().begin(); i != request.headers().end(); i++ )
    {
        lua_pushstring( lua, i->second.c_str() );
        lua_setfield( lua, -2, i->first.c_str() );
    }
    
    lua_setfield( lua, -2, "headers" );
    
    lua_setglobal( lua, "__req" );
    
    
    //
    //  call lua 
    //
    int result = lua_pcall( lua, 0, 0, -2 );

    if (result > 0)
    {
        //
        //  error executing lua
        //
        TRACE_ERROR("%s", lua_tostring( lua, -1 ));

        response.setStatus( 500 );
        if ( m_development )
        {
            response.setBody( lua_tostring( lua, -1 ) );
        }
         
        lua_pop( lua, 1 );
    }
    
    //
    //  remove debug.traceback from stack
    //
    lua_remove( lua, -1 );
    lua_remove( lua, -2 );
    
    //
    //  load response from lua
    //
    lua_getglobal( lua, "__res" );

    lua_getfield( lua, -1, "status" );
    response.setStatus( lua_tounsigned( lua, -1 ) );
    lua_pop( lua, 1 );
    
    lua_getfield( lua, -1, "headers" );

    lua_pushnil( lua );
    while ( lua_next( lua, -2 ) != 0 )
    {
        const char* name = lua_tostring( lua, -2 );
        const char* value = lua_tostring( lua, -1 );

        response.addHeader( name, value );

        lua_pop( lua, 1 );
    }
    
    lua_pop( lua, 1 );
    
    lua_getfield( lua, -1, "body" );
    size_t length = 0;
    const char* body = lua_tolstring( lua, -1, &length );

    response.setBody( body, length );
    lua_pop( lua, 1 );
    
    lua_pop( lua, 1 );
    
    
    unsigned int responseTime = sys::General::getMillisecondTimestamp() - request.timestamp();
    
    //
    //  collect high level metrics
    //
    state->metrics.collect( 
        responseTime, 
        response.status() > 500,    
        request.uri()
    );
 }

 void Breeze::onThreadStarted( sys::ThreadPool::Worker& thread )
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
     
         
     thread.setData( new ThreadState( lua ) );
     
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
    //  create empty table
    //
    lua_newtable( lua );
    lua_setglobal( lua, "breezeApi" );

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

    for ( std::list< std::string >::iterator i = m_paths.begin( ); i != m_paths.end( ); i++ )
    {
        path.append( ";" );
        path.append( i->c_str( ) );
        path.append( "/?.lua" );
        
        path.append( ";" );
        path.append( i->c_str( ) );
        path.append( "/?/init.lua" );
    }

    TRACE( "%s", path.c_str( ) );

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
     m_server.setConnectionReadTimeout( 30 );
     m_server.setConnectionWriteTimeout( 30 );
     
     
     if ( m_development )
     {
         //
         // limit number of threads in development mode
         //
         m_server.setPoolThreadCount( 1 );
         m_server.setConnectionThreadCount( 1 );
     }
     else
     {
         m_server.setPoolThreadCount( m_poolThreads );
         m_server.setConnectionThreadCount( m_connectionThreads );
     }
          
     
     m_server.addTimer( m_dataCollectTimeout );
      
     m_server.start();
 }

 void Breeze::onTimer( unsigned int interval, void* data )
 {
     //
     // collect statistics from all threads
     //
     sys::ThreadPool::WorkerList& threads = m_server.threads();
     for ( sys::ThreadPool::WorkerList::iterator i = threads.begin(); i != threads.end(); i++ )
     {
         sys::ThreadPool::Worker* worker = *i;
         ThreadState* state = ( ThreadState* ) worker->data();
         
         sys::LockEnterLeave lock( worker->lock() );
         
         m_metrics.add( state->metrics, i == threads.begin() ? m_dataCollectTimeout : 0 );
         state->metrics.reset();
     }
     
     //
     // export stats to lua
     //
     for ( sys::ThreadPool::WorkerList::iterator i = threads.begin(); i != threads.end(); i++ )
     {
         sys::ThreadPool::Worker* worker = *i;
         ThreadState* state = ( ThreadState* ) worker->data();
         
         sys::LockEnterLeave lock( worker->lock() );
         
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
     }
 }
 