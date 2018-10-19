#ifndef BASE_NET_CLIENT_H
#define BASE_NET_CLIENT_H

#include "eventio.h"
#include "Observer.h"
#include <string>
#include <deque>
#include <vector>
#include "BaseBuffer.h"

namespace base
{
    namespace net
    {
        class Client : public EventIO
        {
        public:
            Client()
                : connect_(false), connect_pending_(false), sendBuffer(sendq_, sizeof(sendq_)), 
                recvBuffer(recvq_, sizeof(recvq_)){}

            virtual ~Client();
            
            virtual const char* GetObjectName() {
                return "base::net::Client";
            }

            const std::string& ipaddr() const {
                return ipaddr_;
            }

            int port() const {
                return port_;
            }

            bool connect() const {
                return connect_;
            }

            void AcceptConnect(SOCKET clientfd);

            void Connect(const char* host, int port);

            void PushSend(const char* src, unsigned int len) {
                if (!connect()) {
                    return;
                }

                if (len > sendBuffer.FreeSize())
                {
                    sendBuffer.MoveData();
                }

                if (len > sendBuffer.FreeSize())
                {
                    //LOG
                    return;
                }

                sendBuffer.Add(src, len);
                InvokeSend();
            }

            virtual void Close();

        private:
            void ConnectDomain(const char* domain, int port);
            void ConnectIp(const char* ipaddr, int port);

            virtual void OnEventIOReadable();
            virtual void OnEventIOWriteable();
            virtual void OnEventIOClose();
            void OnConnectSuccess() {
                connect_ = true;
                OnConnect();
            }

            virtual void OnConnect() = 0;
            virtual void OnConnectFail(int eno, const char* reason) = 0;
            virtual void OnClose() = 0;
            virtual void OnReceive(std::size_t count) = 0;
            virtual void OnSendCompleted() {}
            void CheckIfConnectCompleted();

    protected:

            void InvokeSend();

            std::string ipaddr_;
            int port_;
            bool connect_;
            bool connect_pending_;

            char sendq_[1024];
            char recvq_[8192];
            BaseBuffer sendBuffer;
            BaseBuffer recvBuffer;
        };
    }
}

#endif // CLIENT_H
