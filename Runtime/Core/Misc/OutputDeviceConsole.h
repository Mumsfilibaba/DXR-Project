#pragma once
#include "OutputDevice.h"

#include "Core/Containers/String.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EConsoleColor

enum class EConsoleColor : uint8
{
    Red    = 0,
    Green  = 1,
    Yellow = 2,
    White  = 3
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FOutputDeviceConsole

struct CORE_API FOutputDeviceConsole
    : public FOutputDevice
{
    FOutputDeviceConsole()          = default;
    virtual ~FOutputDeviceConsole() = default;

    virtual void Show(bool bShow)  = 0;
    virtual bool IsVisible() const = 0;

    virtual void SetTitle(const FString& Title) = 0;
    virtual void SetTextColor(EConsoleColor Color)  = 0;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif

