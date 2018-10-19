#include "httpclient.h"

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <list>

#include "http_parser.h"
#include "hostnameresolver.h"
#include "urlbuilder.h"
#include "utils_string.h"
#include "../net/client.h"
#include "../net/dispatcher.h"
#include "../net/BaseBuffer.h"
#include "../thread/threadpool.h"
#include "TimerMgr.h"
//#include "Log.h"

#include "gzip/gzip.hpp"

namespace base
{
    namespace http
    {
        using namespace std;

        static struct http_parser_settings settings;

        /// HttpConnection
        class HttpConnection : public net::Client
        {
        public:
            struct EventHandler {
                virtual void OnHttpRequest(HttpConnection* connection) = 0;
                virtual void OnConnectionClose(HttpConnection* connection) = 0;
            };

            HttpConnection(const HttpResponseCallBack& onResponse, EventHandler* evtHandler)
                : onResponse_(onResponse), m_eventHandler(evtHandler) {
                http_parser_init(&parser_, HTTP_RESPONSE);
                parser_.data = this;
            }

            virtual ~HttpConnection() {
            }

            void WriteRequestHead(HttpMethod method, const string& host, const string& path) {
                if (m_hasHead) {
                    return;
                }
                m_hasHead = true;
                if (method == HttpMethod::GET) {
                    sendBuffer.Add("GET ", 4);
                    sendBuffer.Add(path.c_str(), path.length());
                    sendBuffer.Add(" HTTP/1.1\r\n", 11);
                    sendBuffer.Add("HOST: ", 6);
                    sendBuffer.Add(host.c_str(), host.length());
                    sendBuffer.Add("\r\n", 2);
                    sendBuffer.Add("Content-Length: 0\r\n", 19);
                    sendBuffer.Add("Accept-Encoding: gzip\r\n", 23);
                    std::string ua("User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:61.0) Gecko/20100101 Firefox/61.0\r\n");
                    sendBuffer.Add(ua.c_str(), ua.length());
                    std::string cc("Cache-Control: max-age=1\r\n");
                    sendBuffer.Add(cc.c_str(), cc.length());
                    sendBuffer.Add("\r\n", 2);
                    m_hasBody = true;
                } else {
                    sendBuffer.Add("POST ", 5);
                    sendBuffer.Add(path.c_str(), path.length());
                    sendBuffer.Add(" HTTP/1.1\r\n", 11);
                    sendBuffer.Add("HOST: ", 6);
                    sendBuffer.Add(host.c_str(), host.length());
                    sendBuffer.Add("\r\n", 2);
                }
            }

            void WriteRequestBody(const char* contentType, const std::string& content) {
                if (!m_hasHead || m_hasBody) {
                    return;
                }
                m_hasBody = true;
                string tmp;
                base::utils::string_append_format(tmp, "Content-Type: %s\r\n", contentType);
                sendBuffer.Add(tmp.c_str(), tmp.length());
                tmp.clear();
                uint32_t len = content.size();
                base::utils::string_append_format(tmp, "Content-Length: %u\r\n", len);
                sendBuffer.Add(tmp.c_str(), tmp.length());
                sendBuffer.Add("\r\n", 2);
                sendBuffer.Add(content.c_str(), content.size());
            }

            void WriteRequestBodyWithForm(const std::string& content) {
                WriteRequestBody("application/x-www-form-urlencoded", content);
            }

            void WriteRequestBodyWithJson(const std::string& content) {
                WriteRequestBody("application/json", content);
            }

            void OnReceive(std::size_t count) override {
                if (count > 0)
                {
                    size_t r = http_parser_execute(&parser_, &settings, recvBuffer.Data(), recvBuffer.DataSize());
                    if (r != recvBuffer.DataSize()) {
                        if (onResponse_)
                        {
                            onResponse_(HttpStatusCode::Not_Acceptable, HTTP_STATUS_STRING(HttpStatusCode::Not_Acceptable));
                        }
                        //return;
                    }
                    recvBuffer.Removed(r);
                    recvBuffer.MoveData();
                }
            }

            void OnConnect() override {
                m_lastActiveTs = g_dispatcher->GetTimestampCache();
                if (m_eventHandler)
                {
                    m_eventHandler->OnHttpRequest(this);
                }
                Flush();
            }

            void OnConnectFail(int eno, const char* reason) override {
                if (reason)
                {
                    //LogInst.GameLogAndPrintWarning("Http response code = %d, url = %s, status = %s", eno, reqUrl_.c_str(), reason);
                }
                
                if (onResponse_)
                {
                    onResponse_(HttpStatusCode::Bad_Request, HTTP_STATUS_STRING(HttpStatusCode::Bad_Request));
                }
            }

