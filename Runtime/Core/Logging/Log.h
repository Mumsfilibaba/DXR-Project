#pragma once
#include "CoreApplication/Platform/PlatformConsoleWindow.h"

// TODO: Use print line instead of print

#define LOG_ERROR(Message)                              \
    {                                                   \
        Assert(GConsoleWindow != nullptr);              \
        GConsoleWindow->SetColor(EConsoleColor::Red);   \
        GConsoleWindow->Print(CString(Message) + "\n"); \
        GConsoleWindow->SetColor(EConsoleColor::White); \
    }

#define LOG_WARNING(Message)                             \
    {                                                    \
        Assert(GConsoleWindow != nullptr);               \
        GConsoleWindow->SetColor(EConsoleColor::Yellow); \
        GConsoleWindow->Print(CString(Message) + "\n");  \
        GConsoleWindow->SetColor(EConsoleColor::White);  \
    }

#define LOG_INFO(Message)                               \
    {                                                   \
        Assert(GConsoleWindow != nullptr);              \
        GConsoleWindow->SetColor(EConsoleColor::Green); \
        GConsoleWindow->Print(CString(Message) + "\n"); \
        GConsoleWindow->SetColor(EConsoleColor::White); \
    }