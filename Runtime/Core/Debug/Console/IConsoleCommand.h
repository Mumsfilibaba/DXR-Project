#pragma once
#include "IConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"

DECLARE_MULTICAST_DELEGATE(FCommandDelegate);

struct IConsoleCommand 
    : public IConsoleObject
{
    // TODO: Add parameters to console commands

    /**
     * @brief - Execute the ConsoleCommand
     */
    virtual void Execute() = 0;

    /**
     * @brief  - Retrieve the delegate that gets broadcasted when the console executes the command 
     * @return - Returns the execute delegate
     */
    virtual FCommandDelegate& GetDelgate() = 0;
};