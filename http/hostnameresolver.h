#ifndef BASE_HTTP_HOSTNAMERESOLVER_H
#define BASE_HTTP_HOSTNAMERESOLVER_H


#include "httpclient.h"

namespace base
{
    namespace http
    {
        class HostnameResolver
        {
        public:
            HostnameResolver(const std::string& host);

            virtual void DoResolver();

            std::string host_;
            DnsRecord result_;
        };
    }
}

#endif // HOSTNAMERESOLVER_H
