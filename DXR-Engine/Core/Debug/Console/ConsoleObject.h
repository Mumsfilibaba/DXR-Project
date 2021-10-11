#pragma once
#include "Core/Containers/String.h"

class CConsoleVariable;
class CConsoleCommand;

class CConsoleObject
{
public:
    virtual ~CConsoleObject() = default;

    virtual CConsoleCommand* AsCommand()
    {
        return nullptr;
    }

    virtual CConsoleVariable* AsVariable()
    {
        return nullptr;
    }
};