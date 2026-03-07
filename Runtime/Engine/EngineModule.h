#pragma once
#include "Core/Modules/ModuleManager.h"

class ENGINE_API FEngineModule : public FModuleInterface
{
public:
    virtual ~FEngineModule();
    virtual bool Load() override final;

private:
    FDelegateHandle PreEngineInitHandle;
};