#include "urlbuilder.h"

namespace base
{
    namespace http
    {
        using namespace std;

        static inline unsigned char toHex(unsigned char x)
        {
            return  x > 9 ? x + 55 : x + 48;
        }

        string UrlBuilder::UrlEncode(const string& value)
        {
            std::string strTemp = "";
            size_t length = value.length();
            for (size_t i = 0; i < length; i++) {
                if (isalnum((unsigned char)value[i]) ||
                        (value[i] == '-') ||
                        (value[i] == '_') ||
                        (value[i] == '.') ||
                        (value[i] == '~'))
                    strTemp += value[i];
                else if (value[i] == ' ')
                    strTemp += "+";
                else {
                    strTemp += '%';
                    strTemp += toHex((unsigned char)value[i] >> 4);
                    strTemp += toHex((unsigned char)value[i] % 16);
                }
            }
            return strTemp;
        }

    }
}
