#pragma once
#include "IConsoleObject.h"

#include "Core/Delegates/MulticastDelegate.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Delegate to be executed when the console-command gets executed

DECLARE_MULTICAST_DELEGATE(CExecutedDelegate);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-Command interface 

class IConsoleCommand : public IConsoleObject
{
public:

    // TODO: Add parameters to console commands

    /**
     * Execute the ConsoleCommand
     */
    virtual void Execute() = 0;

    /**
     * Retrieve the delegate that gets broadcasted when the console executes the command 
     * 
     * @return: Returns the execute delegate
     */
    virtual CExecutedDelegate& GetExecutedDelgate() = 0;
};