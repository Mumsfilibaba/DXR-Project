#pragma once

#if PLATFORM_WINDOWS
#include "Core/Windows/Windows.h"

#include "Core/Core.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Interface/PlatformConsoleWindow.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4275) // Non DLL-interface class used '...' as base for DLL-interface class '...'
#pragma warning(disable : 4251) // Class '...' needs to have DLL-interface to be used by clients of class '...'
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows-specific interface for ConsoleWindow

class COREAPPLICATION_API CWindowsConsoleWindow final : public CPlatformConsoleWindow
{
public:

    /* Creates a new console, can only be called once */
    static CWindowsConsoleWindow* Make();

    virtual void Print(const CString& Message) override final;
    virtual void PrintLine(const CString& Message) override final;

    virtual void Clear() override final;

    virtual void SetTitle(const CString& Title) override final;
    virtual void SetColor(EConsoleColor Color)  override final;

private:

    CWindowsConsoleWindow();
    ~CWindowsConsoleWindow();

    /* Handle to the console window */
    HANDLE ConsoleHandle;

    /* Mutex protecting for errors when printing from multiple threads */
    CCriticalSection ConsoleMutex;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)
#endif

#endif