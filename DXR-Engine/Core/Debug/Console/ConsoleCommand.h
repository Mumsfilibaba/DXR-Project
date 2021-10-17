#pragma once
#include "ConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"

class CORE_API CConsoleCommand : public CConsoleObject, public IConsoleCommand
{
public:

    CConsoleCommand()
        : ExecuteDelegate()
    {
    }

    virtual ~CConsoleCommand() = default;

    virtual IConsoleCommand* AsCommand() override
    {
        return this;
    }

    virtual void Execute() override 
    {
        ExecuteDelegate.Broadcast();
    }

    virtual CExecutedDelegate& GetExecutedDelgate() override
    {
        return ExecuteDelegate;
    }

private:
    CExecutedDelegate ExecuteDelegate;
};