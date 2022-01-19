#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Base console-object */

class IConsoleObject
{
public:

    virtual ~IConsoleObject() = default;

    /* Cast to a console-variable */
    virtual class IConsoleVariable* AsVariable() = 0;
    /* Cast to a console-command */
    virtual class IConsoleCommand* AsCommand() = 0;
};
