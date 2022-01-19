#pragma once
#include "IConsoleCommand.h"

#include "Core/Delegates/MulticastDelegate.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ConsoleCommand - Executes a single function from the console

class CConsoleCommand : public IConsoleCommand
{
public:

    CConsoleCommand()
        : ExecuteDelegate()
    {
    }

    virtual ~CConsoleCommand() = default;

    /* Cast to a console command */
    virtual IConsoleCommand*  AsCommand() override { return this; }
    /* Cast to a console variable */
    virtual IConsoleVariable* AsVariable() override { return nullptr; }

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