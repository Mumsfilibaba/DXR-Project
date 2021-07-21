#pragma once
#include "ConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"

class ConsoleCommand : public ConsoleObject
{
public:
    virtual ConsoleCommand* AsCommand() override
    {
        return this;
    }

    void Execute()
    {
        OnExecute.Broadcast();
    }

    TMulticastDelegate<void> OnExecute;
};