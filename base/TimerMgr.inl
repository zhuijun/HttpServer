template 
<
	class T
>
DWORD UnsafeTimerMgr::AddFunction( int iDelayFromNow, T* pThis, void (T::*cbFunc)( void* cbData ), void* pData, int iRepeatCycle,  int iRepeatCnt  )
{
	MemberTimerJob<T> * pJob = new MemberTimerJob<T>( (T*)pThis, pData, cbFunc);
	if ( NULL == pJob )
	{
		return 0;
	}

	TimeEventObject* pObj = new TimeEventObject;
	if ( NULL == pObj )
	{
		SAFE_DELETE(pJob);
		return 0;
	}

    pObj->m_ulEventTick = ((ULONG)iDelayFromNow * 1000) + m_tickprovider();
	pObj->m_pJob = pJob;
	pObj->m_ulEventGap = ((ULONG)iRepeatCycle*1000);
	pObj->m_iRepeatCnt = iRepeatCnt;
	pObj->m_dwJobHandle = m_idGenerator.GetUniqueID();
	pObj->m_pMemoryHandle = (MemoryHandle*)pThis;
	pObj->m_hMemory = pObj->m_pMemoryHandle->GetHandle();

	if ( !m_mapTimerRepeated.insert(MapTimerRepeated::value_type(pObj->m_dwJobHandle, pObj)).second)
	{
		SAFE_DELETE(pObj);
		SAFE_DELETE(pJob);
		return 0;
	}

	return pObj->m_dwJobHandle;
}

//	iRepeatCycle: �����ظ��ļ��ʱ�� > 0,  Ĭ��Ϊ 1��
//	iRepeatCnt : -1 ʱΪ���޴��� > 0Ϊ����Ĵ���������Ϊ 0���Ҳ���С�� -1 
template 
<
	class T
>
DWORD UnsafeTimerMgr::AddMemberFunctionTimeRepeat(int iDelayFromNow, T* pT, void (T::*cbFunc)( void* cbData ), void* pData, int iRepeatCycle,  int iRepeatCnt )
{
	if ( iRepeatCycle < 1 )
	{
		return 0;
	}

	if ( 0 == iRepeatCnt ||  -1 >  iRepeatCycle )
	{
		return 0;
	}

	return	AddFunction( iDelayFromNow, pT, cbFunc, pData, iRepeatCycle,  iRepeatCnt );
}