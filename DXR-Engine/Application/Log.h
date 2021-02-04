#pragma once
#include "Platform/PlatformOutputDevice.h"

#define LOG_ERROR(Message) \
    { \
        VALIDATE(gConsoleOutput != nullptr); \
        gConsoleOutput->SetColor(EConsoleColor::Red); \
        gConsoleOutput->Print(std::string(Message) + "\n"); \
        gConsoleOutput->SetColor(EConsoleColor::White); \
    }

#define LOG_WARNING(Message) \
    { \
        VALIDATE(gConsoleOutput != nullptr); \
        gConsoleOutput->SetColor(EConsoleColor::Yellow); \
        gConsoleOutput->Print(std::string(Message) + "\n"); \
        gConsoleOutput->SetColor(EConsoleColor::White); \
    }

#define LOG_INFO(Message) \
    {\
        VALIDATE(gConsoleOutput != nullptr); \
        gConsoleOutput->SetColor(EConsoleColor::Green); \
        gConsoleOutput->Print(std::string(Message) + "\n"); \
        gConsoleOutput->SetColor(EConsoleColor::White); \
    }