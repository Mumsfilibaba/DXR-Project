#pragma once
#include "Debug/GenericDebugMisc.h"

#include "Windows.h"

/*
* WindowsDebugMisc
*/

class WindowsDebugMisc : public GenericDebugMisc
{
public:
	void static FORCEINLINE DebugBreak()
	{
		__debugbreak();
	}

	void static FORCEINLINE OutputDebugString(const std::string& Message)
	{
		::OutputDebugStringA(Message.c_str());
	}
};