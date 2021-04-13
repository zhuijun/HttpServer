#include "utils_string.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ObjBase.h>

namespace base
{
    namespace utils
    {
        using namespace std;

        void string_path_append(string& path1, const string& path2)
        {
            if (!path1.empty() && !path2.empty())
            {
                if (path1.back() == '/' && path2.front() == '/')
                {
                    path1.pop_back();
                }
            }
            if (!path1.empty() && path1.back() != '/' && !path2.empty() && path2.front() != '/')
            {
                path1.push_back('/');
            }
            path1.append(path2);
        }

        string string_join(const vector< string >& str_arr, char sep)
        {
            string ret;
            if (!str_arr.empty())
            {
                for (const string & s : str_arr)
                {
                    ret.append(s);
                    ret.push_back(sep);
                }
            }
            ret.pop_back();
            return ret;
        }

        string string_join(const vector< int32_t >& int_arr, char sep)
        {
            string ret;
            if (!int_arr.empty())
            {
                for (int32_t i : int_arr)
                {
                    ret.append(std::to_string(i));
                    ret.push_back(sep);
                }
                ret.pop_back();
            }
            return ret;
        }

        string string_lr_trim(const char* str)
        {
            string ret;

            const char* begin = str;
            while (*begin)
            {
                if (isblank(*begin))
                {
                    ++begin;
                }
                else
                {
                    break;
                }
            }

            const char* end = str;
            const char* p = str;
            while (*p)
            {
                if (!isblank(*p))
                {
                    end = p;
                }
                ++p;
            }

            for (p = begin; p <= end; ++p)
            {
                ret.push_back(*p);
            }
            return ret;
        }

        std::vector<string> string_split(const string& str, char separator, bool trim_empty)
        {
            std::vector<string> list;
            for (size_t pos = 0; pos < str.length(); ++pos)
            {
                size_t tag = str.find(separator, pos);
                string temp;
                if (tag != string::npos)
                {
                    temp = str.substr(pos, tag - pos);
                    pos = tag;
                }
                else
                {
                    temp = str.substr(pos);
                }
                if (!trim_empty || !temp.empty())
                {
                    list.emplace_back(std::move(temp));
                }
                if (tag == string::npos) break;
            }
            return list;
        }

        vector< int32_t > string_split_int(const char* str, char sep)
        {
            vector<int32_t> ret;
            string tmp;
            const char* p = str;
            while (*p)
            {
                if (*p == sep)
                {
                    if (!tmp.empty())
                    {
                        ret.push_back(atoi(tmp.c_str()));
                    }
                    tmp.clear();
                }
                else
                {
                    tmp.push_back(*p);
                }
                ++p;
            }
            if (!tmp.empty())
            {
                ret.push_back(atoi(tmp.c_str()));
            }
            return ret;
        }

        void string_append_format(std::string& str, const char* format, ...)
        {
            static char tmp[2048];
            va_list va;
            va_start(va, format);
            vsprintf(tmp, format, va);
            va_end(va);
            str.append(tmp);
        }

        string string_dump_binary(const string& src)
        {
            string ret;
            for (uint32_t i = 0; i < src.length(); ++i)
            {
                ret.append(base::utils::to_string((uint32_t)(uint8_t)src[i]));
                ret.append(",");
            }
            return ret;
        }

        size_t utf8strlen(const string& data)
        {
            size_t result = 0;
            mbstate_t s;
            memset(&s, 0, sizeof(s));
            //setlocale(LC_ALL, "en_US.utf8");
            const char* begin = data.data();
            const char* end = data.data() + data.length();
            size_t r = mbrlen(begin, end - begin, &s);
            while (r > 0)
            {
                begin += r;
                ++result;
                r = mbrlen(begin, end - begin, &s);
            }
            return result;
        }

        static char __convert_buff[256];

        //std::string convert(float data)
        //{
        //    snprintf(__convert_buff, 256, "%g", data);
        //    return __convert_buff;
        //}

        //std::string convert(double data)
        //{
        //    snprintf(__convert_buff, 256, "%g", data);
        //    return __convert_buff;
        //}

        string urldecode(const char* cd, size_t len)
        {
            char p[2];
            string decd;
            for (size_t i = 0; i < len; i++)
            {
                memset(p, 0, 2);
                if (cd[i] != '%')
                {
                    decd.push_back(cd[i]);
                    continue;
                }

                p[0] = cd[++i];
                p[1] = cd[++i];

                p[0] = p[0] - 48 - ((p[0] >= 'A') ? 7 : 0) - ((p[0] >= 'a') ? 32 : 0);
                p[1] = p[1] - 48 - ((p[1] >= 'A') ? 7 : 0) - ((p[1] >= 'a') ? 32 : 0);
                decd.push_back(p[0] * 16 + p[1]);
            }
            return decd;
        }

        static inline unsigned char toHex(unsigned char x)
        {
            return  x > 9 ? x + 55 : x + 48;
        }

        string urlencode(const string& value)
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

        std::string GenerateGuid()
        {
            GUID guid;
            CoCreateGuid(&guid);
            char cBuffer[64] = { 0 };
            sprintf_s(cBuffer, sizeof(cBuffer),
                "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                guid.Data1, guid.Data2,
                guid.Data3, guid.Data4[0],
                guid.Data4[1], guid.Data4[2],
                guid.Data4[3], guid.Data4[4],
                guid.Data4[5], guid.Data4[6],
                guid.Data4[7]);
            return std::string(cBuffer);
        }
    }
}
