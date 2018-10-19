
#pragma once

#include "IDGenerator.h"
#include "Singleton.h"

#include <map>
#include <functional>

#ifdef _WIN32
#include <Windows.h>
#elif __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif // _WIN32

class MemoryHandle
{
private:
	unsigned long m_handle;

public:
	enum { INVALID_MEMORY_HANDLE };

	MemoryHandle(unsigned long handle) : m_handle(handle)
	{

	}

	~MemoryHandle()
	{
		m_handle = INVALID_MEMORY_HANDLE;
	}

	unsigned long GetHandle() const
	{
		return m_handle;
	}

	bool IsValid() const
	{
		return !(INVALID_MEMORY_HANDLE == m_handle);
	}

    bool IsValidWith(unsigned long handle) const
	{
		return (handle == m_handle);
	}
};

class NowTickProvider 
{
public:
    NowTickProvider()
    {
#ifdef _WIN32
        m_init = ::GetTickCount64();
#endif // _WIN32
    }

    unsigned long long operator()() {
#ifdef _WIN32
        return ::GetTickCount64() - m_init;
#elif defined __APPLE__
        timeval now;
        gettimeofday(&now, NULL);
        return now.tv_sec * 1000 + now.tv_usec / 1000;
#else
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        return now.tv_sec * 1000 + now.tv_nsec / 1000000;
#endif // _WIN32
    }

private:
    unsigned long long m_init = 0;
};

class TimerJob
{
public:
	TimerJob() {}
	virtual ~TimerJob() {}
	virtual void Execute() = 0;
};


class StdFunctionTimerJob : public TimerJob
{
public:
    StdFunctionTimerJob(const std::function<void()>& fun)
        :m_fun(fun) {}
    ~StdFunctionTimerJob() {}
    virtual void Execute() { m_fun(); }

private:

    std::function<void()> m_fun;
};


class StaticTimerJob : public TimerJob
{
public:
	typedef void (*CBFunc)( void* cbData );
private:
	void* m_pData;
	CBFunc m_pfnFunc;

public:
	StaticTimerJob( void* cbData, CBFunc cbFunc ) : m_pData( cbData ), m_pfnFunc ( cbFunc ) {}
	~StaticTimerJob() {}
	virtual void Execute() { (*m_pfnFunc)( m_pData ); }
};

template 
<
	class T
>
class MemberTimerJob : public TimerJob
{
public:
	typedef void (T::*CBFunc)( void* cbData );

private:
	T*		m_pObject;
	void*	m_pData;
	CBFunc	m_pfnFunc;
public:
	MemberTimerJob( T * object, void* cbData, CBFunc cbFunc ) 	: m_pObject( object )	, m_pData ( cbData ), m_pfnFunc( cbFunc ) { }
	virtual ~MemberTimerJob() { } 
	virtual void Execute( ) { (m_pObject->*m_pfnFunc)( m_pData ); 	}
};

class TimeEventObject
{
	friend class SafeTimerMgr;
	friend class UnsafeTimerMgr;

private:

	unsigned long m_dwJobHandle;
    unsigned long m_ulEventTick;
    unsigned long m_ulEventGap;
	TimerJob* m_pJob;
	unsigned long m_hMemory;
	int m_iRepeatCnt;
	MemoryHandle* m_pMemoryHandle;

public:
	TimeEventObject()
	{
		m_ulEventTick = m_ulEventGap = 0;
		m_pJob = NULL;			
		m_dwJobHandle = 0;
		m_hMemory = 0;
		m_iRepeatCnt = -1;
		m_pMemoryHandle = NULL;
	}
};

/*
线程非安全的定时器管理类，只适合单线程调用，比如gameserver的逻辑单线程调用
*/
class UnsafeTimerMgr : public Singleton<UnsafeTimerMgr>
{
private:
	typedef std::map<unsigned long, TimeEventObject*>		MapTimerRepeated;
	typedef MapTimerRepeated::iterator							MapTimerRepeatedItr;
	typedef MapTimerRepeated::const_iterator				MapTimerRepeatedCItr;
	typedef std::vector<TimeEventObject*>					ArrayTimerExcute;
	typedef ArrayTimerExcute::iterator								ArrayTimerExcuteItr;
	typedef ArrayTimerExcute::const_iterator					ArrayTimerExcuteCItr;

private:
	unsigned long						m_handleNum;
	unsigned long						m_dwTick;
	IDGenerator				m_idGenerator;
	MapTimerRepeated	m_mapTimerRepeated;
	
    NowTickProvider m_tickprovider;

private:

	// member function call
	template <class T> unsigned long AddFunction( int, T*, void (T::*CBFunc)( void* CBData ), void*, int, int ); 

	// static function call
	unsigned long AddFunctionStatic( int, void (*CBFunc)( void* CBData ),	void*, int, int  );	

public:

	void UpdateTimer(); 

	void RemoveFunction(unsigned long);

	size_t GetTimerCount() const { return m_mapTimerRepeated.size(); }

    /*
        iDelayFromNow 延迟 iDelayFromNow秒启动定时器
    	iRepeatCycle: 两次重复的间隔时间 > 0,  默认为 1秒
    	iRepeatCnt : -1 时为不限次数 > 0为具体的次数，不能为 0，且不可小于 -1
    */
    //添加 成员函数
	template <class T> unsigned long AddMemberFunctionTimeRepeat(int iDelayFromNow, T* pT, void (T::*CBFunc)( void* CBData ), void* pData, int iRepeatCycle = 1,  int iRepeatCnt = -1 );
    //添加 静态函数
	unsigned long AddStaticFunctionXTimeRepeat(int iDelayFromNow,  void (*CBFunc)( void* CBData ), void* pData, int iRepeatCycle = 1,  int iRepeatCnt = -1 );
    //添加 std::function
    unsigned long AddStdFunctionTimeRepeat(int iDelayFromNow, const std::function<void()>& fun, int iRepeatCycle = 1, int iRepeatCnt = -1);

	UnsafeTimerMgr();

	virtual ~UnsafeTimerMgr();
};

#include "TimerMgr.inl"

#define UnsafeTimerInst  UnsafeTimerMgr::Instance()
