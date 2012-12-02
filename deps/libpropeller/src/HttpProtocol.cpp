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

#include "HttpProtocol.h"


HttpProtocol::MethodMap HttpProtocol::s_methods;

HttpProtocol::ReasonMap HttpProtocol::s_reasons;

const std::string empty;

void HttpProtocol::initialize ( )
{
    s_methods["GET"] = "GET";
    s_methods["HEAD"] = "HEAD";
    s_methods["POST"] = "POST";
    s_methods["PUT"] = "PUT";
    s_methods["DELETE"] = "DELETE";

    s_reasons[ Continue ] = "Continue";
    s_reasons[ SwitchingProtocols ] = "Switching Protocols";

    s_reasons[ Ok ] = "OK";
    s_reasons[ Created ] = "Created";
    s_reasons[ Accepted ] = "Accepted";
    s_reasons[ NonAuthoritativeInformation ] = "Non-Authoritative Information";
    s_reasons[ NoContent ] = "No Content";
    s_reasons[ ResetContent ] = "Reset Content";
    s_reasons[ PartialContent ] = "Partial Content";

    s_reasons[ MultipleChoices ] = "Multiple Choices";
    s_reasons[ MovedPermanently ] = "Moved Permanently";
    s_reasons[ Found ] = "Found";
    s_reasons[ SeeOther ] = "See Other";
    s_reasons[ NotModified ] = "Not Modified";
    s_reasons[ UseProxy ] = "Use Proxy";
    s_reasons[ Unused ] = "Unused";
    s_reasons[ TemporaryRedirect ] = "Temporary Redirect";

    s_reasons[ BadRequest ] = "Bad Request";
    s_reasons[ Unauthorized ] = "Unauthorized";
    s_reasons[ PaymentRequired ] = "Payment Required";
    s_reasons[ Forbidden ] = "Forbidden";
    s_reasons[ NotFound ] = "Not Found";
    s_reasons[ MethodNotAllowed ] = "Method Not Allowed";
    s_reasons[ NotAcceptable ] = "NotAcceptable";
    s_reasons[ ProxyAuthenticationRequired ] = "Proxy Authentication Required";
    s_reasons[ RequestTimeout ] = "Request Timeout";
    s_reasons[ Conflict ] = "Conflict";
    s_reasons[ Gone ] = "Gone";
    s_reasons[ LengthRequired ] = "Length Required";
    s_reasons[ PreconditionFailed ] = "Precondition Failed";
    s_reasons[ RequestEntityTooLarge ] = "Request Entity Too Large";
    s_reasons[ RequestURITooLong ] = "Request-URI Too Long";
    s_reasons[ UnsupportedMediaType ] = "Unsupported Media Type";
    s_reasons[ RequestedRangeNotSatisfiable ] = "Requested Range Not Satisfiable";
    s_reasons[ ExpectationFailed ] = "Expectation Failed";

    s_reasons[ InternalServerError ] = "Internal Server Error";
    s_reasons[ NotImplemented ] = "Not Implemented";
    s_reasons[ BadGateway ] = "Bad Gateway";
    s_reasons[ GatewayTimeout ] = "Gateway Timeout";
    s_reasons[ HTTPVersionNotSupported ] = "HTTP Version Not Supported";
}

const std::string& HttpProtocol::method ( const char* methodString )
{
    MethodMap::iterator i = s_methods.find( methodString );

    if ( i != s_methods.end( ) )
    {
        return i->second;
    }

    return empty;
}

const std::string& HttpProtocol::reason ( unsigned int status )
{
    ReasonMap::iterator i = s_reasons.find( status );

    if ( i != s_reasons.end( ) )
    {
        return i->second;
    }

    return empty;
}

