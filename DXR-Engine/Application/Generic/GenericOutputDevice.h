#pragma once
#include "Core.h"

enum class EConsoleColor : UInt8
{
    Red    = 0,
    Green  = 1,
    Yellow = 2,
    White  = 3
};

class GenericOutputDevice
{
public:
    virtual ~GenericOutputDevice() = default;

    virtual void Print(const std::string& Message) = 0;
    
    virtual void Clear() = 0;

    virtual void SetTitle(const std::string& Title) = 0;
    virtual void SetColor(EConsoleColor Color)      = 0;

    static GenericOutputDevice* Make()
    {
        return nullptr;
    }
};