            void OnTimeOut() {
                //LogInst.GameLogAndPrintWarning("Http response code = %d, url = %s, status = %s", HttpStatusCode::Request_Timeout, reqUrl_.c_str(), HTTP_STATUS_STRING(HttpStatusCode::Request_Timeout));
                if (onResponse_)
                {
                    onResponse_(HttpStatusCode::Request_Timeout, HTTP_STATUS_STRING(HttpStatusCode::Request_Timeout));
                }
            }

            void OnClose() override {
                if (m_eventHandler)
                {
                    m_eventHandler->OnConnectionClose(this);
                }
                Release();
            }

            void Flush() {
                InvokeSend();
            }

            int64_t LastActiveTs() const {
                return m_lastActiveTs;
            }

            int TimeoutSeconds() const{
                return m_timeoutSeconds;
            }

            void SetTimeoutSeconds(int timeoutSeconds) {
                m_timeoutSeconds = timeoutSeconds;
            }

            void SetReqUrl(std::string reqUrl)
            {
                reqUrl_ = reqUrl;
            }

        private:
            bool m_hasHead = false;
            bool m_hasBody = false;

            http_parser parser_;
            std::string resp_body_;
            HttpResponseCallBack onResponse_;
            EventHandler* m_eventHandler = nullptr;
            int64_t m_lastActiveTs = 0;
            int m_timeoutSeconds = 30; //³¬Ê±ÃëÊý
            std::vector<std::string> m_header_fields;
            std::vector<std::string> m_header_values;
            std::string reqUrl_;

        public:
            static int on_message_begin_cb(http_parser* parser) {
                HttpConnection* client = static_cast<HttpConnection*>(parser->data);
                client->resp_body_.reserve(1024);
                return 0;
            }

            static int on_status(http_parser* parser, const char* at, size_t len) {
                return 0;
            }

            static int on_header_field(http_parser* parser, const char* at, size_t len) {
                HttpConnection* client = static_cast<HttpConnection*>(parser->data);
                client->m_header_fields.emplace_back(at, len);
                return 0;
            }

            static int on_header_value(http_parser* parser, const char* at, size_t len) {
                HttpConnection* client = static_cast<HttpConnection*>(parser->data);
                client->m_header_values.emplace_back(at, len);
                return 0;
            }

            static int on_headers_complete(http_parser* parser) {
                return 0;
            }

            static int on_body_cb(http_parser* parser, const char* at, size_t len) {
                HttpConnection* client = static_cast<HttpConnection*>(parser->data);
                client->resp_body_.append(at, len);
                return 0;
            }

            static int on_message_complete_cb(http_parser* parser) {
                HttpConnection* client = static_cast<HttpConnection*>(parser->data);
                if (client->onResponse_)
                {
                    bool is_gzip = false;
                    for (size_t i = 0; i < client->m_header_fields.size(); i++)
                    {
                        if (client->m_header_fields[i] == "Content-Encoding")
                        {
                            if (client->m_header_values.size() > i)
                            {
                                if (client->m_header_values[i] == "gzip")
                                {
                                    is_gzip = true;
                                    Decode::GZIP gz;
                                    gz.decompress((const uint8_t*)client->resp_body_.data(), client->resp_body_.length());
                                    std::string tmp((char*)gz.data, gz.size);
                                    client->onResponse_((HttpStatusCode)parser->status_code, tmp);
                                }
                            }
                            break;
                        }
                    }
                    if (!is_gzip)
                    {
                        client->onResponse_((HttpStatusCode)parser->status_code, client->resp_body_);
                    }
                }

                if ((HttpStatusCode)parser->status_code != base::http::HttpStatusCode::OK)
                {
                    //LogInst.GameLogAndPrintWarning("Http response code = %d, url = %s", parser->status_code, client->reqUrl_.c_str());
                }
                //else
                //{
                //    printf("Http response code = %d, url = %s\n", parser->status_code, client->reqUrl_.c_str());
                //}

                client->Close();
                return 0;
            }
        };

        static struct http_parser_settings_initializer {
            http_parser_settings_initializer() {
                memset(&settings, 0, sizeof(settings));
                settings.on_status = HttpConnection::on_status;
                settings.on_header_field = HttpConnection::on_header_field;
                settings.on_header_value = HttpConnection::on_header_value;
                settings.on_headers_complete = HttpConnection::on_headers_complete;
                settings.on_body = HttpConnection::on_body_cb;
                settings.on_message_begin = HttpConnection::on_message_begin_cb;
                settings.on_message_complete = HttpConnection::on_message_complete_cb;
            }
        } _settings_initializer;


