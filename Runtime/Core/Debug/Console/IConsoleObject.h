#pragma once
#include "Core/Core.h"

struct IConsoleObject
{
    virtual ~IConsoleObject() = default;

    /**
     * @brief  - Cast to a console-variable if the console-variable interface is implemented
     * @return - Returns either a console-variable or nullptr
     */
    virtual struct IConsoleVariable* AsVariable() = 0;

    /**
     * @brief  - Cast to a console-command if the console-command interface is implemented
     * @return - Returns either a console-command or nullptr
     */
    virtual struct IConsoleCommand* AsCommand() = 0;
};
