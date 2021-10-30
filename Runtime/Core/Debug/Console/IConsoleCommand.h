#pragma once
#include "IConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"

DECLARE_MULTICAST_DELEGATE( CExecutedDelegate );

class IConsoleCommand : public IConsoleObject
{
public:

    // TODO: Add parameter-list
    virtual void Execute() = 0;

    /* Retrieve the delegate that gets broadcasted when the console executes the command */
    virtual CExecutedDelegate& GetExecutedDelgate() = 0;
};