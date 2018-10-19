#ifndef BASE_THREAD_TASK_H
#define BASE_THREAD_TASK_H

#include "Global.h"
#include <memory>

namespace base
{
    namespace thread
    {
        class Task : public std::enable_shared_from_this<Task>
        {
        public:
            Task() {}
            virtual ~Task();

        private:
            DISABLE_COPY(Task)
            virtual void OnTaskExecute() = 0;
            friend class ThreadPool;
            friend class ThreadPoolImpl;
        };

        class AsyncTask : public Task
        {
        public:
            AsyncTask();
            virtual ~AsyncTask();

            bool is_done() const {
                return is_done_;
            }

        private:
            // execute in main thread
            virtual void onPreExecute() {}
            // execute in main thread
            virtual void onPostExecute() = 0;
            // execute in work thread
            virtual void doInWorkThread() = 0;

            virtual void OnTaskExecute() final;

            void invokePostExecute();

            bool is_done_ = false;
        };
    }
}

#endif
