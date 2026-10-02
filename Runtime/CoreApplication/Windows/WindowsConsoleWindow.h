#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/PlatformInterface/IPlatformConsoleWindow.h"

class COREAPPLICATION_API FWindowsConsoleWindow final : public IPlatformConsoleWindow
{
public:
    static IPlatformConsoleWindow* Create();

public:
    virtual ~FWindowsConsoleWindow();

    // IPlatformConsoleWindow Interface
    virtual void Show(bool bShow) override final;
    virtual bool IsVisible() const override final { return (ConsoleHandle != nullptr); }

    virtual void SetTitle(const String& Title) override final;
    virtual void SetTextColor(EConsoleTextColor Color) override final;
    virtual void SetOnClosed(const TFunction<void()>& Callback) override final;

    // IOutputDevice Interface
    virtual void Log(const String& Message) override final;
    virtual void Log(ELogSeverity Severity, const String& Message) override final;
    virtual void Flush() override final;

private:
    static BOOL WINAPI ConsoleCtrlHandler(DWORD Type);

    FWindowsConsoleWindow();

    bool NotifyClosed();
    void Write(const CHAR* Data, uint32 Length);

    String            Title;
    HANDLE            ConsoleHandle;
    FCriticalSection  ConsoleHandleCS;
    TFunction<void()> OnClosed;
};
