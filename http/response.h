#ifndef BASE_HTTP_RESPONSE_H_
#define BASE_HTTP_RESPONSE_H_

#include "../base/Object.h"
#include "constant.h"
#include <vector>
#include "../net/BaseBuffer.h"

namespace base
{
    namespace gateway
    {
        class PacketOutBase;
    }

    namespace http
    {
        class Response
        {
        public:
            Response(){};
            virtual ~Response();

            bool finish() const {
                return m_finish;
            }

            // 设置状态码
            void SetStatusCode(HttpStatusCode statusCode) {
                m_statusCode = statusCode;
            }

            // 添加header
            void AddHeader(const std::string& name, const std::string& value) {
                m_headers.emplace_back(name, value);
            }

            virtual void Prepare() {};
            virtual void FlushHead(base::net::BaseBuffer& pktout);
            virtual void FlushBody(base::net::BaseBuffer& pktout);

            void SetContent(const std::string& content) {
                m_content = content;
            }

            void SetContent(std::string && content) {
                m_content = move(content);
            }

        protected:
            std::string m_content;
            std::size_t m_contentPos = 0u;
            bool m_finish = false;

        private:
            HttpStatusCode m_statusCode = HttpStatusCode::OK;
            std::vector<std::pair<std::string, std::string>> m_headers;
        };

        class FileResponse : public Response
        {
        public:
            FileResponse(const std::string& file) : m_file(file) {
            }

            FileResponse(std::string && file) : m_file(std::move(file)) {
            }

            virtual void Prepare() override;

        private:
            std::string m_file;
        };

        class HtmlResponse : public Response
        {
        public:
            HtmlResponse() {
                AddHeader("Content-Type", "text/html; charset=utf-8");
            }

            void AppendHtml(const char* content) {
                m_content.append(content);
            }

            void AppendHtml(const std::string& content) {
                m_content = content;
            }

            void AppendHtml(std::string && content) {
                m_content = std::move(content);
            }
        };

        class Html404Response : public HtmlResponse
        {
        public:
            Html404Response();
        };

        class Html500Response : public HtmlResponse
        {
        public:
            Html500Response();
        };

        class Html503Response : public HtmlResponse
        {
        public:
            Html503Response();
        };
    }
}

#endif // RESPONSE_H
