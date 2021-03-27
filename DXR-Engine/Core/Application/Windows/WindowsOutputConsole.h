#pragma once
#include "Core/Application/Generic/OutputConsole.h"

#include "Windows/Windows.h"

class WindowsOutputConsole : public OutputConsole
{
public:
    WindowsOutputConsole();
    ~WindowsOutputConsole();

    virtual void Print(const std::string& Message) override final;
    
    virtual void Clear() override final;

    virtual void SetTitle(const std::string& Title) override final;
    virtual void SetColor(EConsoleColor Color)      override final;

private:
    HANDLE ConsoleHandle;
};