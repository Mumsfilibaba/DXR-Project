#pragma once
#include "IConsoleCommand.h"

#include "Core/Delegates/MulticastDelegate.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CConsoleCommand - Executes a single function from the console

class CConsoleCommand : public IConsoleCommand
{
public:

    CConsoleCommand()
        : ExecuteDelegate()
    { }

    CConsoleCommand(const CExecutedDelegateType& Delegate)
        : ExecuteDelegate()
    {
        ExecuteDelegate.Add(Delegate);
    }

    virtual ~CConsoleCommand() = default;

    virtual IConsoleCommand*  AsCommand() override { return this; }
    virtual IConsoleVariable* AsVariable() override { return nullptr; }

    // TODO: Add parameters to console commands
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