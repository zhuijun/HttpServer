#ifndef BASE_UTILS_STRING_UTILS_H
#define BASE_UTILS_STRING_UTILS_H

#include "Global.h"
#include <string>
#include <vector>
#include <stdint.h>

namespace base
{
    namespace utils
    {
        using namespace std;

        // 拼接文件路径
        void string_path_append(std::string& path1, const std::string& path2);

        // 拼接字符串
        std::string string_join(const std::vector<std::string>& str_arr, char sep);

        // 拼接字符串
        std::string string_join(const std::vector<int32_t>& int_arr, char sep);

        // 剔除首尾空白字符
        std::string string_lr_trim(const char* str);

        // 切割字符串
        std::vector<string> string_split(const string& str, char separator, bool trim_empty = true);

        // 切割字符串，并转换为integer
        std::vector<int32_t> string_split_int(const char* str, char sep);
        inline std::vector<int32_t> string_split_int(const std::string& str, char sep)
        {
            return string_split_int(str.c_str(), sep);
        }

        // 计算字符串哈希值
        inline uint32_t string_hash(const char* str)
        {
            uint32_t seed = 13131; // 31 131 1313 13131 131313 etc..
            uint32_t hash = 0;
            while (*str)
            {
                hash = hash * seed + (*str++);
            }
            return (hash & 0x7FFFFFFF);
        }

        // 附加格式化字符串 (非线程安全)
        void string_append_format(std::string& str, const char* format, ...);

        // 将二进制数据转为可读格式
        std::string string_dump_binary(const std::string& src);

        // utf8 格式的字符串长度
        size_t utf8strlen(const std::string& data);

        // 字符串与数字之间转换
        template<typename T>
        inline typename std::enable_if<std::is_integral<typename std::remove_reference<T>::type>::value, std::string>::type
            convert(const T& data)
        {
                return std::to_string(data);
            }
        //std::string convert(float data);
        //std::string convert(double data);

        template<typename T>
        T convert(const char* str);

        template<>
        inline unsigned short convert<unsigned short>(const char* str)
        {
            return (unsigned short)strtol(str, nullptr, 10);
        }

        template<>
        inline short convert<short>(const char* str)
        {
            return (short)strtol(str, nullptr, 10);
        }

        template<>
        inline unsigned int convert<unsigned int>(const char* str)
        {
            return (unsigned int)strtoul(str, nullptr, 10);
        }

        template<>
        inline int convert<int>(const char* str)
        {
            return (int)strtol(str, nullptr, 10);
        }

        template<>
        inline unsigned long convert<unsigned long>(const char* str)
        {
            return strtoul(str, nullptr, 10);
        }

        template<>
        inline long convert<long>(const char* str)
        {
            return strtol(str, nullptr, 10);
        }

        template<>
        inline unsigned long long convert<unsigned long long>(const char* str)
        {
            return strtoull(str, nullptr, 10);
        }

        template<>
        inline long long convert<long long>(const char* str)
        {
            return strtoll(str, nullptr, 10);
        }

        template<>
        inline float convert<float>(const char* str)
        {
            return strtof(str, nullptr);
        }

        template<>
        inline double convert<double>(const char* str)
        {
            return strtod(str, nullptr);
        }

        string urldecode(const char* cd, size_t len);
        string urlencode(const string& value);

        std::string GenerateGuid();
    }
}

#endif
