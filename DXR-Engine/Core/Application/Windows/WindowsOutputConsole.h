#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Core/CoreOutputConsole.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "Core/Windows/Windows.h"

class CWindowsOutputConsole final : public CCoreOutputConsole
{
public:

    /* Creates a new console, can only be called once */
    static FORCEINLINE CWindowsOutputConsole* Make()
    {
        return new CWindowsOutputConsole();
    }

    virtual void Print( const CString& Message )     override final;
    virtual void PrintLine( const CString& Message ) override final;

    virtual void Clear()         override final;
    virtual void ClearLastLine() override final;

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

#endif