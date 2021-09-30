#pragma once
#include "Core.h"

// TODO: Remove
#include <string>

enum class EConsoleColor : uint8
{
    Red = 0,
    Green = 1,
    Yellow = 2,
    White = 3
};

class CCoreOutputConsole
{
public:

    static FORCEINLINE CCoreOutputConsole* Make()
    {
        return nullptr;
    }

    virtual void Print( const std::string& Message ) = 0;
    virtual void PrintLine( const std::string& Message ) = 0;

    virtual void Clear() = 0;
    virtual void ClearLastLine() = 0;

    virtual void SetTitle( const std::string& Title ) = 0;
    virtual void SetColor( EConsoleColor Color ) = 0;

    virtual void Release()
    {
        delete this;
    }

protected:

    CCoreOutputConsole() = default;
    virtual ~CCoreOutputConsole() = default;
};

extern CCoreOutputConsole* GConsoleOutput;
