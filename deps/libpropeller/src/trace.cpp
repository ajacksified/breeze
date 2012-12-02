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

#include "trace.h"
#include "system.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdarg.h>


#ifdef _PROPELLER_DEBUG
static unsigned long TRACE_LEVEL = TRACE_LEVEL_INFO;
#else
static unsigned long TRACE_LEVEL = TRACE_LEVEL_ERROR;
#endif

void traceText ( const char* scope, unsigned long level, const char *format, ... )
{

    //
    // Check do we need to process request
    //
    if ( level <= TRACE_LEVEL )
    {
        char temp[ 0x10000 ];
        unsigned int bytes = 0;

        va_list next;

        va_start( next, format );

        //
        // Format the string
        //
        memset( &temp, 0, sizeof( temp ) );

        

        if ( level <= TRACE_LEVEL_ERROR )
        {
            strcat( temp, "*Error*: " );
        }
        else
        {
            char time[32] = "0";
            sprintf( time, "%u ", sys::General::getTime( ) );
            strcat( temp, time );
        }

        if ( scope )
        {
            sprintf( temp + strlen( temp ), "%s: ", scope );
        }


        



        vsnprintf( temp + strlen( temp ), sizeof( temp ) - strlen( temp ) - 1, format, next );

        //
        //	append new line
        //
        strcat( temp, "\n" );

        //
        // Dump just generated string to the debug output
        //
        //#ifdef WIN32
        //      OutputDebugStringA( temp );
        //#else
        fprintf( stderr, "%s", temp );
        //#endif

        va_end( next );
    }
}

