#ifndef BASE_OBJECT_H
#define BASE_OBJECT_H

#include "Global.h"

namespace base
{
    // 引用计数的对象
    class Object
    {
    public:
        Object();

        // 对象的引用次数
        uint32_t reference_count() const {
            return reference_count_;
        }

        // 是否单一引用
        bool IsSingleReference() const {
            return reference_count_ == 1;
        }
        // 释放一次引用
        void Release();
        // 持有一次引用
        void Retain();
        
        virtual const char* GetObjectName() {
            return "base::Object";
        }

    protected:
        virtual ~Object();

    private:
        uint32_t reference_count_;
    };

    class ObjectScope
    {
    public:
        ObjectScope(Object* o, bool retain = false) : o_(o) {
            if (retain) {
                o_->Retain();
            }
        }
        ~ObjectScope() {
            o_->Release();
        }
        Object* get() {
            return o_;
        }

        DISABLE_COPY(ObjectScope);
    private:
        Object* o_;
    };
}

#define SAFE_RETAIN(obj) do { if (obj != nullptr) { obj->Retain(); } } while (0)
#define SAFE_RELEASE(obj) do { if (obj != nullptr) { obj->Release(); obj = nullptr; } } while(0)

#endif // OBJECT_H
