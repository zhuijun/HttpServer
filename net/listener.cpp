#include "listener.h"

#include "utils.h"
#include "client.h"

namespace base
{
    namespace net
    {
        using namespace std;

        Listener::~Listener()
        {
        }

        bool Listener::Bind(const char* ipaddr, int port)
        {
            sockaddr_in serveraddr;
            convert_sockaddr(&serveraddr, ipaddr, port);

            SOCKET listenfd = ::socket(AF_INET, SOCK_STREAM, 0);

            int reuseaddr = 1;
            int rc = 0;
            rc = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr, sizeof(int));
            //errno_assert(rc == 0);

            int sndbuf = 8192;
            rc = setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, (char *)&sndbuf, sizeof(int));
            //errno_assert(rc == 0);

            int rcvbuf = 8192;
            rc = setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuf, sizeof(int));
            //errno_assert(rc == 0);

            make_fd_nonblocking(listenfd);

            // bind
            rc = ::bind(listenfd, (sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
            if (rc != 0) {
                //LOG_ERROR("bind port fail, %s:%d\n", ipaddr, port);
                return false;
            }

            // listen
            rc = listen(listenfd, 5);
            //errno_assert(rc == 0);

            fd_ = listenfd;
            AddToDispatcher(listenfd, IO_READABLE);
            return true;
        }

        void Listener::OnEventIOReadable()
        {
            while (true)
            {
                sockaddr_in clientaddr;
                int clientaddrlen = sizeof(struct sockaddr_in);
                SOCKET clientfd = accept(fd(), (sockaddr*)&clientaddr, &clientaddrlen);
                if (clientfd == INVALID_SOCKET)
                {
                    int err = WSAGetLastError();
                    if (err == WSAEWOULDBLOCK)
                    {
                        break;
                    }
                    //LOG_ERROR("accept fail, errno:%d\n", errno);
                    break;
                }
                OnAccept(clientfd);
            }
        }

        void Listener::OnEventIOWriteable()
        {
            //release_assert("panic");
        }
    }
}

