#pragma once
#include "Debug/GenericDebugMisc.h"

#include "Windows.h"

class WindowsDebugMisc : public GenericDebugMisc
{
public:
    static FORCEINLINE void DebugBreak()
    {
        __debugbreak();
    }

    static FORCEINLINE void OutputDebugString(const std::string& Message)
    {
        ::OutputDebugStringA(Message.c_str());
    }

    static FORCEINLINE Bool IsDebuggerPresent()
    {
        return ::IsDebuggerPresent();
    }
};