#ifndef BASE_EVENT_DISPATCHER_H
#define BASE_EVENT_DISPATCHER_H

#include <cstddef>
#include <stdint.h>
#include <map>
#include <queue>
#include <functional>
#include <time.h>
#include <WinSock2.h>
#include "TimerMgr.h"
#include "intrusive_list.h"

namespace base
{
        class EventIO;
        class DispatcherImpl;

        class Dispatcher
        {
        public:
            struct QuickTickProvider {
                int64_t operator()() {
                    return Dispatcher::instance().GetTickCache();
                }
            };
        public:
            static Dispatcher& instance() {
                static Dispatcher ins;
                return ins;
            }
            ~Dispatcher();

            uint64_t frame_no() const {
                return frame_no_;
            }
            
            void Dispatcher::ExecuteAtNextLoop(const std::function<void()>& cb);

            void Dispatch();

            int64_t inline GetTickCache() const {
                return tick_;
            }

            int64_t inline GetTimestampCache() const {
                return GetTickCache() / 1000;
            }

            int64_t inline UpdateTickCache();

            void Clear();

            void NormalExit();

            void DebugDump();

            void AddFD(EventIO* fd);

            void SetIOEvent(EventIO* fd, int evt);

            void DelFD(EventIO* fd);

        private:
            Dispatcher();
            fd_set fds_read;
            fd_set fds_write;
            fd_set fds_err;

            uint64_t socket_key_ = 0;
            std::map<uint64_t, EventIO*> all_sockets_;
            std::queue<EventIO*> closed_io_list_;
            utils::IntrusiveList<EventIO> io_list_;
            EventIO* cur_select_ = nullptr;

            bool exit_;
            int64_t tick_last_;
            int64_t tick_;
            int64_t ts_begin_ = 0;
            int64_t tick_begin_ = 0;
            int wait_;
            uint64_t frame_no_;
            std::vector<std::function<void()>> execute_at_next_loop_;
            DispatcherImpl* impl_ = nullptr;
            friend class EventTimeout;
            friend class EventIO;
            friend class EventUpdate;
            friend class DispatcherImpl;
        };
}

extern base::Dispatcher* g_dispatcher;

#endif // EVENTDISPATCHER_H
