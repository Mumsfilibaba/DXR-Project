#pragma once
#include "OutputDevice.h"

#include "Core/Containers/String.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

enum class EConsoleColor : uint8
{
    Red    = 0,
    Green  = 1,
    Yellow = 2,
    White  = 3
};

struct CORE_API FOutputDeviceConsole : public IOutputDevice
{
    FOutputDeviceConsole()          = default;
    virtual ~FOutputDeviceConsole() = default;

    virtual void Show(bool bShow)  = 0;
    virtual bool IsVisible() const = 0;

    virtual void SetTitle(const FString& Title) = 0;
    virtual void SetTextColor(EConsoleColor Color)  = 0;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

