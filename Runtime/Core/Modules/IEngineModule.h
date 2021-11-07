#pragma once
#include "Core/CoreModule.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

#define IMPLEMENT_ENGINE_MODULE( ModuleClassType )  \
extern "C"                                          \
{                                                   \
    MODULE_EXPORT IEngineModule* LoadEngineModule() \
    {                                               \
        return dbg_new ModuleClassType();           \
    }                                               \
}

///////////////////////////////////////////////////////////////////////////////////////////////////

/* Interface that all engine modules must implement */
class IEngineModule
{
public:

    virtual ~IEngineModule() = default;

    /* Called when the module is first loaded into the application */
    virtual bool Load() = 0;

    /* Called before the module is unloaded by the application */
    virtual bool Unload() = 0;

    /* The name of the module */
    virtual const char* GetName() const = 0;
};

typedef IEngineModule* (*PFNLoadEngineModule)();

///////////////////////////////////////////////////////////////////////////////////////////////////

/* Default EngineModule that implements empty Load and Unload functions for modules that do not require these */
class CDefaultEngineModule : public IEngineModule
{
public:

    /* Called when the module is first loaded into the application */
    virtual bool Load() override
    {
        return true;
    }

    /* Called before the module is unloaded by the application */
    virtual bool Unload() override
    {
        return true;
    }
};
