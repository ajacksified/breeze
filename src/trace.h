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

#ifndef _TRACE_H_
#define _TRACE_H_

#include <string.h>
#include <string>
#include <typeinfo>


#define	TRACE_LEVEL_INFO			3
#define TRACE_LEVEL_WARNING			2	
#define TRACE_LEVEL_ERROR			1	

//#ifdef __PRETTY_FUNCTION__
//#else
//#define __PRETTY_FUNCTION__ __FUNCTION__
//#endif

void
traceText ( const char* scope, unsigned long level, const char *format, ... );

class TraceEnterLeave
{
public:

    TraceEnterLeave ( const char* function )
    : m_function ( function )
    {
        size_t brace = m_function.find ( '(' );

        if ( brace )
        {
            m_function[brace] = 0 ;
            m_function.append ( "()" );
        }

        traceText ( NULL, TRACE_LEVEL_INFO, "Entering %s ", m_function.c_str ( ) );
    }

    ~TraceEnterLeave ( )
    {
        traceText ( NULL, TRACE_LEVEL_INFO, "Leaving %s ", m_function.c_str ( ) );
    }

private:
    std::string m_function;
};


#ifdef _DEBUG
#define TRACE_ENTERLEAVE()  TraceEnterLeave trace( __PRETTY_FUNCTION__ )
#define TRACE( format, ... ) traceText( __FUNCTION__, TRACE_LEVEL_INFO, format, __VA_ARGS__ )
#define TRACE_WARNING( format, ... ) traceText( __FUNCTION__, TRACE_LEVEL_WARNING, format, __VA_ARGS__ )
#define TRACE_ERROR( format, ... ) traceText( __FUNCTION__, TRACE_LEVEL_ERROR, format, __VA_ARGS__ )

#else
#define TRACE_ENTERLEAVE() TraceEnterLeave trace( __FUNCTION__ )
#define TRACE( format, ... )
#define TRACE_WARNING( format, ... )
#define TRACE_ERROR( format, ... ) traceText( __FUNCTION__, TRACE_LEVEL_ERROR, format, __VA_ARGS__ )

#endif



#endif //_TRACE_H_
