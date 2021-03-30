#pragma once
#include "Core/Application/Generic/GenericOutputConsole.h"
#include "Core/Threading/Platform/Mutex.h"

#include "Windows/Windows.h"

class WindowsOutputConsole : public GenericOutputConsole
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

    Mutex ConsoleMutex;
};