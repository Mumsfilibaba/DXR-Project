#pragma once
#include "Core/Modules/ModuleManager.h"

struct ENGINE_API FEngineModule
    : public FModuleInterface
{
    virtual bool Load() override final;
};