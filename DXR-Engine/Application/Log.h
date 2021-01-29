#pragma once
#include "Platform/PlatformOutputDevice.h"

#define LOG_ERROR(Message) \
    { \
        VALIDATE(gConsoleOutput != nullptr); \
        gConsoleOutput->SetColor(EConsoleColor::ConsoleColor_Red); \
        gConsoleOutput->Print(std::string(Message) + "\n"); \
        gConsoleOutput->SetColor(EConsoleColor::ConsoleColor_White); \
    }

#define LOG_WARNING(Message) \
    { \
        VALIDATE(gConsoleOutput != nullptr); \
        gConsoleOutput->SetColor(EConsoleColor::ConsoleColor_Yellow); \
        gConsoleOutput->Print(std::string(Message) + "\n"); \
        gConsoleOutput->SetColor(EConsoleColor::ConsoleColor_White); \
    }

#define LOG_INFO(Message) \
    {\
        VALIDATE(gConsoleOutput != nullptr); \
        gConsoleOutput->SetColor(EConsoleColor::ConsoleColor_Green); \
        gConsoleOutput->Print(std::string(Message) + "\n"); \
        gConsoleOutput->SetColor(EConsoleColor::ConsoleColor_White); \
    }