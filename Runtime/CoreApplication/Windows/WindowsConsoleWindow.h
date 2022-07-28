#pragma once
#include "Core/Windows/Windows.h"

#include "Core/Core.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Generic/GenericConsoleWindow.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4275) // Non DLL-interface class used '...' as base for DLL-interface class '...'
    #pragma warning(disable : 4251) // Class '...' needs to have DLL-interface to be used by clients of class '...'
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsConsoleWindow

class COREAPPLICATION_API FWindowsConsoleWindow final 
    : public FGenericConsoleWindow
{
private:
    FWindowsConsoleWindow();
    ~FWindowsConsoleWindow();

public:
    static FWindowsConsoleWindow* CreateWindowsConsole();

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FWindowsConsoleWindow Interface

    virtual void Print(const FString& Message)     override final;
    virtual void PrintLine(const FString& Message) override final;

    virtual void Clear() override final;

    virtual void SetTitle(const FString& Title) override final;
    virtual void SetColor(EConsoleColor Color)  override final;

private:
    HANDLE           ConsoleHandle;
    FCriticalSection ConsoleHandleCS;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#endif
