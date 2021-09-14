#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Application/Generic/GenericOutputConsole.h"

#if defined(__OBJC__)
@class CCocoaConsoleWindow;
#else
class CCocoaConsoleWindow;
#endif

class CMacOutputConsole : public CGenericOutputConsole
{
public:
	CMacOutputConsole();
    ~CMacOutputConsole();

    virtual void Print( const std::string& Message ) override final;

    virtual void Clear() override final;

    virtual void SetTitle( const std::string& Title ) override final;
    virtual void SetColor( EConsoleColor Color )      override final;

private:
	CCocoaConsoleWindow* Window;
};

#endif