        class HttpClientImpl : public HttpConnection::EventHandler
        {
        public:
            HttpClientImpl(){}
            ~HttpClientImpl()
            {
                UnsafeTimerInst.RemoveFunction(m_aliveTimer);
            }

            virtual void OnHttpRequest(HttpConnection* connection) override
            {
                m_connections.push_back(connection);
            }

            virtual void OnConnectionClose(HttpConnection* connection) override
            {
                m_connections.remove(connection);
            }

            void Start() {
                m_aliveTimer = UnsafeTimerInst.AddStdFunctionTimeRepeat(0, bind(&HttpClientImpl::CheckConnectionIsAlive, this), 30);
            }

        private:
            void CheckConnectionIsAlive() {
                int64_t now = g_dispatcher->GetTimestampCache();
                for (HttpConnection* conn : m_connections)
                {
                    if (now - conn->LastActiveTs() > conn->TimeoutSeconds()) {
                        conn->OnTimeOut();
                        conn->Close();
                    }
                }
            }
            std::list<HttpConnection*> m_connections;
            uint32_t m_aliveTimer = 0;
        };


        HttpClient::HttpClient()
        {
            m_impl = new HttpClientImpl();
            m_impl->Start();
        }

        HttpClient::~HttpClient()
        {
        }

        static HttpClient* s_instance = nullptr;

        HttpClient* HttpClient::Create()
        {
            if (s_instance == nullptr) {
                s_instance = new HttpClient();
            }
            return s_instance;
        }

        void HttpClient::Destroy()
        {
            SAFE_DELETE(s_instance);
        }

        HttpClient* HttpClient::instance()
        {
            return s_instance;
        }

        void HttpClient::ResolveHostname(const std::string& hostname, std::function<void(const DnsRecord& result)> callback)
        {
             //find from cache
            auto it = dns_cache_.find(hostname);
            if (it != dns_cache_.end()) {
                const DnsRecord& record = it->second;
                if (g_dispatcher->GetTimestampCache() - record.createTs < 30 * 60) {
                    callback(record);
                    return;
                }
            }

            auto callback2 = [this, callback](base::http::DnsRecord& result){
                if (!result.empty()) {
                    dns_cache_[result.hostname] = result;
                }
                else {
                    dns_cache_.erase(result.hostname);
                }
                callback(result);
            };

            std::shared_ptr<http::HostnameResolver> resolver(new http::HostnameResolver(hostname, callback2));
            base::thread::ThreadPool::getInstance()->queueUserWorkItem(resolver);
        }

        static HttpClient::Error parseUrl(const string& url, string& host, string& path, int* port)
        {
            http_parser_url urlParser;
            http_parser_url_init(&urlParser);
            int err = http_parser_parse_url(url.c_str(), url.length(), 0, &urlParser);
            if (err) {
                return HttpClient::Error::BAD_URL;
            }
            if ((urlParser.field_set & (1 << UF_HOST)) == 0) {
                return HttpClient::Error::BAD_URL;
            }

            host.assign(url.c_str() + urlParser.field_data[UF_HOST].off, urlParser.field_data[UF_HOST].len);
            *port = urlParser.port == 0 ? 80 : urlParser.port;

            path.assign("/");
            if ((urlParser.field_set & (1 << UF_PATH)) != 0) {
                path.assign(url.c_str() + urlParser.field_data[UF_PATH].off, urlParser.field_data[UF_PATH].len);
            }
            if ((urlParser.field_set & (1 << UF_QUERY)) != 0) {
                path.append("?");
                path.append(url.c_str() + urlParser.field_data[UF_QUERY].off, urlParser.field_data[UF_QUERY].len);
            }
            return HttpClient::Error::OK;
        }

		void HttpClient::ShowLog(const std::string & url, const std::vector<std::pair<std::string, std::string>>& formParams)
		{
			string path(url);
			string content;
			for (size_t i = 0u; i < formParams.size(); ++i) {
				const pair<string, string>& param = formParams[i];
				content.append(UrlBuilder::UrlEncode(param.first));
				content.append("=");
				content.append(UrlBuilder::UrlEncode(param.second));
				if (i < formParams.size() - 1u) {
					content.append("&");
				}
			}

			if (formParams.size() > 0)
			{
				if (path.find_last_of('?') == std::string::npos)
				{
					path.append("?");
				}
				else
				{
					path.append("&");
				}
			}

			path.append(content);
			//LogInst.GameLogAndPrintWarning("Http Request %s", path.c_str());
		}

