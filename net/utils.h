#ifndef BASE_NET_UTILS_H
#define BASE_NET_UTILS_H

#include <stdint.h>
#include <string>

#include "../Header.h"

namespace base
{
    namespace net
    {
        // ����ipaddr��port��sockaddr_in��
        void convert_sockaddr(sockaddr_in* addr, const char* ipaddr, int port);

        // ����fdΪ������ģʽ
        void make_fd_nonblocking(int fd);

        // ����ΪNO_DELAY
        void make_socket_nodelay(int sock);

        // ��ȡ�ɶ�ip��ַ�ַ�����˿�
        void getpeername(int fd, std::string* ip, int* port);

        // ������������תΪ�ɶ����ַ���
        std::string dump_raw_data(const char* raw, uint32_t count);

        // �ж��Ƿ�ΪIP
        bool is_ip_addr(const char* addr);
    }
}

#endif
