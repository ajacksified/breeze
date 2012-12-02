libpropeller is super fast, asynchronous, lightweight, multithreaded HTTP server library 

Multithreaded architecture allows having synchronous code in the callbacks that are called asynchronously

**Building:**

    git clone git@github.com:sergeyzavadski/libpropeller.git
    cd libpropeller
    configure && make
    sudo make install

**Using**

Create new file TestServer.c

    #include <propeller.h>
    #include <stdlib.h>

    void handle( void* request, void* response, void* data, void* threadData )
    {
        propeller_responseSetBody( response, "test" );
    }

    int main ( int argc, char** argv )
    {
        void* server = propeller_serverCreate( 8080 );

        propeller_serverSetHttpHandler( server, handle, NULL );

        propeller_serverStart( server );

        return 0;
    }


then

    gcc TestServer.c -lpropeller -o TestServer
    
and run

    ./TestServer
    
Now you have a webserver running on port 8080 that can handle about 20000 requests  per second

To see full API please refer to `include/propeller.h`


**Supported platforms**  
liboropeller has been tested on linux and OS X, and *should* be building on other platforms that support POSIX. Windows support is not present currently but is relatively easy to add as it requires just modifications to build files

    
    
    

