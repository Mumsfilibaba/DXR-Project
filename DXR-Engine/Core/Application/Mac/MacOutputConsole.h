#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Application/Generic/GenericOutputConsole.h"

#if defined(__OBJC__)
@class CCocoaConsoleWindow;
#else
class CCocoaConsoleWindow;
#endif

class CMacOutputConsole final : public CGenericOutputConsole
{
public:

    /* Creates a new console */
    static FORCEINLINE CMacOutputConsole* Make()
    {
        return new CMacOutputConsole();
    }

    virtual void Print( const std::string& Message )     override final;
    virtual void PrintLine( const std::string& Message ) override final;

    virtual void Clear()         override final;
    virtual void ClearLastLine() override final;

    virtual void SetTitle( const std::string& Title ) override final;
    virtual void SetColor( EConsoleColor Color )      override final;

private:

    CMacOutputConsole();
    ~CMacOutputConsole();

    /* Console window*/
    CCocoaConsoleWindow* Window;
};

#endif
