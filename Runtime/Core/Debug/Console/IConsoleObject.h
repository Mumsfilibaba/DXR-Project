#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Console-object interface */

class IConsoleObject
{
public:

    virtual ~IConsoleObject() = default;

    /**
     * Cast to a console-variable if the console-variable interface is implemented
     * 
     * @return: Returns either a console-variable or nullptr
     */
    virtual class IConsoleVariable* AsVariable() = 0;

    /**
     * Cast to a console-command if the console-command interface is implemented
     *
     * @return: Returns either a console-command or nullptr
     */
    virtual class IConsoleCommand* AsCommand() = 0;
};
