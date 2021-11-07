#pragma once
#include "Core/Misc/EngineGlobals.h"

#include "CoreApplication/Platform/PlatformConsoleWindow.h"

// TODO: Use print line instead of print

#define LOG_ERROR(Message)                              \
    {                                                   \
        Assert(NErrorDevice::ConsoleWindow != nullptr);              \
        NErrorDevice::ConsoleWindow->SetColor(EConsoleColor::Red);   \
        NErrorDevice::ConsoleWindow->Print(CString(Message) + "\n"); \
        NErrorDevice::ConsoleWindow->SetColor(EConsoleColor::White); \
    }

#define LOG_WARNING(Message)                             \
    {                                                    \
        Assert(NErrorDevice::ConsoleWindow != nullptr);               \
        NErrorDevice::ConsoleWindow->SetColor(EConsoleColor::Yellow); \
        NErrorDevice::ConsoleWindow->Print(CString(Message) + "\n");  \
        NErrorDevice::ConsoleWindow->SetColor(EConsoleColor::White);  \
    }

#define LOG_INFO(Message)                               \
    {                                                   \
        Assert(NErrorDevice::ConsoleWindow != nullptr);              \
        NErrorDevice::ConsoleWindow->SetColor(EConsoleColor::Green); \
        NErrorDevice::ConsoleWindow->Print(CString(Message) + "\n"); \
        NErrorDevice::ConsoleWindow->SetColor(EConsoleColor::White); \
    }