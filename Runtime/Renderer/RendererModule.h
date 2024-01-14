#pragma once
#include "Core/Modules/ModuleManager.h"

class RENDERER_API FRendererModule : public FModuleInterface
{
public:
    virtual bool Load() override final;

private:
    FDelegateHandle PostApplicationCreateHandle;
};