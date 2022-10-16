#pragma once
#include "IConsoleCommand.h"

#include "Core/Delegates/MulticastDelegate.h"

class FConsoleCommand 
    : public IConsoleCommand
{
public:
    FConsoleCommand()
        : ExecuteDelegate()
    { }

    FConsoleCommand(const FCommandDelegateType& Delegate)
        : ExecuteDelegate()
    {
        ExecuteDelegate.Add(Delegate);
    }

    virtual ~FConsoleCommand() = default;

public:

    virtual IConsoleCommand*  AsCommand()  override { return this; }
    virtual IConsoleVariable* AsVariable() override { return nullptr; }

    // TODO: Add parameters to console commands
    virtual void Execute() override
    {
        ExecuteDelegate.Broadcast();
    }

    virtual FCommandDelegate& GetDelgate() override
    {
        return ExecuteDelegate;
    }

private:
    FCommandDelegate ExecuteDelegate;
};