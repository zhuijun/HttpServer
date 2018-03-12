#ifndef BASE_EVENT_EVENTIO_H
#define BASE_EVENT_EVENTIO_H
#include <WinSock2.h>
#include <cstddef>
#include "object.h"
#include "intrusive_list.h"


namespace base
{
    enum IOEventType {
        IO_NONE             = 0,   
        IO_READABLE         = 1, 
        IO_WRITEABLE        = 2,   
    };

    class EventIO : public Object
    {
    public:
        EventIO() {}
        virtual ~EventIO();
            
        virtual const char* GetObjectName() {
            return "base::event::EventIO";
        }

        bool closed() const {
            return closed_;
        }

        SOCKET fd() const {
            return fd_;
        }

        void SetKey(uint64_t key)
        {
            key_ = key;
        }

        uint64_t Key() const {
            return key_;
        }

        void AddToDispatcher(SOCKET fd, int evt);

        void ModifyIOEvent(int ioevt);

        virtual void Close();

        void SetIOEvent();

protected:
        void CloseFD();

        virtual void OnEventIOReadable() = 0;

        virtual void OnEventIOWriteable() = 0;

        virtual void OnEventIOClose() = 0;

        int ioevt_ = IO_NONE;
        SOCKET fd_ = INVALID_SOCKET;
        bool closed_ = false;
        uint64_t key_ = 0;
        INTRUSIVE_LIST(EventIO)
        friend class Dispatcher;
    };
}

#endif // IOEVENT_H
