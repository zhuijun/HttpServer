#ifndef BASE_NET_LISTENER_H
#define BASE_NET_LISTENER_H

#include "Global.h"
#include "eventio.h"

namespace base
{
    namespace net
    {
        class Client;
        class Listener;

        class Listener : public EventIO
        {
        public:
            struct EventHandler {
                virtual ~EventHandler() {}
                virtual void OnListenerAccept(Listener* sender, SOCKET clientfd) = 0;
            };
            Listener(EventHandler& eventhandler) : eventhandler_(eventhandler) {}
            virtual ~Listener();
            
            virtual const char* GetObjectName() {
                return "base::net::Listener";
            }

            bool Bind(const char* ipaddr, int port);

        private:
            virtual void OnEventIOReadable();
            virtual void OnEventIOWriteable();
            virtual void OnEventIOClose() {}

            void OnAccept(SOCKET clientfd) {
                eventhandler_.OnListenerAccept(this, clientfd);
            }

            EventHandler& eventhandler_;
            DISABLE_COPY(Listener)
        };
    }
}
#endif // LISTENER_H
