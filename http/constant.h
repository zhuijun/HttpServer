#ifndef BASE_HTTP_CONSTANT_H
#define BASE_HTTP_CONSTANT_H

namespace base
{
    namespace http
    {
        enum class HttpMethod
        {
            DELETE1 = 0,
            GET = 1,
            HEAD = 2,
            POST = 3,
            PUT = 4,
            CONNECT = 5,
            OPTIONS = 6,
            TRACE = 7
        };
        
        const char* HTTP_METHOD_STRING(HttpMethod method);

        enum class HttpStatusCode : int
        {
            Continue                      = 100,
            Switching_Protocols           = 101,
            OK                            = 200,
            Created                       = 201,
            Accepted                      = 202,
            Non_Authoritative_Information = 203,
            No_Content                    = 204,
            Reset_Content                 = 205,
            Partial_Content               = 206,
            Multiple_Choices              = 300,
            Moved_Permanently             = 301,
            Found                         = 302,
            See_Other                     = 303,
            Not_Modified                  = 304,
            Use_Proxy                     = 305,
            Temporary_Redirect            = 307,
            Bad_Request                   = 400,
            Unauthorized                  = 401,
            Payment_Required              = 402,
            Forbidden                     = 403,
            Not_Found                     = 404,
            Method_Not_Allowed            = 405,
            Not_Acceptable                = 406,
            Proxy_Authentication_Required = 407,
            Request_Timeout               = 408,
            Conflict                      = 409,
            Gone                          = 410,
            Length_Required               = 411,
            Precondition_Failed           = 412,
            Payload_Too_Large             = 413,
            URI_Too_Long                  = 414,
            Unsupported_Media_Type        = 415,
            Range_Not_Satisfiable         = 416,
            Expectation_Failed            = 417,
            Upgrade_Required              = 426,
            Internal_Server_Error         = 500,
            Not_Implemented               = 501,
            Bad_Gateway                   = 502,
            Service_Unavailable           = 503,
            Gateway_Timeout               = 504,
            HTTP_Version_Not_Supported    = 505,
        };

        const char* HTTP_STATUS_STRING(HttpStatusCode statusCode);
    }
}

#endif
