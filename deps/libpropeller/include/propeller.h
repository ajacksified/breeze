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

#ifndef _PROPELLER_H_
#define _PROPELLER_H_	

//
//	needed macros for windows
//
#ifdef WIN32
#define PROPELLER_API __stdcall
#ifdef PROPELLER_DLL
#define PROPELLER_APIEXP __declspec(dllexport) 
#else 
#define PROPELLER_APIEXP
#endif //PROPELLER_DLL
#else
#define PROPELLER_APIEXP
#define PROPELLER_API
#endif //WIN32

//
//	Status codes
//
#define PROPELLER_STATUS_SUCCESS 1
#define PROPELLER_STATUS_FAILURE 0


#ifdef __cplusplus
extern "C"
{
#endif

    //
    //	server creation and deletion
    //

    /*!
     * Create server
     *
     * @param port that server should listen on (ip address is INADDR_ANY)
     * @return pointer to created server
     */
    PROPELLER_APIEXP void* PROPELLER_API propeller_serverCreate( unsigned int port );
    /*!
     * Start server (blocking)
     *
     * @param server pointer to server
     * @return 1 on success, 0 on failure
     */
    PROPELLER_APIEXP short PROPELLER_API propeller_serverStart( void* server );
     
    //
    // These functions are not meant to be used since server is meant to live throughout the life of the process
    //
    
    /*!
     * Stop server 
     *
     * @return 1 on success, 0 on failure
     */
    //PROPELLER_APIEXP short PROPELLER_API propeller_serverStop( void* server );
     /*!
     * Free memory allocated by the server
     *
     * @return none
     */
    //PROPELLER_APIEXP void PROPELLER_API propeller_serverDestroy( void* server );

    //
    //	Server initialization functions (invoke before server start)
    //

    /*
     * Set number of connection threads
     *
     * @param server pointer to server
     * @param threads number of connection threads (default is 5)
     * @return 0 on success, 1 on failure
     */
    PROPELLER_APIEXP short PROPELLER_API propeller_serverSetConnectionThreadCount( void* server, unsigned int threads );
    /*!
     * Get number of connection threads
     *
     * @param server pointer to server
     * @return number of connection threads
     */
    PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetConnectionThreadCount( void* server );
    /*
     * Set number of process pool threads
     *
     * @param server pointer to server
     * @param threads number of process  pool threads (default is 10)
     * @return 0 on success, 1 on failure
     */
    PROPELLER_APIEXP short PROPELLER_API propeller_serverSetPoolThreadCount( void* server, unsigned int threads );
    /*!
     * Get number of process pool threads
     *
     * @param server pointer to server
     * @return number of process pool threads
     */
    PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetPoolThreadCount( void* server );
    /*!
     * Set connection read timeout
     *
     * @param server pointer to server
     * @param server timeout new read timeout value in seconds (default not set)
     * @return 1 on success, 0 on failure
     */
    PROPELLER_APIEXP short PROPELLER_API propeller_serverSetConnectionReadTimeout( void* server, unsigned int timeout );
     /*!
     * Set connection write timeout
     *
     * @param server pointer to server
     * @param server timeout new write timeout value in seconds (default not set)
     * @return 1 on success, 0 on failure
     */
    PROPELLER_APIEXP short PROPELLER_API propeller_serverSetConnectionWriteTimeout( void* server, unsigned int timeout );
     /*!
     * Get connection read timeout
     *
     * @param server pointer to server
     * @return connection read timeout
     */
    PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetConnectionReadTimeout( void* server );
     /*!
     * Get connection write timeout
     *
     * @param server pointer to server
     * @return connection write timeout
     */
    PROPELLER_APIEXP unsigned int PROPELLER_API propeller_serverGetConnectionWriteTimeout( void* server );

    //
    //	HTTP functions
    //

    /*!
     * HTTP request handler callback definition
     *
     * @param request pointer to request
     * @param response pointer to response
     * @param data custom data
     */
     typedef void ( PROPELLER_API *OnRequest ) ( const void* request, void* response, void* data, void* threadData );
     /*!
     * callback function called when new request process thread is started
     *
     * @param threadData pointer to pointer to data associated  with a thread
     * @param data custom data
     * @param lock thread lock
     */
     typedef void ( PROPELLER_API *OnThreadStarted ) ( void** threadData, void* data, void* lock );
     /*!
     * callback function called when request process thread is stopped
     *
     * @param threadData pointer to data associated  with a thread
     * @param data custom data
     */
     typedef void ( PROPELLER_API *OnThreadStopped ) ( void* threadData, void* data );
     
     typedef void ( PROPELLER_API *TimerCallback ) ( void* data );
          
     
    /*!
     * Set  handler callback
     *
     * @param server pointer to server
     * @param pointer to handler function to be called back
     * @param custom data (or NULL)
     * @return 0 on success, 1 on failure
     */
    PROPELLER_APIEXP short PROPELLER_API propeller_serverSetOnRequestCallback( void* server, OnRequest callback, void* data );
    /*!
     * Set  thread started callback
     *
     * @param server pointer to server
     * @param pointer to function to be called back
     * @param custom data (or NULL)
     * @return 0 on success, 1 on failure
     */
    PROPELLER_APIEXP short PROPELLER_API propeller_serverSetOnThreadStartedCallback( void* server, OnThreadStarted callback, void* data );
    /*!
     * Set  thread stopped callback
     *
     * @param server pointer to server
     * @param pointer to function to be called back
     * @param custom data (or NULL)
     * @return 0 on success, 1 on failure
     */
    PROPELLER_APIEXP short PROPELLER_API propeller_serverSetOnThreadStoppedCallback( void* server, OnThreadStopped callback, void* data );

    PROPELLER_APIEXP short PROPELLER_API propeller_serverSetTimer( void* server, unsigned int interval, TimerCallback callback, void* data );
    
    /*!
     * Get request body string
     *
     * @param request pointer to request
     * @return request body as a string
     */
    PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetBody( const void* request );
    /*!
     * Get request URI
     *
     * @param request pointer to request
     * @return URI string
     */
    PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetUri( const void* request );
    /*!
     * Get request header
     *
     * @param request pointer to request
     * @param request name header name
     *
     * @return header value
     */
    PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetHeader( const void* request, const char* name );
    /*!
     * Get request method
     *
     * @param request pointer to request
     * @return request method string
     */
    PROPELLER_APIEXP const char* PROPELLER_API propeller_requestGetMethod( const void* request );
    
    //
    //	responses
    //

    /*!
     * Get response HTTP status code
     *
     * @param response pointer to response
     * @return status code
     */
    PROPELLER_APIEXP unsigned int PROPELLER_API propeller_responseGetStatus( const void* response );
    /*!
     * Set response HTTP status code
     *
     * @param response pointer to response
     * @return none
     */
    PROPELLER_APIEXP void PROPELLER_API propeller_responseSetStatus( void* response, unsigned int status );
    /*!
     * Set response header
     *
     * @param response pointer to response
     * @param name header name
     * @param name header value
     * @return none
     */
    PROPELLER_APIEXP void PROPELLER_API propeller_responseSetHeader( void* response, const char* name, const char* value );
    /*!
     * Set response body
     *
     * @param response pointer to response
     * @param name string containing reponse body
     * @param length body length (or zero if body is null terminated string)
     * @return none
     */
    PROPELLER_APIEXP void PROPELLER_API propeller_responseSetBody( void* response, const char* body, unsigned int length );
    
    PROPELLER_APIEXP void PROPELLER_API propeller_utilLockEnter( void* lock );
    
    PROPELLER_APIEXP void PROPELLER_API propeller_utilLockLeave( void* lock );
    

#ifdef __cplusplus
}
#endif

#endif //_PROPELLER_H_

