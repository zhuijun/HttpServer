#include "hostnameresolver.h"

#include <stdlib.h>
#include <time.h>
#include <WinSock2.h>
#include "../net/dispatcher.h"

namespace base
{
    namespace http
    {
        using namespace std;

        HostnameResolver::HostnameResolver(const std::string& host, std::function<void(DnsRecord& result)> callback)
            : host_(host), callback_(callback)
        {
            result_.hostname = host_;
        }

        void HostnameResolver::DoResolver()
        {
            int error = 0;
            int rc = 0;
            struct hostent* he;
            he = gethostbyname(host_.c_str());

            if (he != NULL) {
                struct in_addr** addr_list = (struct in_addr**) he->h_addr_list;
                for (int i = 0; addr_list[i] != NULL; ++i) {
                    string ip(inet_ntoa(*addr_list[i]));
                    result_.ipList.emplace_back(std::move(ip));
                }
                if (!result_.empty()) {
                    result_.createTs =Dispatcher::instance().GetTimestampCache();
                }
            }
        }

        void HostnameResolver::doInWorkThread()
        {
            DoResolver();
        }

        void HostnameResolver::onPostExecute()
        {
            callback_(result_);
        }
    }
}
