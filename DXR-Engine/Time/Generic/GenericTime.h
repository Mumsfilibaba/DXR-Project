#pragma once
#include "Core.h"

/*
* GenericTime
*/

class GenericTime
{
public:
	static FORCEINLINE UInt64 QueryPerformanceCounter()
	{
		return 0;
	}

	static FORCEINLINE UInt64 QueryPerformanceFrequency()
	{
		return 1;
	}
};