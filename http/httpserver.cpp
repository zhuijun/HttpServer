#include "httpserver.h"
#include "http_parser.h"
#include "response.h"
#include "jsonresponse.h"
#include "../base/utils_string.h"
#include "../net/client.h"
#include "../net/dispatcher.h"
#include "../net/listener.h"
#include "../base/Object.h"
#include "../base/Global.h"
#include "../base/TimerMgr.h"
#include <string.h>

namespace base
{
    namespace http
    {
        using namespace std;

        void Request::ParseQueryString(const char* str, uint16_t len)
        {
            string name;
            string val;
            bool isKey = true;

            for (uint16_t i = 0; i < len; ++i) {
                const char* p = str + i;
                char c = *p;
                switch (c) {
                    case '=':
                        isKey = false;
                        break;
                    case '&':
                        if (!name.empty() || !val.empty()) {
                            queries.emplace(move(name), move(val));
                        }
                        isKey = true;
                        break;
                    default:
                        if (isKey) {
                            name.push_back(c);
                        } else {
                            val.push_back(c);
                        }
                        break;
                }
            }
            if (!name.empty() || !val.empty()) {
                queries.emplace(move(name), move(val));
            }
        }

        int32_t Request::GetParamInt(const string& name, int32_t defaultValue) const
        {
            auto it = queries.find(name);
            return it == queries.end() ? defaultValue : utils::convert<int32_t>(it->second.c_str());
        }

        struct Handler {
            Handler(const std::function<bool(const Request&)>& f) : fun(f) {
            }
            ~Handler() {
            }
            std::function<bool(const Request&)> fun;
            DISABLE_COPY(Handler);
        };

        static struct http_parser_settings settings;

        class Connection : public net::Client
        {
        public:
            struct EventHandler {
                virtual void OnHttpRequest(Connection* connection, const Request& request) = 0;
                virtual void OnConnectionClose(Connection* connection) = 0;
            };

            Connection(int session) : m_session(session), m_request(*this) {
                http_parser_init(&m_parser, HTTP_REQUEST);
                m_parser.data = this;
                //cout << "http::Connection ctor " << session << endl;
            }

            virtual ~Connection() {
                //cout << "http::Connection ~dtor " << m_session << endl;
            }

            int session() const {
                return m_session;
            }

            int64_t lastActiveTs() const {
                return m_lastActiveTs;
            }

            void SetEventHandler(EventHandler* eventHandler) {
                m_eventHandler = eventHandler;
            }

            void SendResponse(const Response& response) {
                Response& response1 = const_cast<Response&>(response);
                response1.Prepare();

                response1.FlushHead(sendBuffer);
                do {
                    response1.FlushBody(sendBuffer);
                    InvokeSend();
                } while (!response1.finish());
            }

        public:
            static int on_message_begin_cb(http_parser* parser) {
                Connection* conn = static_cast<Connection*>(parser->data);
                conn->m_request.body.reserve(1024);
                return 0;
            }

            static int on_url(http_parser* parser, const char* at, size_t len) {
                Connection* conn = static_cast<Connection*>(parser->data);
                conn->m_url.append(at, len);
                return 0;
            }

            static int on_header_field(http_parser* parser, const char* at, size_t len) {
                Connection* conn = static_cast<Connection*>(parser->data);
                conn->ConsumeHeader();
                conn->m_temp1.append(at, len);
                return 0;
            }

            static int on_header_value(http_parser* parser, const char* at, size_t len) {
                Connection* conn = static_cast<Connection*>(parser->data);
                conn->m_temp2.append(at, len);
                return 0;
            }

            static int on_headers_complete(http_parser* parser) {
                Connection* conn = static_cast<Connection*>(parser->data);
                conn->ConsumeHeader();
                conn->m_request.method = static_cast<HttpMethod>(parser->method);
                return 0;
            }

            static int on_body_cb(http_parser* parser, const char* at, size_t len) {
                Connection* conn = static_cast<Connection*>(parser->data);
                conn->m_request.body.append(at, len);
                if (conn->m_request.body.size() > 1024 * 20) {
                    //LOG_ERROR("request body size exceed limit");
                    conn->Close();
                    return 0;
                }
                return 0;
            }

            static int on_message_complete_cb(http_parser* parser) {
                Connection* conn = static_cast<Connection*>(parser->data);
                conn->ParserUrl();
                conn->OnHttpRequestComplete();
                //conn->Close();
                return 0;
            }

