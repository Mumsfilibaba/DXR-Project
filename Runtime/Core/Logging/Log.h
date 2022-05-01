#pragma once
#include "Core/Misc/EngineGlobals.h"

#include "CoreApplication/Platform/PlatformConsoleWindow.h"

// TODO: Use print line instead of print

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Logging macros

#define LOG_ERROR(Message)                                                 \
    do                                                                     \
    {                                                                      \
        if (NErrorDevice::GConsoleWindow)                                  \
        {                                                                  \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Red);    \
            NErrorDevice::GConsoleWindow->Print(String(Message) + "\n");   \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White);  \
        }                                                                  \
    } while (0)

#define LOG_WARNING(Message)                                               \
    do                                                                     \
    {                                                                      \
        if (NErrorDevice::GConsoleWindow)                                  \
        {                                                                  \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Yellow); \
            NErrorDevice::GConsoleWindow->Print(String(Message) + "\n");   \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White);  \
        }                                                                  \
    } while (0)

#define LOG_INFO(Message)                                                 \
    do                                                                    \
    {                                                                     \
        if (NErrorDevice::GConsoleWindow)                                 \
        {                                                                 \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::Green); \
            NErrorDevice::GConsoleWindow->Print(String(Message) + "\n");  \
            NErrorDevice::GConsoleWindow->SetColor(EConsoleColor::White); \
        }                                                                 \
    } while (0)
