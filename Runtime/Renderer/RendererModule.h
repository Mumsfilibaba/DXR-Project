#pragma once
#include "Core/Modules/ModuleManager.h"

struct RENDERER_API FRendererModule
    : public FModuleInterface
{
    virtual bool Load() override final;
};