        private:
            virtual void OnClose() override {
                if (m_eventHandler) {
                    m_eventHandler->OnConnectionClose(this);
                }
            }

            virtual void OnConnect() override {
                m_lastActiveTs = g_dispatcher->GetTimestampCache();
            }

            virtual void OnConnectFail(int eno, const char* reason) override {
            }

            virtual void OnReceive(std::size_t count) override {
                if (count > 0)
                {
                    size_t r = http_parser_execute(&m_parser, &settings, recvBuffer.Data(), recvBuffer.DataSize());
                    if (r != recvBuffer.DataSize()) {
                        //LOG_ERROR("bad http request");
                        Close();
                        return;
                    }
                    recvBuffer.Removed(r);
                    recvBuffer.MoveData();
                }
            }

            virtual void OnSendCompleted() override {
                Close();
            }

            void OnHttpRequestComplete() {
                m_lastActiveTs = g_dispatcher->GetTimestampCache();
                m_request.session = m_session;
                if (m_eventHandler) {
                    m_eventHandler->OnHttpRequest(this, m_request);
                }
                // keep-alive
                m_request.Reset();
                http_parser_init(&m_parser, HTTP_REQUEST);
                m_url.clear();
                m_temp1.clear();
                m_temp2.clear();
            }

            void ConsumeHeader() {
                if (!m_temp1.empty() && !m_temp2.empty()) {
                    m_request.headers.emplace(move(m_temp1), move(m_temp2));
                }
            }

            void ParserUrl() {
                http_parser_url urlParser;
                http_parser_url_init(&urlParser);
                const char* str = m_url.c_str();
                if (http_parser_parse_url(str, m_url.length(), 0, &urlParser) == 0) {
                    m_request.path.append(str + urlParser.field_data[UF_PATH].off, urlParser.field_data[UF_PATH].len);
                    m_request.ParseQueryString(str + urlParser.field_data[UF_QUERY].off, urlParser.field_data[UF_QUERY].len);
                } else {
                    //LOG_ERROR("http parse url fail: %s", m_url.c_str());
                    Close();
                }
            }

            int64_t m_lastActiveTs = 0;
            int m_session;
            std::string m_url;
            std::string m_temp1;
            std::string m_temp2;

            Request m_request;

            http_parser m_parser;
            EventHandler* m_eventHandler = nullptr;
        };

        static struct http_parser_settings_initializer2 {
            http_parser_settings_initializer2() {
                memset(&settings, 0, sizeof(settings));
                settings.on_url = Connection::on_url;
                settings.on_header_field = Connection::on_header_field;
                settings.on_header_value = Connection::on_header_value;
                settings.on_headers_complete = Connection::on_headers_complete;
                settings.on_body = Connection::on_body_cb;
                settings.on_message_begin = Connection::on_message_begin_cb;
                settings.on_message_complete = Connection::on_message_complete_cb;
            }
        } _settings_initializer2;

        void Request::SendResponse(Response& response) const
        {
            m_connection.SendResponse(response);
        }

        class HttpServerImpl : public net::Listener::EventHandler, public Connection::EventHandler
        {
        public:
            HttpServerImpl(HttpServer& server) : m_server(server) {}
            virtual ~HttpServerImpl() {
                for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
                    it->second->SetEventHandler(nullptr);
                    it->second->Release();
                }
                m_connections.clear();
                SAFE_RELEASE(m_listener);
                FOREACH_DELETE_MAP(m_handlers);
                SAFE_DELETE(m_defaultHandler);
            }

            void Start(const char* ip, int port) {
                m_listener = new net::Listener(*this);
                bool ok = m_listener->Bind(ip, port);
                if (!ok) {
                    //LOG_ERROR("bind ip=%s, port=%d", ip, port);
                    assert(0);
                }
                m_aliveTimer = UnsafeTimerInst.AddStdFunctionTimeRepeat(0, bind(&HttpServerImpl::CheckConnectionIsAlive, this), 30);
            }

            void BeginCleanup(std::function<void()> callback) {
                m_listener->Close();

                for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
                    Connection* conn = it->second;
                    conn->Close();
                }
                UnsafeTimerInst.RemoveFunction(m_aliveTimer);
                m_cleanupTimer = UnsafeTimerInst.AddStdFunctionTimeRepeat(0, [this, callback]() {
                    if (m_connections.empty()) {
                        UnsafeTimerInst.RemoveFunction(m_cleanupTimer);
                        callback();
                    }
                }, 1);
            }

