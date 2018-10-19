#pragma once

#include <queue>
//#include <windows.h>

#include <atomic>

class IDGenerator
{
public:
    std::atomic_ulong m_dwLastID;

public:
	IDGenerator()
	{
	}

	inline unsigned long GetUniqueID()
	{
        return ++m_dwLastID;
		//return ::InterlockedIncrement( (LONG volatile *) &m_dwLastID );
	}
};

