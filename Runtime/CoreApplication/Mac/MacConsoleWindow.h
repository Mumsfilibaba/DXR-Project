#pragma once

#if PLATFORM_MACOS
#include "CoreApplication/Interface/PlatformConsoleWindow.h"

#if defined(__OBJC__)
@class CCocoaConsoleWindow;
#else
class CCocoaConsoleWindow;
#endif

class CMacConsoleWindow final : public CPlatformConsoleWindow
{
public:

    /* Creates a new console */
    static FORCEINLINE CMacConsoleWindow* Make()
    {
        return new CMacConsoleWindow();
    }

    virtual void Print( const CString& Message )     override final;
    virtual void PrintLine( const CString& Message ) override final;

    virtual void Clear() override final;

    virtual void SetTitle( const CString& Title ) override final;
    virtual void SetColor( EConsoleColor Color )  override final;

private:

    CMacConsoleWindow();
    ~CMacConsoleWindow();

    /* Console window*/
    CCocoaConsoleWindow* Window;
};

#endif
