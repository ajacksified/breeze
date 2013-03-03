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

#include <propeller/HttpServer.h>

//
//  include lua
//
#define LUA_COMPAT_MODULE
#include <lua.hpp>
#include "Metrics.h"


struct ThreadState
{
    ThreadState( lua_State* _lua )
    : lua( _lua )
    {
    }
    
    lua_State* lua;
    Metrics metrics;
};

class Breeze : public propeller::Server::EventHandler
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
    
    virtual void onRequest( const propeller::Request& request, propeller::Response&, sys::ThreadPool::Worker& thread );
    virtual void onThreadStarted( sys::ThreadPool::Worker& thread );
    virtual void onTimer( unsigned int interval, void* data );
    
    bool loadScript( lua_State* lua );
    void loadLibraries( lua_State* lua );
    
    void setConnectionThreads( unsigned int connectionThreads )
    {
        m_connectionThreads = connectionThreads;
    }
    
    void setPoolThreads( unsigned int poolThreads )
    {
        m_poolThreads = poolThreads;
    }
    
    
private:
    std::string m_script;
    std::map< lua_State*, int > m_onRequest;
    static Breeze* m_instance;
    bool m_development;
    std::string m_environment;
    Metrics m_metrics;
    unsigned int m_dataCollectTimeout;
    std::list< std::string > m_paths;
    propeller::http::Server m_server;
    unsigned int m_connectionThreads;
    unsigned int m_poolThreads;
    
};

#endif	/* _BREEZE_H */

