#ifndef BASE_OBSERVER_H
#define BASE_OBSERVER_H

#include "Object.h"

namespace base
{
    class AutoObserver;
    class Observer : public Object
    {
    public:

        virtual ~Observer() {}

        bool IsExist() const {
            return exist_;
        }

        void SetNotExist() {
            exist_ = false;
        }
        
        virtual const char* GetObjectName() {
            return "base::Observer";
        }

    private:
        Observer() : exist_(true) {}
        bool exist_;

        friend class AutoObserver;
    };

    class AutoObserver
    {
    public:
        AutoObserver() : observer_(new Observer) {}
        ~AutoObserver() {
            observer_->SetNotExist();
            observer_->Release();
        }

        Observer* GetObserver() const {
            return observer_;
        }

        void SetNotExist() {
            observer_->SetNotExist();
        }

        // 重新创建，使以前的连接失败
        void ReCreate() {
            observer_->SetNotExist();
            observer_->Release();
            observer_ = new Observer;
        }

        AutoObserver(const AutoObserver& rhs) = delete;
        AutoObserver& operator = (const AutoObserver& rhs) = delete;

    private:
        Observer* observer_;
    };
}

#endif // OBSERVER_H
