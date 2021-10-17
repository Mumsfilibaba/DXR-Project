#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Windows/Windows.h"

#include "Core/CoreAPI.h"
#include "Core/Application/Core/CoreOutputConsole.h"
#include "Core/Threading/Platform/CriticalSection.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4275) // Non DLL-interface class used '...' as base for DLL-interface class '...'
#pragma warning(disable : 4251) // Class '...' needs to have DLL-interface to be used by clients of class '...'
#endif

class CORE_API CWindowsOutputConsole final : public CCoreOutputConsole
{
public:

    /* Creates a new console, can only be called once */
    static CWindowsOutputConsole* Make();

    virtual void Print( const CString& Message ) override final;
    virtual void PrintLine( const CString& Message ) override final;

    virtual void Clear() override final;

    virtual void SetTitle( const CString& Title ) override final;
    virtual void SetColor( EConsoleColor Color )  override final;

private:

    CWindowsOutputConsole();
    ~CWindowsOutputConsole();

    /* Handle to the console window */
    HANDLE ConsoleHandle;

    /* Mutex protecting for errors when printing from multiple threads */
    CCriticalSection ConsoleMutex;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)
#endif

#endif