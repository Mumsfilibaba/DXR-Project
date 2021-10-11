#pragma once
#include "Core/CoreAPI.h"

class IEngineModule
{
public:

    /* Called when the module is first loaded into the application */
    virtual bool Load() = 0;

    /* Called before the module is unloaded by the application */
    virtual bool Unload() = 0;

    /* The name of the module */
    virtual const char* GetName() = 0;
};

typedef IEngineModule*(*PFNLoadEngineModule)();