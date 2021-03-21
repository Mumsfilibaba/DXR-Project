#pragma once
#include "Debug/GenericDebugMisc.h"

#include "Windows.h"

class WindowsDebugMisc : public GenericDebugMisc
{
public:
    FORCEINLINE static void DebugBreak()
    {
        __debugbreak();
    }

    FORCEINLINE static void OutputDebugString(const std::string& Message)
    {
        OutputDebugStringA(Message.c_str());
    }

    FORCEINLINE static bool IsDebuggerPresent()
    {
        return IsDebuggerPresent();
    }
};