#include "TimerMgr.h"
#include "Global.h"

UnsafeTimerMgr::UnsafeTimerMgr(void) : m_dwTick(0), m_handleNum(0) 
{
    m_dwTick = m_tickprovider();
}

UnsafeTimerMgr::~UnsafeTimerMgr(void)
{

}

void UnsafeTimerMgr::UpdateTimer()
{ 
    m_dwTick = m_tickprovider();

	ArrayTimerExcute array;

	// find 
	MapTimerRepeatedItr itr = m_mapTimerRepeated.begin();
	MapTimerRepeatedCItr endItr = m_mapTimerRepeated.cend();
	for ( ; itr != endItr ; )
	{
		if ( NULL == itr->second )
		{
			itr = m_mapTimerRepeated.erase(itr);
			continue;
		}

		if ( itr->second->m_iRepeatCnt == 0 )
		{
			SAFE_DELETE(itr->second->m_pJob);
			SAFE_DELETE(itr->second);
			itr = m_mapTimerRepeated.erase(itr);
			continue;
		}

		if (itr->second->m_ulEventTick < m_dwTick)
		{
			array.push_back(itr->second);
			if( itr->second->m_iRepeatCnt != -1 )
			{
				itr->second->m_iRepeatCnt--;
			}

			itr->second->m_ulEventTick += itr->second->m_ulEventGap;
		}

		++itr;
	}
 
	// Excute
	ArrayTimerExcuteItr Itr = array.begin();
	ArrayTimerExcuteCItr EndItr = array.end();
	for ( ; Itr != EndItr ; ++Itr)
	{
		TimeEventObject* timeEventObiect = NULL;
		timeEventObiect = *Itr;

		if ( timeEventObiect->m_pMemoryHandle )
		{
			if ( timeEventObiect->m_pMemoryHandle->IsValidWith( timeEventObiect->m_hMemory) )
			{
				timeEventObiect->m_pJob->Execute();
			}
			else
			{
				timeEventObiect->m_iRepeatCnt = 0; 
			}
		}
		else
		{ 
			timeEventObiect->m_pJob->Execute();
		}
	}
}

void UnsafeTimerMgr::RemoveFunction(DWORD dwJobHandle)
{
	if ( 0 == dwJobHandle )
	{
		return;
	}

	MapTimerRepeatedItr Itr = m_mapTimerRepeated.find(dwJobHandle);
	if ( Itr != m_mapTimerRepeated.cend() )
	{
		if ( Itr->second )
		{
			Itr->second->m_iRepeatCnt = 0;
		}
		else
		{
			m_mapTimerRepeated.erase(Itr);
		}
	}
}

DWORD UnsafeTimerMgr::AddFunctionStatic( int iDelayFromNow, void (*CBFunc)( void* CBData ), void* pData, int iRepeatCycle, int iRepeatCnt )
{
	if ( 0 == iRepeatCycle && ( -1 == iRepeatCnt || 1 < iRepeatCnt  ))
	{
		return 0;
	}

	StaticTimerJob* pJob = new StaticTimerJob(pData, CBFunc);
	TimeEventObject* pObj = new TimeEventObject;

	if (pJob != NULL && pObj != NULL)
	{
        pObj->m_ulEventTick = ((ULONG)iDelayFromNow * 1000) + m_tickprovider();
		pObj->m_pJob = pJob;
		pObj->m_ulEventGap = ((ULONG)iRepeatCycle * 1000);
		pObj->m_iRepeatCnt = iRepeatCnt;
		pObj->m_dwJobHandle = m_idGenerator.GetUniqueID();
		pObj->m_pMemoryHandle = NULL;
		pObj->m_hMemory = 0;

		m_mapTimerRepeated.insert(MapTimerRepeated::value_type(pObj->m_dwJobHandle, pObj));

		return pObj->m_dwJobHandle;
	} 
	else
	{
		return 0;
	}
}

//	iRepeatCycle: 两次重复的间隔时间 > 0,  默认为 1秒
//	iRepeatCnt : -1 时为不限次数 > 0为具体的次数，不能为 0，且不可小于 -1 
DWORD UnsafeTimerMgr::AddStaticFunctionXTimeRepeat(int iDelayFromNow,  void (*CBFunc)( void* CBData ), void* pData, int iRepeatCycle,  int iRepeatCnt )
{
	if ( iRepeatCycle < 1 )
	{
		return 0;
	}

	if ( 0 == iRepeatCnt ||  -1 >  iRepeatCycle )
	{
		return 0;
	}

	return AddFunctionStatic( iDelayFromNow, CBFunc, pData, iRepeatCycle, iRepeatCnt );
}

DWORD UnsafeTimerMgr::AddStdFunctionTimeRepeat(int iDelayFromNow, const std::function<void()>& fun, int iRepeatCycle /*= 1*/, int iRepeatCnt /*= -1*/)
{
    if (iRepeatCycle < 1)
    {
        return 0;
    }

    if (0 == iRepeatCnt || -1 >  iRepeatCycle)
    {
        return 0;
    }

    StdFunctionTimerJob* pJob = new StdFunctionTimerJob(fun);
    TimeEventObject* pObj = new TimeEventObject;

    if (pJob != NULL && pObj != NULL)
    {
        pObj->m_ulEventTick = ((ULONG)iDelayFromNow * 1000) + m_tickprovider();
        pObj->m_pJob = pJob;
        pObj->m_ulEventGap = ((ULONG)iRepeatCycle * 1000);
        pObj->m_iRepeatCnt = iRepeatCnt;
        pObj->m_dwJobHandle = m_idGenerator.GetUniqueID();
        pObj->m_pMemoryHandle = NULL;
        pObj->m_hMemory = 0;

        m_mapTimerRepeated.insert(MapTimerRepeated::value_type(pObj->m_dwJobHandle, pObj));

        return pObj->m_dwJobHandle;
    }
    else
    {
        return 0;
    }
}
