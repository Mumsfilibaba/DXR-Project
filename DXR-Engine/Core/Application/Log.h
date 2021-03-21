#pragma once
#include "Platform/PlatformOutputDevice.h"

#define LOG_ERROR(Message) \
    { \
        Assert(GConsoleOutput != nullptr); \
        GConsoleOutput->SetColor(EConsoleColor::Red); \
        GConsoleOutput->Print(std::string(Message) + "\n"); \
        GConsoleOutput->SetColor(EConsoleColor::White); \
    }

#define LOG_WARNING(Message) \
    { \
        Assert(GConsoleOutput != nullptr); \
        GConsoleOutput->SetColor(EConsoleColor::Yellow); \
        GConsoleOutput->Print(std::string(Message) + "\n"); \
        GConsoleOutput->SetColor(EConsoleColor::White); \
    }

#define LOG_INFO(Message) \
    {\
        Assert(GConsoleOutput != nullptr); \
        GConsoleOutput->SetColor(EConsoleColor::Green); \
        GConsoleOutput->Print(std::string(Message) + "\n"); \
        GConsoleOutput->SetColor(EConsoleColor::White); \
    }