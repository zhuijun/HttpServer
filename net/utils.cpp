#include "utils.h"
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <Ws2tcpip.h>
#include "utils_string.h"

namespace base
{
    namespace net
    {
        using namespace std;

        void convert_sockaddr(sockaddr_in* addr, const char* ipaddr, int port)
        {
            memset(addr, 0, sizeof(struct sockaddr_in));
            addr->sin_family = AF_INET;
            if (strcmp(ipaddr, "*") == 0 || strcmp(ipaddr, "any") == 0) {
                addr->sin_addr.s_addr = INADDR_ANY;
            } else {
                inet_pton(AF_INET, ipaddr, &(addr->sin_addr));
            }
            addr->sin_port = htons(port);
        }

        void make_fd_nonblocking(int fd)
        {
            u_long iMode = 1;
            int iResult = ioctlsocket(fd, FIONBIO, &iMode);
            if (iResult != NO_ERROR)
                printf("ioctlsocket failed with error: %ld\n", iResult);
        }

        void make_socket_nodelay(int sock)
        {
            int iOptVal = 0;
            int iOptLen = sizeof(int);
            int rc = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&iOptVal, iOptLen);
            //errno_assert(rc != -1);
        }

        void getpeername(int fd, std::string* ip, int* port)
        {
            sockaddr_in addr;
            int len = sizeof(addr);
            ::getpeername(fd, (sockaddr*)&addr, &len);

            *ip = inet_ntoa(addr.sin_addr);
            *port = addr.sin_port;
        }

        string dump_raw_data(const char* raw, uint32_t count)
        {
            std::string ret;
            for (uint32_t i = 0; i < count; ++i) {
                utils::string_append_format(ret, "%u,", (uint32_t)((uint8_t)(*(raw + i))));
            }
            return ret;
        }

        bool is_ip_addr(const char* addr)
        {
            vector<int> arr = utils::string_split_int(addr, '.');
            if (arr.size() == 4u) {
                if (arr[0] > 0 || arr[1] > 0 || arr[2] > 0 || arr[3] > 0) {
                    return true;
                }
            }
            return false;
        }
    }
}
