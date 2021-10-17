#pragma once
#include "IConsoleObject.h"

#include "Core/CoreAPI.h"
#include "Core/Containers/String.h"

class CORE_API CConsoleObject : public IConsoleObject
{
public:
    virtual ~CConsoleObject() = default;

    virtual IConsoleCommand* AsCommand() override
    {
        return nullptr;
    }

    virtual IConsoleVariable* AsVariable() override
    {
        return nullptr;
    }
};