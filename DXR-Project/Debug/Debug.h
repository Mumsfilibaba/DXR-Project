#pragma once
#include "Defines.h"
#include "Types.h"

#include <string>

#ifdef OutputDebugString
	#undef OutputDebugString
#endif

class Debug
{
public:
	static void DebugBreak();
	static void OutputDebugString(const std::string& Message);
};