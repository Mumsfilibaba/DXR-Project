#pragma once
#include "ConsoleObject.h"

#include "Core/Delegates/Event.h"

class ConsoleCommand : public ConsoleObject
{
public:
    virtual ConsoleCommand* AsCommand() override { return this; }

    void Execute()
    {
        OnExecute.Broadcast();
    }

    DECLARE_EVENT(OnExecuteEvent, ConsoleCommand);
    OnExecuteEvent OnExecute;
};