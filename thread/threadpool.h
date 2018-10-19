#ifndef BASE_THREAD_THREADPOOL_H
#define BASE_THREAD_THREADPOOL_H

#include "Global.h"
#include <memory>

namespace base
{
    namespace thread
    {
        class Task;
        class ThreadPoolImpl;
        class ThreadPool
        {
        public:
            static ThreadPool* getInstance();

            ThreadPool();
            ~ThreadPool();

            void start(int threadCnt);
            void stop();

            void queueUserWorkItem(std::shared_ptr<Task> task);

        private:
            void* workerThread(void* args);

            int thread_min_;
            bool run_;

            ThreadPoolImpl* impl_;
            friend class ThreadPoolImpl;
        };
    }
}

#endif
