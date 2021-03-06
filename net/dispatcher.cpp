#include "dispatcher.h"
#include "eventio.h"
#include <mutex>
#include <math.h>
#include "../base/TimerMgr.h"

base::Dispatcher* g_dispatcher = nullptr;

namespace base
{
        using namespace std;

        /// DispatcherImpl;
        class DispatcherImpl
        {
        public:
            
            bool TryLock()
            {
                return m_cs.try_lock();
            }

            void Lock()
            {
                m_cs.lock();
            }

            void Unlock()
            {
                m_cs.unlock();
            }
            
        protected:

            std::mutex m_cs;
        };

        /// EventDispatcher
        Dispatcher::Dispatcher()
            : exit_(false), tick_last_(0), tick_(0), wait_(25), frame_no_(0)
        {            
            impl_ = new DispatcherImpl;
            UpdateTickCache();
            g_dispatcher = this;
        }

        Dispatcher::~Dispatcher()
        {
            delete impl_;
            g_dispatcher = nullptr;
        }

        void Dispatcher::NormalExit()
        {
            exit_ = true;
        }


        int64_t Dispatcher::UpdateTickCache()
        {
            tick_ = nowtick();
            return tick_;
        }

        void Dispatcher::Clear()
        {
            // TODO
        }

        void Dispatcher::DebugDump()
        {
            //cout << "Dispatcher: is_exit=" << boolalpha << exit_ <<
            //     ", io_list_=" << io_list_.size() <<
            //     ", closed_io_list_=" << closed_io_list_.size() << endl;
        }


        void Dispatcher::AddFD(EventIO* fd)
        {
            fd->Retain();
            fd->SetKey(++socket_key_);
            all_sockets_.emplace(fd->Key(), fd);

            io_list_.push_back(fd);
            if (cur_select_ == nullptr)
            {
                cur_select_ = fd;
            }
        }

        void Dispatcher::SetIOEvent(EventIO* fd, int evt)
        {
            if (evt & IO_READABLE) {
                FD_SET(fd->fd(), &fds_read);
            }
            if (evt & IO_WRITEABLE) {
                FD_SET(fd->fd(), &fds_write);
            }
        }

        void Dispatcher::DelFD(EventIO* fd)
        {
            closed_io_list_.push(fd);
        }

        //??????????????cb
        void Dispatcher::ExecuteAtNextLoop(const std::function<void()>& cb)
        {
            impl_->Lock();
            execute_at_next_loop_.push_back(cb);
            impl_->Unlock();
        }

        void Dispatcher::Dispatch()
        {
            int maxfd = 0;
            int32_t BUSY_WEIGHT = wait_ * 2;

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = wait_ * 1000;

            UpdateTickCache();
            tick_last_ = tick_;
            int64_t tick_span = 0;
            int64_t last_log_ts = 0;
            int32_t busy_count = 0;
            int64_t max_cost_time = 0;
            int n = 0;
            while (!exit_) {
                ++frame_no_;
                // update tick
                UpdateTickCache();
                tick_span = tick_ - tick_last_;
                tick_last_ = tick_;
                
                if (tick_span > BUSY_WEIGHT) {
                    ++busy_count;
                    if (tick_span > max_cost_time) {
                        max_cost_time = tick_span;
                    }
                    if (last_log_ts < tick_ - 10000) {
                        //LOG_WARN("process is too busy, cost_time=%dms, max_cost_time=%d, busy_count=%d\n", tick_span, max_cost_time, busy_count);
                        last_log_ts = tick_;
                        max_cost_time = 0;
                        busy_count = 0;
                    }
                }

                if (!execute_at_next_loop_.empty()) {
                    if (impl_->TryLock())
                    {
                        std::vector<std::function<void()>> tmp;
                        tmp = execute_at_next_loop_;
                        execute_at_next_loop_.clear();
                        impl_->Unlock();
                        
                        for (const auto & cb : tmp) {
                            cb();
                        }
                    }
                }

                UnsafeTimerInst.UpdateTimer();

				int select_cnt = io_list_.size() / FD_SETSIZE  + 1;
                int wait_usec = wait_ * 1000 / select_cnt;

				while (select_cnt > 0)
				{
                    maxfd = 0;
					--select_cnt;
					FD_ZERO(&fds_read);
					FD_ZERO(&fds_write);
					FD_ZERO(&fds_err);

					if (!io_list_.empty())
					{
						for (int i = 0; i < FD_SETSIZE; ++i)
						{
							if (cur_select_ == nullptr)
							{
								cur_select_ = io_list_.front();
							}
							if (cur_select_)
							{
                                if (cur_select_->fd() > maxfd)
                                {
                                    maxfd = cur_select_->fd();
                                }
								cur_select_->SetIOEvent();
								cur_select_ = io_list_.next(cur_select_);
							}
							if (cur_select_ == nullptr)
							{
								break;
							}
						}
					}

                    timeout.tv_sec = 0;
                    timeout.tv_usec = wait_usec;
					n = select(maxfd + 1, &fds_read, &fds_write, &fds_err, &timeout);

					if (n <= -1) {

#ifdef _WIN32
                        int err = WSAGetLastError();
                        // error
#else

#endif // _WIN32
						// TODO error handle
					}
					else if (n == 0) {
						// timeout
					}
					else {
						// handle io event
						EventIO* pre_evt_io = nullptr;
						if (cur_select_ == nullptr)
						{
							pre_evt_io = io_list_.back();
						}
						else
						{
							pre_evt_io = io_list_.pre(cur_select_);
						}
						int tmp = 0;
						for (int x = 0; x < FD_SETSIZE; ++x)
						{
							EventIO* evt_io = pre_evt_io;
							if (evt_io == nullptr)
							{
								break;
							}
							pre_evt_io = io_list_.pre(evt_io);

							SOCKET fd = evt_io->fd();
							if (FD_ISSET(fd, &fds_read))
							{
								++tmp;
								evt_io->OnEventIOReadable();
							}
							if (FD_ISSET(fd, &fds_write))
							{
								++tmp;
								evt_io->OnEventIOWriteable();
							}

							if (tmp >= n)
							{
								break;
							}
						}
					}
				}

                while (!closed_io_list_.empty()) {
                    EventIO* cur = closed_io_list_.front();
                    EventIO* next = io_list_.erase(cur);
                    if (cur_select_ == cur)
                    {
                        cur_select_ = next;
                    }
                    all_sockets_.erase(cur->Key());
                    cur->OnEventIOClose();
                    cur->Release();
                    closed_io_list_.pop();
                }
            }
        }
}
