#ifndef BASE_GLOBAL_H
#define BASE_GLOBAL_H

/*
#ifndef __GNUC__
#error "only support gcc"
#endif

#if (__GNUC__ >=4) && (__GNUC_MINOR__ >= 6)
#else
#error "not supported gcc version"
#endif
*/

#include <cstdint>
#define DISABLE_COPY(Class) \
    Class(const Class&) = delete; \
    Class & operator=(const Class&) = delete;

#include <cstddef>
#include <iostream>

#ifndef SAFE_DELETE
#define SAFE_DELETE(ptr) do { if (ptr != nullptr) { delete ptr; ptr = nullptr; } } while(0)
#endif

#define WARN_IF_UNUSED __attribute__ ((warn_unused_result))

#define FIRE_DEPRECATED __attribute__ ((deprecated))

#define FOREACH_DELETE_MAP(map) \
        for (auto it = map.begin(); it != map.end(); ++it) {\
            if (it->second) {\
                delete it->second;\
            }\
        }\
        map.clear()

#define FOREACH_DELETE_LIST(list) \
        for (auto it = list.begin(); it != list.end(); ++it) {\
            if (*it) {\
                delete *it;\
            }\
        }\
        list.clear()

#define FOREACH_DELETE_VECTOR(vector) \
        for (auto it = vector.begin(); it != vector.end(); ++it) {\
            if (*it) {\
                delete *it;\
            }\
        }\
        vector.clear()

#define FOREACH_DELETE_ARRAY(array) \
        for (auto it = array.begin(); it != array.end(); ++it) {\
            if (*it) {\
                delete *it;\
            }\
        }

#define DEFINE_SINGLETON(type) \
    public:\
        static type* Create();\
        static void Destroy();\
        static type* Instance() { return s_instance; }\
    private:\
        static type* s_instance;

#define IMPLEMENT_SINGLETON(type) \
    type* type::s_instance = nullptr;\
    type* type::Create() {\
        if (s_instance == nullptr) {\
            s_instance = new type();\
        }\
        return s_instance;\
    }\
    void type::Destroy() {\
        if (s_instance != nullptr) {\
            delete s_instance;\
            s_instance = nullptr;\
        }\
    }

#define IMPLEMENT_SINGLETON_WITH_INIT(type) \
    type* type::s_instance = nullptr;\
    type* type::Create() {\
        if (s_instance == nullptr) {\
            s_instance = new type();\
            s_instance->Init(); \
        }\
        return s_instance;\
    }\
    void type::Destroy() {\
        if (s_instance != nullptr) {\
            delete s_instance;\
            s_instance = nullptr;\
        }\
    }

#endif

