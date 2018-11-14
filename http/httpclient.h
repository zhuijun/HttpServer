#ifndef BASE_HTTP_HTTPCLIENT_H
#define BASE_HTTP_HTTPCLIENT_H

#include "Observer.h"
#include "constant.h"
#include <vector>
#include <map>
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

        typedef std::function<void()> HttpCloseCallBack;
        typedef std::function<void(HttpStatusCode code, const std::string& body, const std::string& request)> HttpResponseCallBack;

        class HttpConnection;
        class HttpClientImpl;
        class HttpClient
        {
        public:
            static HttpClient* Create();
            static void Destroy();
            static HttpClient* instance();
            std::function<void(const std::string message)> log;

            void ResolveHostname(const std::string& hostname, std::function<void(const DnsRecord& result)> callback);

            enum class Error
            {
                OK,
                BAD_URL,
            };

            Error GetAsync(const std::string& url, const std::vector<std::pair<std::string, std::string>>& formParams, HttpResponseCallBack onResponse, int timeoutSecond = 30);
            Error PostFormAsync(const std::string& url, const std::vector<std::pair<std::string, std::string>>& formParams, HttpResponseCallBack onResponse, int timeoutSecond = 30);
            Error PostJsonAsync(const std::string& url, const std::string& json, HttpResponseCallBack onResponse, int timeoutSecond = 30);

        private:
            HttpClient();
            virtual ~HttpClient();

            std::map<std::string, DnsRecord> dns_cache_;
            HttpClientImpl * m_impl = nullptr;
        };
    }
}

#endif // HTTPCLIENT_H
