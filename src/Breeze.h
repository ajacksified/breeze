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

#ifndef _BREEZE_H
#define	_BREEZE_H

#include "common.h"

//
//  include lua
//
#define LUA_COMPAT_MODULE
#include <lua.hpp>
#include "Metrics.h"
#include <Server.h>

struct ThreadState
{
    ThreadState( lua_State* _lua, sys::Lock* _lock )
    : lua( _lua ), lock( _lock )
    {
    }
    
    lua_State* lua;
    sys::Lock* lock;
    Metrics metrics;
};

class Breeze
{
public:
    
    virtual ~Breeze();
    void run();
    

    static Breeze* instance();
    static Breeze* create( unsigned int port );

    void setEnvironment( const char* environment );
    void addPath( const std::string& path )
    {
        m_paths.push_back( path );
    }
    
    void setScript( const std::string& script )
    {
        m_script = script;
    }

private:
    Breeze( unsigned int port );
    
    static void onRequestStatic( const propeller::Request* request, propeller::Response* response, void* data, void* threadData );
    void onRequest( const propeller::Request* request, propeller::Response* response, void* threadData );

    static void onThreadStartedStatic( void** threadData, void* data, sys::Lock* lock );
    void onThreadStarted( void** threadData, sys::Lock* lock );

    bool loadScript( lua_State* lua );
    void loadLibraries( lua_State* lua );
    
    static void onTimerStatic( void* data );
    void onTimer();
    
    void setRequestData( lua_State* lua, const propeller::Request* request );
    void setResponseData( lua_State* lua, propeller::Response* response );
    
    
private:
    std::string m_script;
    std::map< lua_State*, int > m_onRequest;
    static Breeze* m_instance;
    bool m_development;
    std::string m_environment;
    std::list< ThreadState* > m_threadStates;
    Metrics m_metrics;
    unsigned int m_dataCollectTimeout;
    std::list< std::string > m_paths;
    propeller::Server m_server;
};

#endif	/* _BREEZE_H */

