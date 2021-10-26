#pragma once
#include "IConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"

class CORE_API CConsoleCommand : public IConsoleCommand
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

    virtual IConsoleVariable* AsVariable() override
    {
        return nullptr;
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