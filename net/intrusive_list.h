#ifndef INTRUSIVE_LIST_H
#define INTRUSIVE_LIST_H

#include <cassert>

#define INTRUSIVE_LIST(t) \
public:\
    base::utils::IntrusiveListMemberHook<t> list_member_hook_;\
    t* list_next() { return list_member_hook_.list_next_; } \
    t* list_pre() { return list_member_hook_.list_pre_; } \
    bool list_linked() const { return list_member_hook_.list_ != nullptr; }

namespace base
{
    namespace utils
    {
        template<typename T>
        class IntrusiveList;

        template<typename T>
        struct IntrusiveListMemberHook {
            IntrusiveListMemberHook()
                : list_pre_(nullptr), list_next_(nullptr), list_(nullptr) {}
            T* list_pre_;
            T* list_next_;
            IntrusiveList<T>* list_;
        };

        template<typename T>
        class IntrusiveList
        {
        public:
            IntrusiveList()
                : head_(NULL), tail_(NULL), size_(0) {}

            bool empty() const {
                return size_ == 0;
            }

            size_t size() const {
                return size_;
            }

            T* front() {
                return head_;
            }
            
            T* back() {
                return tail_;
            }

            bool contains(T* item) const {
                return item->list_member_hook_.list_ == this;
            }

            template<typename T1>
            static T1* next(T1* item) {
                return item->list_member_hook_.list_next_;
            }
            
            template<typename T1>
            static T1* pre(T1* item) {
                return item->list_member_hook_.list_pre_;
            }

            // erase current element, return next linked element
            T* erase(T* item) {
                T* next = nullptr;
                if (item == head_ && item == tail_) {
                    head_ = nullptr;
                    tail_ = nullptr;
                    next = nullptr;
                } else if (item == head_) {
                    assert(head_->list_member_hook_.list_next_ != nullptr);
                    head_->list_member_hook_.list_next_->list_member_hook_.list_pre_ = nullptr;
                    head_ = head_->list_member_hook_.list_next_;
                    next = head_;
                } else if (item == tail_) {
                    assert(tail_->list_member_hook_.list_pre_ != nullptr);
                    tail_->list_member_hook_.list_pre_->list_member_hook_.list_next_ = nullptr;
                    tail_ = tail_->list_member_hook_.list_pre_;
                    next = nullptr;
                } else {
                    assert(item->list_member_hook_.list_next_ != nullptr);
                    item->list_member_hook_.list_next_->list_member_hook_.list_pre_ = item->list_member_hook_.list_pre_;
                    assert(item->list_member_hook_.list_pre_ != nullptr);
                    item->list_member_hook_.list_pre_->list_member_hook_.list_next_ = item->list_member_hook_.list_next_;
                    next = item->list_member_hook_.list_next_;
                }
                --size_;
                item->list_member_hook_.list_next_ = nullptr;
                item->list_member_hook_.list_pre_ = nullptr;
                item->list_member_hook_.list_ = nullptr;
                return next;
            }

            // push element at front
            void push_front(T* item) {
                if (head_ == NULL && tail_ == NULL) {
                    item->list_member_hook_.list_pre_ = nullptr;
                    item->list_member_hook_.list_next_ = nullptr;
                    head_ = item;
                    tail_ = item;
                } else {
                    item->list_member_hook_.list_next_ = head_;
                    item->list_member_hook_.list_pre_ = nullptr;
                    head_->list_member_hook_.list_pre_ = item;
                    head_ = item;
                }
                item->list_member_hook_.list_ = this;
                ++size_;
            }

            // push element at back
            void push_back(T* item) {
                if (head_ == NULL && tail_ == NULL) {
                    item->list_member_hook_.list_pre_ = nullptr;
                    item->list_member_hook_.list_next_ = nullptr;
                    head_ = item;
                    tail_ = item;
                } else {
                    item->list_member_hook_.list_next_ = nullptr;
                    item->list_member_hook_.list_pre_ = tail_;
                    tail_->list_member_hook_.list_next_ = item;
                    tail_ = item;
                }
                item->list_member_hook_.list_ = this;
                ++size_;
            }

        private:
            T* head_;
            T* tail_;
            size_t size_;
        };
    }
}

#endif
