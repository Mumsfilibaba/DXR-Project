#pragma once
#include "Core/Application/Generic/GenericOutputDevice.h"

#include "Windows/Windows.h"

class WindowsConsoleOutput : public GenericOutputDevice
{
public:
    WindowsConsoleOutput();
    ~WindowsConsoleOutput();

    virtual void Print(const std::string& Message) override final;
    
    virtual void Clear() override final;

    virtual void SetTitle(const std::string& Title) override final;
    virtual void SetColor(EConsoleColor Color)      override final;

    static GenericOutputDevice* Create();

private:
    HANDLE ConsoleHandle;
};