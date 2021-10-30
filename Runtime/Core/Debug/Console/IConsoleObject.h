#pragma once
#include "Core.h"

class IConsoleObject
{
public:
    virtual class IConsoleVariable* AsVariable() = 0;
    virtual class IConsoleCommand* AsCommand() = 0;
};
