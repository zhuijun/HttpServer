#pragma once

#include <queue>
#include <windows.h>

class IDGenerator
{
public:
	DWORD m_dwLastID;

public:
	IDGenerator() : m_dwLastID(0)
	{
	}

	inline DWORD GetUniqueID()
	{
		return ::InterlockedIncrement( (LONG volatile *) &m_dwLastID );
	}
};

