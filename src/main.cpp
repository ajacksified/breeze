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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Breeze.h"

#include <signal.h>

#include <execinfo.h>
#include "version.h"



void sigSegvHandler( int sig ) {
  void *array[10];
  size_t size = backtrace( array, 10 );
  backtrace_symbols_fd( array, size, 2 );
  exit(1);
}

class CmdOption
{
public:
    CmdOption( const std::string& optShort, const std::string& optLong, const std::string& description, std::string name = "", bool needValue = false )
    : m_long( optLong ), m_short( optShort ), m_description( description ), m_needValue( needValue ), m_name( name )
    {
        m_value[0] = 0;
    }

    CmdOption( const CmdOption& option )
    {
        m_value[0] = 0;
        m_long = option.m_long;
        m_short = option.m_short;
        m_needValue = option.m_needValue;
        m_description = option.m_description;
        m_name = option.m_name;
    }

    std::string usage()
    {
        std::string usage;

        if ( m_short.size() > 0)
        {
            usage += m_short + ", ";
        }

        if ( m_long.size() > 0 )
        {
            usage += m_long;
            if ( m_needValue )
            {
                usage += "=val";
            }
        }

        usage += "\t\t" + m_description;

        return usage;
    }

    const char* value() const
    {
        return m_value;
    }

    bool parse( const char* arg ) const
    {
        if ( strcmp( arg, m_short.c_str() ) == 0 || strstr( arg, m_long.c_str() ) )
        {
            if ( m_needValue )
            {
                std::string format = m_long + "=%s";
                if ( sscanf( arg, format.c_str(), m_value ) != 1 )
                {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    static bool isOption( const char* arg )
    {
        return arg[0] == '-';
    }

    const std::string& name() const
    {
        return m_name;
    }

private:
    std::string m_long;
    std::string m_short;
    bool m_needValue;

    std::string m_description;
    char m_value[128];
    std::string m_name;
};

std::vector< CmdOption > options;

void usage( bool help = false )
{
    printf("Usage: breeze [options] [script] \n\n");
    printf("Options: \n");

    for ( int  i = 0; i < options.size(); i++ )
    {
        printf( "%s\n", options[i].usage().c_str() );
    }

    if( help )
    {
        printf("\nEnvironment variables:\n");
        printf("BREEZE_ENV\tenvironment\n");
    }

    exit( 1 );
}

const CmdOption* findOption( const char* arg )
{
    for ( int i = 0; i < options.size( ); i++ )
    {
        const CmdOption* option = &options[i];
        
        if ( option->parse( arg ) )
        {
            return option;
        }
    }
    
    return NULL;
}

int main( int argc, char** argv )
{

    signal( SIGSEGV, sigSegvHandler );

    options.push_back( CmdOption( "", "--port", "listening port", "port", true ) );
    options.push_back( CmdOption( "-h", "--help", "prints help", "help" ) );
    options.push_back( CmdOption( "-v", "--version", "prints version", "version" ) );
//    options.push_back( CmdOption( "", "--connectionThreads", "connection threads", "connectionThreads", true ) );
  //  options.push_back( CmdOption( "", "--poolThreads", "connection threads", "poolThreads", true ) );
    
    //
    //  parse command line
    //
    std::string script;
    unsigned int port = 8080;
    
    try
    {
        for ( int i = 1; i < argc; i++ )
        {
            if ( CmdOption::isOption( argv[i] ) )
            {
                const CmdOption* option = findOption( argv[i] );

                if ( !option )
                {
                    printf( "Unrecognized option %s \n", argv[i] );
                    throw argv[i];
                }

                if ( option->name( ) == "help" )
                {
                    usage( true );
                }
                
                if ( option->name( ) == "version" )
                {
                    printf( "%s \n",  BREEZE_VERSION );
                    exit(1);
                }

                if ( option->name( ) == "port" )
                {
                    port = atoi( option->value( ) );
                }
                
                
             }
            else
            {
                script = argv[i];
            }

        }
    }
    catch ( ... )
    {
        usage();
    }

    if ( !script.size() )
    {
        usage( true );
    }
    

    Breeze* breeze = Breeze::create( port, script.c_str() );
    
    breeze->setEnvironment( getenv("BREEZE_ENV") );
    
    try
    {
        printf( "starting server on port %d \n", port );
        breeze->run();
    }
    catch( ... )
    {
        return 1;
    }
    
    return 0;
}

