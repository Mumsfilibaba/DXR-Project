#pragma once
#include "Core/Misc/EngineGlobals.h"

#include "CoreApplication/Platform/PlatformConsoleWindow.h"

// TODO: Use print line instead of print

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Logging macros

#define LOG_ERROR(Message)                              \
    {                                                   \
        Check(NErrorDevice::GConsoleWindow != nullptr);              \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Red);   \
        NErrorDevice::GConsoleWindow->Print(String(Message) + "\n"); \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White); \
    }

#define LOG_WARNING(Message)                             \
    {                                                    \
        Check(NErrorDevice::GConsoleWindow != nullptr);               \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Yellow); \
        NErrorDevice::GConsoleWindow->Print(String(Message) + "\n");  \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White);  \
    }

#define LOG_INFO(Message)                               \
    {                                                   \
        Check(NErrorDevice::GConsoleWindow != nullptr);              \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Green); \
        NErrorDevice::GConsoleWindow->Print(String(Message) + "\n"); \
        NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White); \
    }