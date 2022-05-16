#pragma once
#include "Core/Misc/EngineGlobals.h"

#include "CoreApplication/Platform/PlatformConsoleWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Logging macros

#define LOG_ERROR(...)                                                                    \
    do                                                                                    \
    {                                                                                     \
        if (NErrorDevice::GConsoleWindow)                                                 \
        {                                                                                 \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Red);                   \
            NErrorDevice::GConsoleWindow->PrintLine(String::CreateFormated(__VA_ARGS__)); \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White);                 \
        }                                                                                 \
    } while (false)

#define LOG_WARNING(...)                                                                  \
    do                                                                                    \
    {                                                                                     \
        if (NErrorDevice::GConsoleWindow)                                                 \
        {                                                                                 \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Yellow);                \
            NErrorDevice::GConsoleWindow->PrintLine(String::CreateFormated(__VA_ARGS__)); \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White);                 \
        }                                                                                 \
    } while (false)

#define LOG_INFO(...)                                                                     \
    do                                                                                    \
    {                                                                                     \
        if (NErrorDevice::GConsoleWindow)                                                 \
        {                                                                                 \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Green);                 \
            NErrorDevice::GConsoleWindow->PrintLine(String::CreateFormated(__VA_ARGS__)); \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White);                 \
        }                                                                                 \
    } while (false)