		HttpClient::Error HttpClient::GetAsync(const string& url, const vector< pair< string, string > >& formParams, HttpResponseCallBack onResponse, int timeoutSecond)
        {
            string host;
            string path;
            int port;
            Error error = parseUrl(url, host, path, &port);
            if (error != Error::OK) {
                onResponse(HttpStatusCode::Bad_Request, "parseUrl fail");
                return error;
            }

            string content;
            for (size_t i = 0u; i < formParams.size(); ++i) {
                const pair<string, string>& param = formParams[i];
                content.append(UrlBuilder::UrlEncode(param.first));
                content.append("=");
                content.append(UrlBuilder::UrlEncode(param.second));
                if (i < formParams.size() - 1u) {
                    content.append("&");
                }
            }

            if (formParams.size() > 0)
            {
                if (path.find_last_of('?') == std::string::npos)
                {
                    path.append("?");
                }
                else
                {
                    path.append("&");
                }
            }

            path.append(content);

            //cout << "req:" << host << ":" << port << path << endl;
            auto callback = [this, host, path, port, onResponse, timeoutSecond](const DnsRecord& result) {
                string ip = result.getIP();
                if (ip.empty()) {
                    if (onResponse)
                    {
                        onResponse(HttpStatusCode::No_Content, "dns resolve fail");
                    }
                }
                else
                {
                    std::string reqUrl("http://");
                    reqUrl.append(host).append(":").append(base::utils::convert(port)).append(path);

                    HttpConnection* conn = new HttpConnection(onResponse, m_impl);
                    conn->SetReqUrl(reqUrl);
                    conn->WriteRequestHead(HttpMethod::GET, host, path);
                    conn->Connect(ip.c_str(), port);
                    conn->SetTimeoutSeconds(timeoutSecond);
                }
            };

            base::Dispatcher::instance().ExecuteAtNextLoop([this, host, callback]() {
                ResolveHostname(host, callback);
            } );
            return Error::OK;
        }

        HttpClient::Error HttpClient::PostFormAsync(const string& url, const vector< pair< string, string > >& formParams, HttpResponseCallBack onResponse, int timeoutSecond)
        {
            string host;
            string path;
            int port;
            Error error = parseUrl(url, host, path, &port);
            if (error != Error::OK) {
                onResponse(HttpStatusCode::Bad_Request, "parseUrl fail");
                return error;
            }

            string content;
            for (size_t i = 0u; i < formParams.size(); ++i) {
                const pair<string, string>& param = formParams[i];
                content.append(UrlBuilder::UrlEncode(param.first));
                content.append("=");
                content.append(UrlBuilder::UrlEncode(param.second));
                if (i < formParams.size() - 1u) {
                    content.append("&");
                }
            }

            auto callback = [this, host, path, port, content, onResponse, timeoutSecond](const DnsRecord& result) {
                string ip = result.getIP();
                if (ip.empty()) {
                    if (onResponse)
                    {
                        onResponse(HttpStatusCode::No_Content, "dns resolve fail");
                    }
                }
                else
                {
                    HttpConnection* conn = new HttpConnection(onResponse, m_impl);
                    conn->WriteRequestHead(HttpMethod::POST, host, path);
                    conn->WriteRequestBodyWithForm(content);
                    conn->Connect(ip.c_str(), port);
                    conn->SetTimeoutSeconds(timeoutSecond);
                }
            };

            base::Dispatcher::instance().ExecuteAtNextLoop([this, host, callback]() {
                ResolveHostname(host, callback);
            });
            return Error::OK;
        }

        HttpClient::Error HttpClient::PostJsonAsync(const string& url, const string& json, HttpResponseCallBack onResponse, int timeoutSecond)
        {
            string host;
            string path;
            int port;
            Error error = parseUrl(url, host, path, &port);
            if (error != Error::OK) {
                onResponse(HttpStatusCode::Bad_Request, "parseUrl fail");
                return error;
            }

            auto callback = [this, host, path, port, json, onResponse, timeoutSecond](const DnsRecord& result) {
                string ip = result.getIP();
                if (ip.empty()) {
                    if (onResponse)
                    {
                        onResponse(HttpStatusCode::No_Content, "dns resolve fail");
                    }
                }
                else
                {
                    HttpConnection* conn = new HttpConnection(onResponse, m_impl);
                    conn->WriteRequestHead(HttpMethod::POST, host, path);
                    conn->WriteRequestBodyWithJson(json);
                    conn->Connect(ip.c_str(), port);
                    conn->SetTimeoutSeconds(timeoutSecond);
                }
            };

            base::Dispatcher::instance().ExecuteAtNextLoop([this, host, callback]() {
                ResolveHostname(host, callback);
            });
            return Error::OK;
        }



    }
}
