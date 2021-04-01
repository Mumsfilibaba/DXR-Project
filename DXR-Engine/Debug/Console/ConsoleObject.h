#pragma once
#include "Core.h"

#include <string>

class ConsoleVariable;
class ConsoleCommand;

using String = std::string;

class ConsoleObject
{
public:
    virtual ~ConsoleObject() = default;

    virtual ConsoleCommand*  AsCommand()  { return nullptr; }
    virtual ConsoleVariable* AsVariable() { return nullptr; }
};