            void SendResponse(int session, const Response& response) {
                auto it = m_connections.find(session);
                if (it != m_connections.end()) {
                    Connection* conn = it->second;
                    conn->SendResponse(response);
                }
            }

            void Close(int session) {
                auto it = m_connections.find(session);
                if (it != m_connections.end()) {
                    Connection* conn = it->second;
                    conn->Close();
                }
            }

            class Listener;
            virtual void OnListenerAccept(net::Listener* sender, SOCKET clientfd) override {
                Connection* conn = new Connection(++m_sessionNo);
                conn->SetEventHandler(this);
                conn->AcceptConnect(clientfd);
                m_connections.emplace(conn->session(), conn);
            }

            virtual void OnConnectionClose(Connection* connection) override {
                auto it = m_connections.find(connection->session());
                if (it != m_connections.end()) {
                    m_connections.erase(it);
                    connection->Release();
                }
            }

            virtual void OnHttpRequest(Connection* connection, const Request& request) override {
                auto it = m_handlers.find(request.path);
                if (it == m_handlers.end()) {
                    bool found = false;
                    if (m_defaultHandler) {
                        found = true;
                        if (!m_defaultHandler->fun(request)) {
                            Html500Response response;
                            connection->SendResponse(response);
                        }
                    }
                    if (!found && !m_resourceDir.empty()) {
                        string file = m_resourceDir;
                        if (request.path == "/") {
                            file += "/index.html";
                        } else {
                            file += request.path;
                        }
                        //if (file.find("..") == string::npos && utils::file_is_exist(file.c_str())) {
                        //    found = true;
                        //    FileResponse response(file);
                        //    connection->SendResponse(response);
                        //}
                    }
                    if (!found) {
                        cout << "unhandled http request:" << request.path << endl;
                        Html404Response response;
                        connection->SendResponse(response);
                    }
                } else {
                    Handler* hd = it->second;
                    if (hd) {
                        if (!hd->fun(request)) {
                            Html500Response response;
                            connection->SendResponse(response);
                        }
                    }
                }
            }

            void SetResourcesDir(const char* dir) {
                m_resourceDir = dir;
            }

        private:
            void CheckConnectionIsAlive() {
                int64_t now = g_dispatcher->GetTimestampCache();
                for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
                    Connection* conn = it->second;
                    if (now - conn->lastActiveTs() > 30) {
                        conn->Close();
                    }
                }
            }

            std::string m_resourceDir;
            int m_sessionNo = 0;

            HttpServer& m_server;
            net::Listener* m_listener = nullptr;
            std::map<int, Connection*> m_connections;
            std::map<string, Handler*> m_handlers;
            Handler* m_defaultHandler = nullptr;
            uint32_t m_aliveTimer = 0;
            uint32_t m_cleanupTimer = 0;
            friend class HttpServer;
        };

        static HttpServer* s_instance = nullptr;

        HttpServer* HttpServer::Create()
        {
            if (s_instance == nullptr) {
                s_instance = new HttpServer();
            }
            return s_instance;
        }

        void HttpServer::Destroy()
        {
            SAFE_DELETE(s_instance);
        }

        HttpServer* HttpServer::instance()
        {
            return s_instance;
        }

        /// HttpServer
        HttpServer::HttpServer()
        {
            m_impl = new HttpServerImpl(*this);
        }

        HttpServer::~HttpServer()
        {
            delete m_impl;
        }

        void HttpServer::Start(const char* ip, int port)
        {
            m_impl->Start(ip, port);
        }

        void HttpServer::BeginCleanup(function< void() > cb)
        {
            m_impl->BeginCleanup(cb);
        }

        void HttpServer::SetResourcesDir(const char* dir)
        {
            m_impl->SetResourcesDir(dir);
        }

        void HttpServer::AddHandler(const std::string& path, std::function< bool(const Request&) > handler)
        {
            auto it = m_impl->m_handlers.find(path);
            if (it != m_impl->m_handlers.end()) {
                return;
            }
            Handler* h = new Handler(handler);
            m_impl->m_handlers.emplace(path, h);
        }

        void HttpServer::SetDefaultHandler(function< bool(const Request&) > handler)
        {
            SAFE_DELETE(m_impl->m_defaultHandler);
            m_impl->m_defaultHandler = new Handler(handler);
        }

        void HttpServer::SendResponse(int session, const Response& response)
        {
            m_impl->SendResponse(session, response);
        }

        void HttpServer::Close(int session)
        {
            m_impl->Close(session);
        }

    }
}
