#include "ModuleInterface.h"
#include "ModuleManager.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FModuleInterface

static auto& GetModuleManagerInstance()
{
    static TOptional<FModuleManager> GInstance(InPlace);
    return GInstance;
}

FModuleInterface& FModuleInterface::Get()
{
    auto& ModuleManager = GetModuleManagerInstance();
    return ModuleManager.GetValue();
}

void FModuleInterface::ReleaseAllLoadedModules()
{
    auto& ModuleManager = GetModuleManagerInstance();
    ModuleManager->ReleaseAllModules();
}

void FModuleInterface::Shutdown()
{
    ReleaseAllLoadedModules();

    auto& ModuleManager = GetModuleManagerInstance();
    ModuleManager.Reset();
}
