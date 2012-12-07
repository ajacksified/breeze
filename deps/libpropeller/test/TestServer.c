#include <propeller.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <execinfo.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/time.h>

void* server = NULL;

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, 2);
  exit(1);
}


void onRequest( const void* request, void* response, void* data, void* threadData )
{
    propeller_responseSetBody( response, "test" );
}

void onThreadStarted( void** threadData, void* data )
{
    *threadData = ( void* ) 5;
}

int main ( int argc, char** argv )
{

     signal(SIGSEGV, handler); 
      
    
    
    //
    //  create server
    //
    server = propeller_serverCreate( 8080 );
    //
    // set hadler callback
    //
    propeller_serverSetOnRequestCallback( server, onRequest, NULL );

    propeller_serverSetOnThreadStartedCallback( server, onThreadStarted, NULL );
    //
    //  start server
    //
    propeller_serverStart( server );

    return 0;
}

