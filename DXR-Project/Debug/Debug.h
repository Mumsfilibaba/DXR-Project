#pragma once
#include "Defines.h"

#include <string>

#ifdef OutputDebugString
	#undef OutputDebugString
#endif

/*
* Debug
*/

class Debug
{
public:
	static void DebugBreak();
	static void OutputDebugString(const std::string& Message);
};