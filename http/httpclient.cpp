#include "httpclient.h"

#include <stdio.h>
#include <iostream>

#include "http_parser.h"
#include "hostnameresolver.h"
#include "urlbuilder.h"
#include "utils_string.h"
#include "../net/client.h"
#include "../net/dispatcher.h"
#include "../net/BaseBuffer.h"

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
            HttpConnection(HttpClientEventHandler* handler)
                : handler_(handler) {
                //SAFE_RETAIN(handler_);
                http_parser_init(&parser_, HTTP_RESPONSE);
                parser_.data = this;
            }

            virtual ~HttpConnection() {
                //SAFE_RELEASE(handler_);
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
                        if (handler_) {
                            handler_->OnHttpResponse(HttpStatusCode::Not_Acceptable, HTTP_STATUS_STRING(HttpStatusCode::Not_Acceptable));
                        }
                        //Close();
                        //return;
                    }
                    recvBuffer.Removed(r);
                    recvBuffer.MoveData();
                }
            }

            void OnConnect() override {
                Flush();
            }

            void OnConnectFail(int eno, const char* reason) override {
                if (handler_ ) {
                    handler_->OnHttpResponse(HttpStatusCode::Request_Timeout, HTTP_STATUS_STRING(HttpStatusCode::Request_Timeout));
                }
            }

            void OnClose() override {
                if (handler_ ) {
                    handler_->OnHttpClose();
                }
                Release();
            }

            void Flush() {
                InvokeSend();
            }
        private:
            bool m_hasHead = false;
            bool m_hasBody = false;

            http_parser parser_;
            std::string resp_body_;
            HttpClientEventHandler* handler_;

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
                return 0;
            }

            static int on_header_value(http_parser* parser, const char* at, size_t len) {
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
                if (client->handler_) {
                    client->handler_->OnHttpResponse((HttpStatusCode)parser->status_code, client->resp_body_.c_str());
                }
                printf("%s\n", client->resp_body_.c_str());
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

        void HttpClient::ResolveHostname(const std::string& hostname)
        {
             //find from cache
            auto it = dns_cache_.find(hostname);
            if (it != dns_cache_.end()) {
                const DnsRecord& record = it->second;
                if (g_dispatcher->GetTimestampCache() - record.createTs < 30 * 60) {
                    return;
                }
            }

            http::HostnameResolver resolver(hostname);
            resolver.DoResolver();
            if (!resolver.result_.empty()) {
                dns_cache_[resolver.result_.hostname] = resolver.result_;
            }
            else {
                dns_cache_.erase(resolver.result_.hostname);
            }
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
            if ((urlParser.field_set && (1 << UF_QUERY)) != 0) {
                path.append("?");
                path.append(url.c_str() + urlParser.field_data[UF_QUERY].off, urlParser.field_data[UF_QUERY].len);
            }
            return HttpClient::Error::OK;
        }

        HttpClient::Error HttpClient::GetAsync(const string& url, const vector< pair< string, string > >& formParams, HttpClientEventHandler* evt_handler)
        {
            string host;
            string path;
            int port;
            Error error = parseUrl(url, host, path, &port);
            if (error != Error::OK) {
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
            path.append(content);

            ResolveHostname(host);
            auto it = dns_cache_.find(host);
            if (it != dns_cache_.end()) {
                DnsRecord& dns = it->second;
                string ip = dns.getIP();
                if (ip.empty()) {
                    if (evt_handler) {
                        evt_handler->OnHttpResponse(HttpStatusCode::No_Content, "dns resolve fail");
                    }
                }
                else {
                    cout << "req:" << host << ":" << port << path << endl;
                    HttpConnection* conn = new HttpConnection(evt_handler);
                    conn->WriteRequestHead(HttpMethod::GET, host, path);
                    conn->Connect(ip.c_str(), port);
                    //conn->AutoRelease();
                }
            }
            return Error::OK;
        }

        HttpClient::Error HttpClient::PostFormAsync(const string& url, const vector< pair< string, string > >& formParams, HttpClientEventHandler* evtHandler)
        {
            string host;
            string path;
            int port;
            Error error = parseUrl(url, host, path, &port);
            if (error != Error::OK) {
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

            ResolveHostname(host);
            auto it = dns_cache_.find(host);
            if (it != dns_cache_.end()) {
                DnsRecord& dns = it->second;
                string ip = dns.getIP();
                if (ip.empty()) {
                    if (evtHandler) {
                        evtHandler->OnHttpResponse(HttpStatusCode::No_Content, "dns resolve fail");
                    }
                }
                else {
                    cout << "req:" << host << ":" << port << path << endl;
                    HttpConnection* conn = new HttpConnection(evtHandler);
                    conn->WriteRequestHead(HttpMethod::POST, host, path);
                    conn->WriteRequestBodyWithForm(content);
                    conn->Connect(ip.c_str(), port);
                    //conn->AutoRelease();
                }
            }
            return Error::OK;
        }

        HttpClient::Error HttpClient::PostJsonAsync(const string& url, const string& json, HttpClientEventHandler* evtHandler)
        {
            string host;
            string path;
            int port;
            Error error = parseUrl(url, host, path, &port);
            if (error != Error::OK) {
                return error;
            }

            ResolveHostname(host);
            auto it = dns_cache_.find(host);
            if (it != dns_cache_.end()) {
                DnsRecord& dns = it->second;
                string ip = dns.getIP();
                if (ip.empty()) {
                    if (evtHandler) {
                        evtHandler->OnHttpResponse(HttpStatusCode::No_Content, "dns resolve fail");
                    }
                }
                else {
                    cout << "req:" << host << ":" << port << path << endl;
                    HttpConnection* conn = new HttpConnection(evtHandler);
                    conn->WriteRequestHead(HttpMethod::POST, host, path);
                    conn->WriteRequestBodyWithJson(json);
                    conn->Connect(ip.c_str(), port);
                    //conn->AutoRelease();
                }
            }
            return Error::OK;
        }
    }
}
