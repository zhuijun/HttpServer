#include "eventio.h"
#include "dispatcher.h"


namespace base
{
        using namespace std;

        EventIO::~EventIO()
        {
        }

        void EventIO::AddToDispatcher(SOCKET fd, int evt)
        {
            ioevt_ = evt;
            Dispatcher::instance().AddFD(this);
        }

        void EventIO::ModifyIOEvent(int ioevt)
        {
            ioevt_ = ioevt;
        }

        void EventIO::CloseFD()
        {
            if (fd_ != INVALID_SOCKET) {
                closesocket(fd_);
                fd_ = INVALID_SOCKET;
            }
        }

        void EventIO::Close()
        {
            if (!closed_ && list_linked())
            {
                ioevt_ = 0;
                closed_ = true;
                Dispatcher::instance().DelFD(this);
                CloseFD();
            }
        }

        void EventIO::SetIOEvent()
        {
            Dispatcher::instance().SetIOEvent(this, ioevt_);
        }

}
