#include "threadpool.h"
#include "task.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <iostream>

namespace base
{
    namespace thread
    {
        static std::mutex g_mutex;

        class ThreadPoolImpl
        {
        public:
            ThreadPoolImpl(ThreadPool& tp) : parent(tp) {
            }

            void workerThread() {
                while (parent.run_) {
                    std::shared_ptr<Task> task = nullptr;
                    std::unique_lock<std::mutex> lk(tasks_mutex);
                    cv.wait(lk, [this]() {
                        return  !parent.run_ || !tasks.empty();
                    });

                    if (!tasks.empty()) {
                        task = tasks.front();
                        tasks.pop();
                    }
                    lk.unlock();

                    if (task.get()) {
                        task->OnTaskExecute();
                    }
                }
            }

            void printLog(const char* msg) {
                std::lock_guard<std::mutex> lock_guard(g_mutex);
                std::cout << "thread=" << std::this_thread::get_id() << ", msg=" << msg << std::endl;
            }

            std::condition_variable cv;

            std::mutex tasks_mutex;
            std::queue<std::shared_ptr<Task>> tasks;
            std::vector<std::thread> workers;

            ThreadPool& parent;
        };

        static ThreadPool* s_instance_ = nullptr;

        ThreadPool* ThreadPool::getInstance()
        {
            if (s_instance_ == nullptr)
            {
                s_instance_ = new ThreadPool();
            }
            return s_instance_;
        }

        ThreadPool::ThreadPool()
            : thread_min_(0)
        {
            impl_ = new ThreadPoolImpl(*this);
        }

        ThreadPool::~ThreadPool()
        {
            delete impl_;
            s_instance_ = nullptr;
        }

        void ThreadPool::start(int threadCnt)
        {
            thread_min_ = threadCnt;
            run_ = true;
            for (int i = 0; i < thread_min_; ++i) {
                std::thread t(&ThreadPoolImpl::workerThread, impl_);
                impl_->workers.emplace_back(std::move(t));
            }
        }

        void ThreadPool::stop()
        {
            run_ = false;

            impl_->cv.notify_all();


            for (std::thread & t : impl_->workers) {
                t.join();
            }
        }

        void ThreadPool::queueUserWorkItem(std::shared_ptr<Task> task)
        {
            {
                std::lock_guard<std::mutex> lock(impl_->tasks_mutex);
                impl_->tasks.push(task);
            }

            impl_->cv.notify_one();
        }
    }
}
