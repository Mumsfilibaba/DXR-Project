#include "ModuleManager.h"

IModule* FModuleManager::LoadModule(const CHAR* ModuleName)
{
    Check(ModuleName != nullptr);

    if (IModule* ExistingModule = GetModule(ModuleName))
    {
        LOG_WARNING("Module '%s' is already loaded", ModuleName);
        return ExistingModule;
    }

    FModuleData NewModule;
    if (FInitializeStaticModuleDelegate* ModuleInitializer = GetStaticModuleDelegate(ModuleName))
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
        TScopedLock Lock(ModulesCS);

        LOG_INFO("Loaded module '%s'", ModuleName);

        NewModule.Name = ModuleName;
        Modules.Emplace(NewModule);
        
        HandleModuleLoaded(ModuleName, NewModule.Interface);
        return NewModule.Interface;
    }
    else
    {
        LOG_ERROR("Failed to load module '%s'", ModuleName);
        SAFE_DELETE(NewModule.Interface);
        return nullptr;
    }
}

IModule* FModuleManager::GetModule(const CHAR* ModuleName)
{
    TScopedLock Lock(ModulesCS);
    
    const int32 Index = GetModuleIndexUnlocked(ModuleName);
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
    TScopedLock Lock(ModulesCS);

    const int32 NumModules = Modules.GetSize();
    for (int32 Index = 0; Index < NumModules; Index++)
    {
        FModuleData& Module = Modules[Index];
        if (IModule* EngineModule = Module.Interface)
        {
            EngineModule->Unload();
            SAFE_DELETE(EngineModule);
        }

        FPlatformLibrary::FreeDynamicLib(Module.Handle);
        Module.Handle = nullptr;
    }

    Modules.Clear();
}

PlatformModule FModuleManager::GetModuleHandle(const CHAR* ModuleName)
{
    TScopedLock Lock(ModulesCS);
    
    const int32 Index = GetModuleIndexUnlocked(ModuleName);
    if (Index >= 0)
    {
        return Modules[Index].Handle;
    }
    else
    {
        return nullptr;
    }
}

void FModuleManager::RegisterStaticModule(const CHAR* ModuleName, FInitializeStaticModuleDelegate InitDelegate)
{
    TScopedLock Lock(StaticModuleDelegatesCS);

    const bool bContains = StaticModuleDelegates.ContainsWithPredicate([=](const FStaticModulePair& Element)
    {
        return (Element.First == ModuleName);
    });

    if (!bContains)
    {
        StaticModuleDelegates.Emplace(ModuleName, InitDelegate);
    }
}

bool FModuleManager::IsModuleLoaded(const CHAR* ModuleName)
{
    const int32 Index = GetModuleIndexUnlocked(ModuleName);
    return (Index >= 0);
}

void FModuleManager::UnloadModule(const CHAR* ModuleName)
{
    TScopedLock Lock(ModulesCS);

    const int32 Index = GetModuleIndexUnlocked(ModuleName);
    if (Index >= 0)
    {
        FModuleData& Module = Modules[Index];
        if (IModule* EngineModule = Module.Interface)
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
    TScopedLock Lock(ModulesCS);
    return GetLoadedModuleCountUnlocked();
}

int32 FModuleManager::GetModuleIndexUnlocked(const CHAR* ModuleName)
{
    const auto Index = Modules.FindWithPredicate([=](const FModuleData& Element)
    {
        return (Element.Name == ModuleName);
    });

    return static_cast<int32>(Index);
}

FModuleManager::FInitializeStaticModuleDelegate* FModuleManager::GetStaticModuleDelegate(const CHAR* ModuleName)
{
    TScopedLock Lock(StaticModuleDelegatesCS);

    const auto Index = StaticModuleDelegates.FindWithPredicate([=](const FStaticModulePair& Element)
    {
        return (Element.First == ModuleName);
    });

    if (StaticModuleDelegates.IsValidIndex(Index))
    {
        FInitializeStaticModuleDelegate& ModuleInitializer = StaticModuleDelegates[Index].Second;
        return &ModuleInitializer;
    }
    else
    {
        return nullptr;
    }
}