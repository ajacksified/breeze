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
    TRACE("0x%x", request);
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

    propeller_responseSetBody( response, lua_tostring( lua, -1 ) );
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

Breeze::Breeze( unsigned int port, const std::string& script )
: m_script( script ), m_development( false )
{
    m_server = propeller_serverCreate( port );

    propeller_serverSetOnRequestCallback( m_server, onRequestStatic, this );
    propeller_serverSetOnThreadStartedCallback( m_server, onThreadStartedStatic, this );
}

Breeze::~Breeze( )
{
    propeller_serverDestroy( m_server );
    m_instance = NULL;
}

Breeze* Breeze::instance()
{
    return m_instance;
}

Breeze* Breeze::create( unsigned int port, const std::string& script )
{
    if ( !m_instance )
    {
        m_instance = new Breeze( port, script );
    }

    return m_instance;
}

void Breeze::onRequestStatic( const void* request, void* response, void* data, void* threadData )
{
   ( ( Breeze* ) data )->onRequest( request, response, threadData );
}

void Breeze::onRequest( const void* request, void* response, void* threadData )
{
    lua_State* lua = ( lua_State* ) threadData;

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
            propeller_responseSetBody( response, lua_tostring( lua, -1 ) );
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

    int result = lua_pcall( lua, 2, 0, -4 );

    if (result > 0)
    {
        //
        //  error executing lua
        //
        TRACE_ERROR("%s", lua_tostring( lua, -1 ));
        propeller_responseSetStatus( response, 500 );
        propeller_responseSetBody( response, lua_tostring( lua, -1 ) );
        lua_pop( lua, 1 );
    }

    //
    //  remove debug.traceback from stack
    //
    lua_remove( lua, -1 );
    lua_remove( lua, -2 );
 }

void Breeze::onThreadStartedStatic( void** threadData, void* data )
{
   ( ( Breeze* ) data )->onThreadStarted( threadData );
}

 void Breeze::onThreadStarted( void** threadData )
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

     

     *threadData = ( void* ) lua;

     bool success = loadScript( lua );

     if ( !success )
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
     path.append( ";" );
     path.append( BREEZE_PATH );
     path.append( "/?.lua" );
     lua_pop( lua, 1 );
     lua_pushstring( lua, path.c_str() );
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
     propeller_serverStart( m_server );
     
 }
