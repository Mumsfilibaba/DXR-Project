#pragma once
#include "Core.h"

class ConsoleVariable;
class ConsoleCommand;

class ConsoleObject
{
public:
    virtual ~ConsoleObject() = default;

    virtual ConsoleCommand*  AsCommand()  { return nullptr; }
    virtual ConsoleVariable* AsVariable() { return nullptr; }
};