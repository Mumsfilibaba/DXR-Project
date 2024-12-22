#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Generic/GenericConsoleOutputDevice.h"

/**
 * @class FWindowsConsoleOutputDevice
 * @brief A Windows-specific implementation of a generic console output device.
 *
 * This class provides console output functionality on Windows platforms. It can show/hide the console
 * window, change its title, set text colors, and log messages with varying severity.
 * Output is synchronized via a critical section to ensure thread-safe writes.
 */
class COREAPPLICATION_API FWindowsConsoleOutputDevice final : public FGenericConsoleOutputDevice
{
public:

    /**
     * @brief Creates a new instance of FWindowsConsoleOutputDevice and returns it as a FGenericConsoleOutputDevice.
     * 
     * This static method simplifies the creation process and may handle additional initialization if required.
     * 
     * @return A pointer to the newly created FGenericConsoleOutputDevice instance.
     */
    static FGenericConsoleOutputDevice* Create();

public:

    /** @brief Destructor */
    virtual ~FWindowsConsoleOutputDevice();

public:

    // FGenericConsoleOutputDevice Interface
    virtual void Show(bool bShow) override final;

    virtual bool IsVisible() const override final { return (ConsoleHandle != nullptr); }

    virtual void Log(const FString& Message) override final;

    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

    virtual void Flush() override final;

    virtual void SetTitle(const FString& Title) override final;

    virtual void SetTextColor(EConsoleColor Color) override final;

private:
    FWindowsConsoleOutputDevice();

    FString          Title;
    HANDLE           ConsoleHandle;
    FCriticalSection ConsoleHandleCS;
};
