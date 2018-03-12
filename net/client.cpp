#include "client.h"
#include "utils.h"
#include "utils_string.h"
#include "../http/hostnameresolver.h"
#include <cerrno>
#include <stdint.h>

namespace base
{
    namespace net
    {
        using namespace std;

        Client::~Client()
        {
        }

        void Client::AcceptConnect(SOCKET clientfd)
        {
            getpeername(clientfd, &ipaddr_, &port_);
            make_fd_nonblocking(clientfd);
            make_socket_nodelay(clientfd);
            fd_ = clientfd;
            AddToDispatcher(clientfd, IO_READABLE);
            OnConnectSuccess();
        }

        void Client::ConnectDomain(const char* domain, int port)
        {
            http::HostnameResolver resolver(domain);
            resolver.DoResolver();
            string ip = resolver.result_.getIP();
            if (ip.empty()) {
                OnConnectFail(-1, "can not resolve domain");
            } else {
                ConnectIp(ip.c_str(), port);
            }
        }

        void Client::Connect(const char* host, int port)
        {
            if (is_ip_addr(host)) {
                ConnectIp(host, port);
            } else {
                ConnectDomain(host, port);
            }
        }

        void Client::ConnectIp(const char* ipaddr, int port)
        {
            ipaddr_ = ipaddr;
            port_ = port;
            SOCKET clientfd = ::socket(AF_INET, SOCK_STREAM, 0);
            //errno_assert(clientfd != -1);
            sockaddr_in addr;
            convert_sockaddr(&addr, ipaddr, port);
            make_fd_nonblocking(clientfd);
            make_socket_nodelay(clientfd);
            fd_ = clientfd;
            AddToDispatcher(clientfd, IO_WRITEABLE);
            int rc = ::connect(clientfd, (sockaddr*)&addr, sizeof(struct sockaddr_in));
            if (rc == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK) {
                    OnConnectFail(err, strerror(errno));
                    Close();
                } else {
                    connect_pending_ = true;
                }
            } else {
                OnConnectSuccess();
            }
        }

        void Client::OnEventIOReadable()
        {
            while (connect() && !closed()) {
                int n = ::recv(fd(), recvBuffer.Tail(), recvBuffer.FreeSize(), 0);
                if (n > 0)
                {
                    recvBuffer.Added(n);
                    bool isfull = recvBuffer.FreeSize() == 0;
                    OnReceive(n);
                    if (!isfull)
                    {
                        break;
                    }
                }
                else if (n == 0)
                {
                    Close();
                    break;
                }
                else
                {
                    int err = WSAGetLastError();
                    if (err != WSAEWOULDBLOCK)
                    {
                        Close();
                    }
                    break;
                }
            }
        }

        void Client::OnEventIOWriteable()
        {
            if (connect_pending_) {
                CheckIfConnectCompleted();
            }
            if (connect_) {
                ModifyIOEvent(IO_READABLE);
                InvokeSend();
            }
        }

        void Client::OnEventIOClose()
        {
            if (connect_) {
                connect_ = false;
                OnClose();
            }
        }

        void Client::CheckIfConnectCompleted()
        {
            connect_pending_ = false;
            int err = 0;
            int errlen = sizeof(int);
            if (getsockopt(fd(), SOL_SOCKET, SO_ERROR, (char *)&err, &errlen) == -1) {
                OnConnectFail(err, strerror(err));
                Close();
            } else {
                if (err) {
                    OnConnectFail(err, strerror(err));
                    Close();
                } else {
                    OnConnectSuccess();
                }
            }
        }

        void Client::InvokeSend()
        {
            if (sendBuffer.IsEmpty())
            {
                return;
            }

            bool sendevt = false;

            int n = ::send(fd(), sendBuffer.Data(), sendBuffer.DataSize(), 0);
            if (n > 0)
            {
                sendevt = true;
                sendBuffer.Removed(n);
            }
            else
            {
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK) {
                    Close();
                }
                else
                {
                    ModifyIOEvent(ioevt_ | IO_WRITEABLE);
                }
            }

            if (sendevt && sendBuffer.IsEmpty()) {
                OnSendCompleted();
            }
        }

        void Client::Close()
        {
            if (fd() != -1) {
                shutdown(fd(), SD_BOTH);
            }
            base::EventIO::Close();
        }
    }
}
