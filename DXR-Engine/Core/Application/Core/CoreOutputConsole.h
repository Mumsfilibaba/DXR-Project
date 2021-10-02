#pragma once
#include "Core.h"

#include "Core/Containers/String.h"

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

    virtual void Print( const CString& Message ) = 0;
    virtual void PrintLine( const CString& Message ) = 0;

    virtual void Clear() = 0;
    virtual void ClearLastLine() = 0;

    virtual void SetTitle( const CString& Title ) = 0;
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
