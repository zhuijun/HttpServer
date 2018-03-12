#ifndef BASE_HTTP_URLBUILDER_H
#define BASE_HTTP_URLBUILDER_H

#include <string>

namespace base
{
    namespace http
    {
        class UrlBuilder
        {
        public:
            // 编码URL
            static std::string UrlEncode(const std::string& value);

        public:
            UrlBuilder(const std::string& path) : m_path(path) {}

            void appendQueryParam(const std::string& name, const std::string& value) {
                if (!m_queryString.empty()) {
                    m_queryString.append("&");
                }
                m_queryString.append(UrlEncode(name));
                m_queryString.append("=");
                m_queryString.append(UrlEncode(value));
            }

            std::string getResult() {
                if (m_queryString.empty()) {
                    return m_path;
                } else {
                    return m_path + "?" + m_queryString;
                }
            }

        private:
            std::string m_path;
            std::string m_queryString;
        };
    }
}

#endif // URLBUILDER_H
