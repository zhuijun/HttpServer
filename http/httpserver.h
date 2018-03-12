#ifndef BASE_HTTP_HTTPSERVER_H
#define BASE_HTTP_HTTPSERVER_H

#include <string>
#include <functional>
#include <unordered_map>
#include "observer.h"
#include "constant.h"

namespace base
{
    namespace http
    {
        class Response;
        class Connection;

        class Request
        {
        public:
            Request(Connection& connection) : m_connection(connection) {}

            int session;

            std::string path;

            HttpMethod method;

            std::unordered_map<std::string, std::string> queries;

            std::unordered_map<std::string, std::string> headers;

            std::string body;

            const std::string& GetParam(const std::string& name) const {
                auto it = queries.find(name);
                return it == queries.end() ? EMPTY : it->second;
            }
            
            int32_t GetParamInt(const std::string& name, int32_t defaultValue = 0) const;

            void ParseQueryString(const char* str, uint16_t len);

            void Reset() {
                path.clear();
                queries.clear();
                headers.clear();
                body.clear();
            }

            void SendResponse(Response& response) const;

        private:
            Connection& m_connection;
            std::string EMPTY;
        };

        class HttpServerImpl;

        class HttpServer
        {
        public:
            static HttpServer* Create();
            static void Destroy();
            static HttpServer* instance();

            void Start(const char* ip, int port);

            void BeginCleanup(std::function<void()> cb);

            void SetResourcesDir(const char* dir);

            void AddHandler(const std::string& path, std::function<bool(const Request&)> handler);

            void SetDefaultHandler(std::function<bool(const Request&)> handler);

            void SendResponse(int session, const Response& response);

        private:
            HttpServer();
            virtual ~HttpServer();

        private:
            HttpServerImpl* m_impl;
        };
    }
}

#endif // HTTPSERVER_H
