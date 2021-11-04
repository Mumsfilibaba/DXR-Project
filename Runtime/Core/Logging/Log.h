#pragma once
#include "Platform/PlatformOutputConsole.h"

// TODO: Use print line instead of print

#define LOG_ERROR(Message)                              \
    {                                                   \
        Assert(GConsoleOutput != nullptr);              \
        GConsoleOutput->SetColor(EConsoleColor::Red);   \
        GConsoleOutput->Print(CString(Message) + "\n"); \
        GConsoleOutput->SetColor(EConsoleColor::White); \
    }

#define LOG_WARNING(Message)                             \
    {                                                    \
        Assert(GConsoleOutput != nullptr);               \
        GConsoleOutput->SetColor(EConsoleColor::Yellow); \
        GConsoleOutput->Print(CString(Message) + "\n");  \
        GConsoleOutput->SetColor(EConsoleColor::White);  \
    }

#define LOG_INFO(Message)                               \
    {                                                   \
        Assert(GConsoleOutput != nullptr);              \
        GConsoleOutput->SetColor(EConsoleColor::Green); \
        GConsoleOutput->Print(CString(Message) + "\n"); \
        GConsoleOutput->SetColor(EConsoleColor::White); \
    }