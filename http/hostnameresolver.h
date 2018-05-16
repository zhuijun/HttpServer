#ifndef BASE_HTTP_HOSTNAMERESOLVER_H
#define BASE_HTTP_HOSTNAMERESOLVER_H


#include "httpclient.h"
#include "../thread/task.h"

namespace base
{
    namespace http
    {
        class HostnameResolver : public base::thread::AsyncTask
        {
        public:
            HostnameResolver(const std::string& host, std::function<void(DnsRecord& result)> callback);

            virtual void DoResolver();

        private:
            virtual void doInWorkThread();
            virtual void onPostExecute();

            std::string host_;
            DnsRecord result_;
            std::function<void(DnsRecord& result)> callback_;
        };
    }
}

#endif // HOSTNAMERESOLVER_H
