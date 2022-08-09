#include "ModuleManager.h"

#include "Core/Templates/StringUtils.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FModuleManager

static auto& GetModuleManagerInstance()
{
    static TOptional<FModuleManager> Instance(InPlace);
    return Instance;
}

FModuleManager& FModuleManager::Get()
{
    auto& ModuleManager = GetModuleManagerInstance();
    return ModuleManager.GetValue();
}

void FModuleManager::ReleaseAllLoadedModules()
{
    auto& ModuleManager = GetModuleManagerInstance();
    ModuleManager->ReleaseAllModules();
}

void FModuleManager::Shutdown()
{
    ReleaseAllLoadedModules();

    auto& ModuleManager = GetModuleManagerInstance();
    ModuleManager.Reset();
}

IModule* FModuleManager::LoadModule(const char* ModuleName)
{
    Check(ModuleName != nullptr);

    IModule* ExistingModule = GetModule(ModuleName);
    if (ExistingModule)
    {
        LOG_WARNING("Module '%s' is already loaded", ModuleName);
        return ExistingModule;
    }

    FModule NewModule;

    FInitializeStaticModuleDelegate* ModuleInitializer = GetStaticModuleDelegate(ModuleName);
    if (ModuleInitializer)
    {
        if (ModuleInitializer->IsBound())
        {
            NewModule.Interface = ModuleInitializer->Execute();
            if (!NewModule.Interface)
            {
                LOG_ERROR("Failed to load static module '%s'", ModuleName);
                return nullptr;
            }
        }
        else
        {
            LOG_ERROR("No initializer bound when trying to load module '%s'", ModuleName);
            return nullptr;
        }
    }
    else
    {
        PlatformModule Module = FPlatformLibrary::LoadDynamicLib(ModuleName);
        if (!Module)
        {
            LOG_ERROR("Failed to find module '%s'", ModuleName);
            return nullptr;
        }

        PFNLoadEngineModule LoadEngineModule = FPlatformLibrary::LoadSymbolAddress<PFNLoadEngineModule>("LoadEngineModule", Module);
        if (!LoadEngineModule)
        {
            FPlatformLibrary::FreeDynamicLib(Module);
            return nullptr;
        }

        NewModule.Interface = LoadEngineModule();
        if (!NewModule.Interface)
        {
            LOG_ERROR("Failed to load module '%s', resulting interface was nullptr", ModuleName);
            FPlatformLibrary::FreeDynamicLib(Module);

            return nullptr;
        }
        else
        {
            NewModule.Handle = Module;
        }
    }

    if (NewModule.Interface->Load())
    {
        LOG_INFO("Loaded module '%s'", ModuleName);

        ModuleLoadedDelegate.Broadcast(ModuleName, NewModule.Interface);

        NewModule.Name = ModuleName;
        Modules.Emplace(NewModule);
        return NewModule.Interface;
    }
    else
    {
        LOG_ERROR("Failed to load module '%s'", ModuleName);
        SAFE_DELETE(NewModule.Interface);
        return nullptr;
    }
}

IModule* FModuleManager::GetModule(const char* ModuleName)
{
    const int32 Index = GetModuleIndex(ModuleName);
    if (Index >= 0)
    {
        IModule* EngineModule = Modules[Index].Interface;
        if (EngineModule)
        {
            LOG_WARNING("Module is loaded but does not contain an EngineModule interface");
            return nullptr;
        }
        else
        {
            return EngineModule;
        }
    }
    else
    {
        return nullptr;
    }
}

void FModuleManager::ReleaseAllModules()
{
    const int32 NumModules = Modules.GetSize();
    for (int32 Index = 0; Index < NumModules; Index++)
    {
        FModule& Module = Modules[Index];

        IModule* EngineModule = Module.Interface;
        if (EngineModule)
        {
            EngineModule->Unload();
            SAFE_DELETE(EngineModule);
        }

        FPlatformLibrary::FreeDynamicLib(Module.Handle);
        Module.Handle = nullptr;
    }

    Modules.Clear();
}

PlatformModule FModuleManager::GetModuleHandle(const char* ModuleName)
{
    const int32 Index = GetModuleIndex(ModuleName);
    if (Index >= 0)
    {
        return Modules[Index].Handle;
    }
    else
    {
        return nullptr;
    }
}

void FModuleManager::RegisterStaticModule(const char* ModuleName, FInitializeStaticModuleDelegate InitDelegate)
{
    const bool bContains = StaticModuleDelegates.Contains([=](const TPair<FString, FInitializeStaticModuleDelegate>& Element)
    {
        return (Element.First == ModuleName);
    });

    if (!bContains)
    {
        StaticModuleDelegates.Emplace(ModuleName, InitDelegate);
    }
}

bool FModuleManager::IsModuleLoaded(const char* ModuleName)
{
    const int32 Index = GetModuleIndex(ModuleName);
    return (Index >= 0);
}

void FModuleManager::UnloadModule(const char* ModuleName)
{
    const int32 Index = GetModuleIndex(ModuleName);
    if (Index >= 0)
    {
        FModule& Module = Modules[Index];

        IModule* EngineModule = Module.Interface;
        if (EngineModule)
        {
            EngineModule->Unload();
            SAFE_DELETE(EngineModule);
        }

        FPlatformLibrary::FreeDynamicLib(Module.Handle);
        Module.Handle = nullptr;

        Modules.RemoveAt(Index);
    }
}

uint32 FModuleManager::GetLoadedModuleCount()
{
    return static_cast<uint32>(Modules.GetSize());
}

int32 FModuleManager::GetModuleIndex(const char* ModuleName)
{
    const int32 Index = Modules.Find([=](const FModule& Element)
    {
        return (Element.Name == ModuleName);
    });

    // Return explicit -1 in case TArray changes in the future
    return (Index >= 0) ? Index : -1;
}

FModuleManager::FInitializeStaticModuleDelegate* FModuleManager::GetStaticModuleDelegate(const char* ModuleName)
{
    const int32 Index = StaticModuleDelegates.Find([=](const TPair<FString, FInitializeStaticModuleDelegate>& Element)
    {
        return (Element.First == ModuleName);
    });

    if (Index >= 0)
    {
        FInitializeStaticModuleDelegate& ModuleInitializer = StaticModuleDelegates[Index].Second;
        return &ModuleInitializer;
    }
    else
    {
        return nullptr;
    }
}