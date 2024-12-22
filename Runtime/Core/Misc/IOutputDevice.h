#pragma once
#include "Core/Containers/String.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

enum class ELogSeverity
{
    Info    = 1,
    Warning = 2,
    Error   = 3,
};

struct CORE_API IOutputDevice
{
    IOutputDevice() = default;
    virtual ~IOutputDevice() = default;

    /** @brief Log a simple message */
    virtual void Log(const FString& Message) = 0;

    /** @brief Log a message with severity */
    virtual void Log(ELogSeverity Severity, const FString& Message) = 0;
    
    /** @brief Clear the output device */
    virtual void Flush() { }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
