#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Generic/GenericConsoleOutputDevice.h"

class COREAPPLICATION_API FWindowsConsoleOutputDevice final : public FGenericConsoleOutputDevice
{
public:
    static FGenericConsoleOutputDevice* Create();

public:
    virtual ~FWindowsConsoleOutputDevice();

    // FGenericConsoleOutputDevice Interface
    virtual void Show(bool bShow) override final;
    virtual bool IsVisible() const override final { return (ConsoleHandle != nullptr); }
    virtual void Log(const String& Message) override final;
    virtual void Log(ELogSeverity Severity, const String& Message) override final;
    virtual void Flush() override final;
    virtual void SetTitle(const String& Title) override final;
    virtual void SetTextColor(EConsoleColor Color) override final;

private:
    FWindowsConsoleOutputDevice();

    String           Title;
    HANDLE           ConsoleHandle;
    FCriticalSection ConsoleHandleCS;
};
