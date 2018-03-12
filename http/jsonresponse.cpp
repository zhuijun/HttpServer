#include "jsonresponse.h"

namespace base
{
    namespace http
    {
        using namespace std;

        JsonResponse::~JsonResponse()
        {
        }

        void JsonResponse::Prepare()
        {
            //const char* str = m_stringBuffer.GetString();
            //size_t size = m_stringBuffer.GetSize();
            //m_content.assign(str, size);
        }
    }
}
