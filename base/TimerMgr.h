
#pragma once

#include "IDGenerator.h"
#include "Singleton.h"

#include <map>
#include <functional>

class MemoryHandle
{
private:
	DWORD m_handle;

public:
	enum { INVALID_MEMORY_HANDLE };

	MemoryHandle(DWORD handle) : m_handle(handle)
	{

	}

	~MemoryHandle()
	{
		m_handle = INVALID_MEMORY_HANDLE;
	}

	DWORD GetHandle() const
	{
		return m_handle;
	}

	BOOL IsValid() const
	{
		return !(INVALID_MEMORY_HANDLE == m_handle);
	}

	BOOL IsValidWith(DWORD handle) const
	{
		return (handle == m_handle);
	}
};

class NowTickProvider 
{
public:
    NowTickProvider()
    {
        m_init = ::GetTickCount64();
    }

    ULONGLONG operator()() {
        return ::GetTickCount64() - m_init;
    }

private:
    ULONGLONG m_init = 0;
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

	DWORD m_dwJobHandle;
	ULONG m_ulEventTick;
	ULONG m_ulEventGap;
	TimerJob* m_pJob;
	DWORD m_hMemory;
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
	typedef std::map<DWORD, TimeEventObject*>		MapTimerRepeated;
	typedef MapTimerRepeated::iterator							MapTimerRepeatedItr;
	typedef MapTimerRepeated::const_iterator				MapTimerRepeatedCItr;
	typedef std::vector<TimeEventObject*>					ArrayTimerExcute;
	typedef ArrayTimerExcute::iterator								ArrayTimerExcuteItr;
	typedef ArrayTimerExcute::const_iterator					ArrayTimerExcuteCItr;

private:
	DWORD						m_handleNum;
	DWORD						m_dwTick;
	IDGenerator				m_idGenerator;
	MapTimerRepeated	m_mapTimerRepeated;
	
    NowTickProvider m_tickprovider;

private:

	// member function call
	template <class T> DWORD AddFunction( int, T*, void (T::*CBFunc)( void* CBData ), void*, int, int ); 

	// static function call
	DWORD AddFunctionStatic( int, void (*CBFunc)( void* CBData ),	void*, int, int  );	

public:

	void UpdateTimer(); 

	void RemoveFunction(DWORD);

	size_t GetTimerCount() const { return m_mapTimerRepeated.size(); }

    /*
        iDelayFromNow 延迟 iDelayFromNow秒启动定时器
    	iRepeatCycle: 两次重复的间隔时间 > 0,  默认为 1秒
    	iRepeatCnt : -1 时为不限次数 > 0为具体的次数，不能为 0，且不可小于 -1
    */
    //添加 成员函数
	template <class T> DWORD AddMemberFunctionTimeRepeat(int iDelayFromNow, T* pT, void (T::*CBFunc)( void* CBData ), void* pData, int iRepeatCycle = 1,  int iRepeatCnt = -1 );
    //添加 静态函数
	DWORD AddStaticFunctionXTimeRepeat(int iDelayFromNow,  void (*CBFunc)( void* CBData ), void* pData, int iRepeatCycle = 1,  int iRepeatCnt = -1 );
    //添加 std::function
    DWORD AddStdFunctionTimeRepeat(int iDelayFromNow, const std::function<void()>& fun, int iRepeatCycle = 1, int iRepeatCnt = -1);

	UnsafeTimerMgr();

	virtual ~UnsafeTimerMgr();
};

#include "TimerMgr.inl"

#define UnsafeTimerInst  UnsafeTimerMgr::Instance()