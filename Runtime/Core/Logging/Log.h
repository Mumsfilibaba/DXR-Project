#pragma once
#include "Core/Misc/EngineGlobals.h"

#include "CoreApplication/Platform/PlatformConsoleWindow.h"

// TODO: Use print line instead of print

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Logging macros

#define LOG_ERROR(Message)                              \
    {                                                   \
        Assert(NErrorDevice::GConsoleWindow != nullptr);              \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Red);   \
        NErrorDevice::GConsoleWindow->Print(CString(Message) + "\n"); \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White); \
    }

#define LOG_WARNING(Message)                             \
    {                                                    \
        Assert(NErrorDevice::GConsoleWindow != nullptr);               \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Yellow); \
        NErrorDevice::GConsoleWindow->Print(CString(Message) + "\n");  \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White);  \
    }

#define LOG_INFO(Message)                               \
    {                                                   \
        Assert(NErrorDevice::GConsoleWindow != nullptr);              \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Green); \
        NErrorDevice::GConsoleWindow->Print(CString(Message) + "\n"); \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White); \
    }