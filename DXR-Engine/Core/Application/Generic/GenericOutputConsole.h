#pragma once
#include "Core.h"

enum class EConsoleColor : uint8
{
    Red = 0,
    Green = 1,
    Yellow = 2,
    White = 3
};

class GenericOutputConsole
{
public:
    virtual ~GenericOutputConsole() = default;

    virtual void Print( const std::string& Message ) = 0;

    virtual void Clear() = 0;

    virtual void SetTitle( const std::string& Title ) = 0;
    virtual void SetColor( EConsoleColor Color ) = 0;

    static GenericOutputConsole* Create();
};

extern GenericOutputConsole* GConsoleOutput;
