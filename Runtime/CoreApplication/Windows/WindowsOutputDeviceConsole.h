#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Misc/OutputDeviceConsole.h"

#include "CoreApplication/CoreApplication.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4275) // Non DLL-interface class used '...' as base for DLL-interface class '...'
    #pragma warning(disable : 4251) // Class '...' needs to have DLL-interface to be used by clients of class '...'
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsOutputDeviceConsole

class COREAPPLICATION_API FWindowsOutputDeviceConsole final 
    : public FOutputDeviceConsole
{
public:
    FWindowsOutputDeviceConsole();
    ~FWindowsOutputDeviceConsole();

    virtual void Show(bool bShow) override final;

    virtual bool IsVisible() const override final { return (ConsoleHandle != nullptr); }

    virtual void Log(const FString& Message) override final;
    
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

    virtual void Flush() override final;

    virtual void SetTitle(const FString& Title) override final;

    virtual void SetTextColor(EConsoleColor Color) override final;

private:
    FString          Title;
    
    HANDLE           ConsoleHandle;
    FCriticalSection ConsoleHandleCS;

};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#endif
