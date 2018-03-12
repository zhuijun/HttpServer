#ifndef BASE_HTTP_JSONRESPONSE_H
#define BASE_HTTP_JSONRESPONSE_H

#include "response.h"

namespace base
{
    namespace http
    {
        class JsonResponse : public Response
        {
        public:
            JsonResponse() {
                AddHeader("Content-Type", "application/json; charset=utf-8");
            }

            virtual ~JsonResponse();

            //rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter() {
            //    return m_jsonWriter;
            //}

            virtual void Prepare() override;

        private:
            //rapidjson::StringBuffer m_stringBuffer;
            //rapidjson::Writer<rapidjson::StringBuffer> m_jsonWriter;
        };

    }
}

#endif // JSONRESPONSE_H
