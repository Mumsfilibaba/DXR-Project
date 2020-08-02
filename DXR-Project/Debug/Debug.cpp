#include "Debug.h"

void Debug::DebugBreak()
{
	__debugbreak();
}

void Debug::OutputDebugString(const std::string& Message)
{
	::OutputDebugStringA(Message.c_str());
}
