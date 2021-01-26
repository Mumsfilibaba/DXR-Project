#pragma once
#include "Platform/PlatformOutputDevice.h"

#define LOG_ERROR(Message) \
    { \
        VALIDATE(GlobalConsoleOutput != nullptr); \
        GlobalConsoleOutput->SetColor(EConsoleColor::ConsoleColor_Red); \
        GlobalConsoleOutput->Print(std::string(Message) + "\n"); \
        GlobalConsoleOutput->SetColor(EConsoleColor::ConsoleColor_White); \
    }

#define LOG_WARNING(Message) \
    { \
        VALIDATE(GlobalConsoleOutput != nullptr); \
        GlobalConsoleOutput->SetColor(EConsoleColor::ConsoleColor_Yellow); \
        GlobalConsoleOutput->Print(std::string(Message) + "\n"); \
        GlobalConsoleOutput->SetColor(EConsoleColor::ConsoleColor_White); \
    }

#define LOG_INFO(Message) \
    {\
        VALIDATE(GlobalConsoleOutput != nullptr); \
        GlobalConsoleOutput->SetColor(EConsoleColor::ConsoleColor_Green); \
        GlobalConsoleOutput->Print(std::string(Message) + "\n"); \
        GlobalConsoleOutput->SetColor(EConsoleColor::ConsoleColor_White); \
    }