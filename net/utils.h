#ifndef BASE_NET_UTILS_H
#define BASE_NET_UTILS_H

#include <stdint.h>
#include <string>

#include "../Header.h"

namespace base
{
    namespace net
    {
        // 解析ipaddr与port至sockaddr_in中
        void convert_sockaddr(sockaddr_in* addr, const char* ipaddr, int port);

        // 设置fd为非阻塞模式
        void make_fd_nonblocking(int fd);

        // 设置为NO_DELAY
        void make_socket_nodelay(int sock);

        // 获取可读ip地址字符串与端口
        void getpeername(int fd, std::string* ip, int* port);

        // 将二进制数据转为可读的字符串
        std::string dump_raw_data(const char* raw, uint32_t count);

        // 判断是否为IP
        bool is_ip_addr(const char* addr);
    }
}

#endif
