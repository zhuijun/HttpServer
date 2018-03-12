#ifndef BASE_HTTP_HTTPCLIENT_H
#define BASE_HTTP_HTTPCLIENT_H

#include "observer.h"
#include "constant.h"
#include <vector>
#include <unordered_map>
#include <functional>

namespace base
{
    namespace memory
    {
        class MemoryPool;
    }

    namespace http
    {

        struct DnsRecord {
            std::string hostname;
            std::vector<std::string> ipList;
            std::string getIP() const {
                return ipList.empty() ? "" : ipList.front();
            }
            bool empty() const {
                return ipList.empty();
            }
            int64_t createTs = 0;
        };

        struct HttpClientEventHandler {

            virtual ~HttpClientEventHandler() {}
            virtual void OnHttpClose() = 0;
            virtual void OnHttpResponse(HttpStatusCode code, const std::string& body) = 0;
        };

        class HttpConnection;

        class HttpClient
        {
        public:
            static HttpClient* Create();
            static void Destroy();
            static HttpClient* instance();

            void ResolveHostname(const std::string& hostname);

            enum class Error
            {
                OK,
                BAD_URL,
            };

            Error GetAsync(const std::string& url, const std::vector<std::pair<std::string, std::string>>& formParams, HttpClientEventHandler* evtHandler);
            Error PostFormAsync(const std::string& url, const std::vector<std::pair<std::string, std::string>>& formParams, HttpClientEventHandler* evtHandler);
            Error PostJsonAsync(const std::string& url, const std::string& json, HttpClientEventHandler* evtHandler);

        private:
            HttpClient(){}
            virtual ~HttpClient();

            std::unordered_map<std::string, DnsRecord> dns_cache_;
            std::vector<HttpConnection*> connections_;
            base::AutoObserver autoObserver_;
        };
    }
}

#endif // HTTPCLIENT_H
