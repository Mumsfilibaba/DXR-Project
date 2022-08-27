#pragma once
#include "Core/Containers/String.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

enum class ELogSeverity
{
    Info    = 1,
    Warning = 2,
    Error   = 3,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FOutputDevice

struct CORE_API FOutputDevice
{
    FOutputDevice()          = default;
    virtual ~FOutputDevice() = default;

    /** @brief: Log a simple message */
    virtual void Log(const FString& Message) = 0;

    /** @brief: Log a message with severity */
    virtual void Log(ELogSeverity Severity, const FString& Message) = 0;
    
    /** @brief: Clear the output device */
    virtual void Flush() = 0;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
