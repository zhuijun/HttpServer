#include "task.h"
#include "threadpool.h"
#include "../net/dispatcher.h"
#include <functional>

namespace base
{
    namespace thread
    {
        /// Task
        Task::~Task()
        {
        }

        /// AsyncTask
        AsyncTask::AsyncTask()
        {
            onPreExecute();
        }

        AsyncTask::~AsyncTask()
        {
        }

        void AsyncTask::OnTaskExecute()
        {
            doInWorkThread();
            std::shared_ptr<Task> pThis = shared_from_this();
            g_dispatcher->ExecuteAtNextLoop([this, pThis]() {
                this->invokePostExecute();
            });
        }

        void AsyncTask::invokePostExecute()
        {
            is_done_ = true;
            onPostExecute();
        }
    }
}
