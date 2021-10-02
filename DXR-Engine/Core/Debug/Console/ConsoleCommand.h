#pragma once
#include "ConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"

class CConsoleCommand : public CConsoleObject
{
public:

    DECLARE_MULTICAST_DELEGATE( CCommandExecutedDelegate );
    CCommandExecutedDelegate OnExecute;

    virtual CConsoleCommand* AsCommand() override
    {
        return this;
    }

    FORCEINLINE void Execute()
    {
        OnExecute.Broadcast();
    }
};