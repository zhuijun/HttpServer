#include "response.h"
#include "utils_string.h"
#include <string.h>

namespace base
{
    namespace http
    {
        using namespace std;

        Response::~Response()
        {
        }

        void Response::FlushHead(BaseBuffer& pktout)
        {
            string tmp;

            pktout.Empty();

            tmp.clear();
            base::utils::string_append_format(tmp, "HTTP/1.1 %d %s\r\n", (int)m_statusCode, HTTP_STATUS_STRING(m_statusCode));
            pktout.Add(tmp.c_str(), tmp.length());

            for (const pair<string, string>& header : m_headers) {
                pktout.Add(header.first.c_str(), header.first.length());
                pktout.Add(": ", 2);
                pktout.Add(header.second.c_str(), header.second.length());
                pktout.Add("\r\n", 2);
            }

            //tmp.clear();
            //pktout.Add("Connection: keep-alive\r\n", strlen("Connection: keep-alive\r\n"));

            tmp.clear();
            uint32_t len = m_content.size();
            base::utils::string_append_format(tmp, "Content-Length: %u\r\n", len);
            pktout.Add(tmp.c_str(), tmp.length());

            tmp.clear();
            pktout.Add("Access-Control-Allow-Origin: *\r\n", strlen("Access-Control-Allow-Origin: *\r\n"));

            pktout.Add("\r\n", 2);
        }

        void Response::FlushBody(BaseBuffer& pktout)
        {
            size_t left = m_content.size() - m_contentPos;
            if (left > 0) {
                size_t eat = left > pktout.FreeSize() ? pktout.FreeSize() : left;
                pktout.Add(m_content.c_str() + m_contentPos, eat);
                m_contentPos += eat;
            }
            m_finish = m_contentPos >= m_content.size();
        }

        void FileResponse::Prepare()
        {
            //const char* name = basename(m_file.c_str());
            //m_content = utils::file_get_content(m_file.c_str());
        }

        Html404Response::Html404Response()
        {
            SetStatusCode(HttpStatusCode::Not_Found);
            static const char* defaultPage = "<html><head></head><body><h2>404 Not Found</h2></body></html>";
            AppendHtml(defaultPage);
        }

        Html500Response::Html500Response()
        {
            SetStatusCode(HttpStatusCode::Internal_Server_Error);
            static const char* defaultPage = "<html><head></head><body><h2>500 Internal Server Error</h2></body></html>";
            AppendHtml(defaultPage);
        }

        Html503Response::Html503Response()
        {
            SetStatusCode(HttpStatusCode::Service_Unavailable);
            static const char* defaultPage = "<html><head></head><body><h2>503 Service Unavailable</h2></body></html>";
            AppendHtml(defaultPage);
        }
    }
}
