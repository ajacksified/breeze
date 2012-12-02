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


class Breeze
{
public:
    
    virtual ~Breeze();
    void run();
    

    static Breeze* instance();
    static Breeze* create( unsigned int port, const std::string& script );

    void setEnvironment( const char* environment );

private:
    Breeze( unsigned int port, const std::string& script );
    
    static void onRequestStatic( const void* request, void* response, void* data, void* threadData );
    void onRequest( const void* request, void* response, void* threadData );

    static void onThreadStartedStatic( void** threadData, void* data );
    void onThreadStarted( void** threadData );

    bool loadScript( lua_State* lua );
    void loadLibraries( lua_State* lua );
    

private:
    std::string m_script;
    std::map< lua_State*, int > m_onRequest;
    static Breeze* m_instance;
    void* m_server;
    bool m_development;
    std::string m_environment;
};

#endif	/* _BREEZE_H */

