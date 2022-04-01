#include "ModuleManager.h"

#include "Core/Windows/Windows.h"
#include "Core/Templates/StringMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ModuleManager

IEngineModule* CModuleManager::LoadEngineModule(const char* ModuleName)
{
    Check(ModuleName != nullptr);

    IEngineModule* ExistingModule = GetEngineModule(ModuleName);
    if (ExistingModule)
    {
        LOG_WARNING("Module '" + String(ModuleName) + "' is already loaded");
        return ExistingModule;
    }

    SModule NewModule;

    CInitializeStaticModuleDelegate* ModuleInitializer = GetStaticModuleDelegate(ModuleName);
    if (ModuleInitializer)
    {
        if (ModuleInitializer->IsBound())
        {
            NewModule.Interface = ModuleInitializer->Execute();
            if (!NewModule.Interface)
            {
                LOG_ERROR("Failed to load static module '" + String(ModuleName) + "'");
                return nullptr;
            }
        }
        else
        {
            LOG_ERROR("No initializer bound when trying to load module '" + String(ModuleName) + "'");
            return nullptr;
        }
    }
    else
    {
        PlatformModule Module = PlatformLibrary::LoadDynamicLib(ModuleName);
        if (!Module)
        {
            LOG_ERROR("Failed to find module '" + String(ModuleName) + "'");
            return nullptr;
        }

        PFNLoadEngineModule LoadEngineModule = PlatformLibrary::LoadSymbolAddress<PFNLoadEngineModule>("LoadEngineModule", Module);
        if (!LoadEngineModule)
        {
            PlatformLibrary::FreeDynamicLib(Module);
            return nullptr;
        }

        NewModule.Interface = LoadEngineModule();
        if (!NewModule.Interface)
        {
            LOG_ERROR("Failed to load module '" + String(ModuleName) + "', resulting interface was nullptr");
            PlatformLibrary::FreeDynamicLib(Module);

            return nullptr;
        }
        else
        {
            NewModule.Handle = Module;
        }
    }

    if (NewModule.Interface->Load())
    {
        LOG_INFO("Loaded module '" + String(ModuleName) + "'");

        ModuleLoadedDelegate.Broadcast(ModuleName, NewModule.Interface);

        NewModule.Name = ModuleName;
        Modules.Emplace(NewModule);
        return NewModule.Interface;
    }
    else
    {
        LOG_ERROR("Failed to load module '" + String(ModuleName) + "'");
        SafeDelete(NewModule.Interface);

        return nullptr;
    }
}

IEngineModule* CModuleManager::GetEngineModule(const char* ModuleName)
{
    const int32 Index = GetModuleIndex(ModuleName);
    if (Index >= 0)
    {
        const SModule& Module = Modules[Index];

        IEngineModule* EngineModule = Module.Interface;
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

CModuleManager& CModuleManager::Get()
{
    TOptional<CModuleManager>& ModuleManager = GetModuleManagerInstance();
    return ModuleManager.GetValue();
}

void CModuleManager::ReleaseAllLoadedModules()
{
    TOptional<CModuleManager>& ModuleManager = GetModuleManagerInstance();
    ModuleManager->ReleaseAllModules();
}

void CModuleManager::Destroy()
{
    TOptional<CModuleManager>& ModuleManager = GetModuleManagerInstance();
    ModuleManager.Reset();
}

void  CModuleManager::ReleaseAllModules()
{
    const int32 NumModules = Modules.Size();
    for (int32 Index = 0; Index < NumModules; Index++)
    {
        SModule& Module = Modules[Index];

        IEngineModule* EngineModule = Module.Interface;
        if (EngineModule)
        {
            EngineModule->Unload();
            SafeDelete(EngineModule);
        }

        PlatformModule Handle = Module.Handle;
        PlatformLibrary::FreeDynamicLib(Handle);
        Module.Handle = nullptr;
    }

    Modules.Clear();
}

PlatformModule CModuleManager::GetModuleHandle(const char* ModuleName)
{
    const int32 Index = GetModuleIndex(ModuleName);
    if (Index >= 0)
    {
        const SModule& Module = Modules[Index];
        return Module.Handle;
    }
    else
    {
        return nullptr;
    }
}

void CModuleManager::RegisterStaticModule(const char* ModuleName, CInitializeStaticModuleDelegate InitDelegate)
{
    const bool bContains = StaticModuleDelegates.Contains([=](const TPair<String, CInitializeStaticModuleDelegate>& Element)
    {
        return (Element.First == ModuleName);
    });

    if (!bContains)
    {
        StaticModuleDelegates.Emplace(ModuleName, InitDelegate);
    }
}

bool CModuleManager::IsModuleLoaded(const char* ModuleName)
{
    const int32 Index = GetModuleIndex(ModuleName);
    return (Index >= 0);
}

void CModuleManager::UnloadModule(const char* ModuleName)
{
    const int32 Index = GetModuleIndex(ModuleName);
    if (Index >= 0)
    {
        SModule& Module = Modules[Index];

        IEngineModule* EngineModule = Module.Interface;
        if (EngineModule)
        {
            EngineModule->Unload();
            SafeDelete(EngineModule);
        }

        PlatformModule Handle = Module.Handle;
        PlatformLibrary::FreeDynamicLib(Handle);
        Module.Handle = nullptr;

        Modules.RemoveAt(Index);
    }
}

uint32 CModuleManager::GetLoadedModuleCount()
{
    return static_cast<uint32>(Modules.Size());
}

TOptional<CModuleManager>& CModuleManager::GetModuleManagerInstance()
{
    static TOptional<CModuleManager> Instance(InPlace);
    return Instance;
}

int32 CModuleManager::GetModuleIndex(const char* ModuleName)
{
    const int32 Index = Modules.Find([=](const SModule& Element)
    {
        return (Element.Name == ModuleName);
    });

    // Return explicit -1 in case TArray changes in the future
    return (Index >= 0) ? Index : -1;
}

CModuleManager::CInitializeStaticModuleDelegate* CModuleManager::GetStaticModuleDelegate(const char* ModuleName)
{
    const int32 Index = StaticModuleDelegates.Find([=](const TPair<String, CInitializeStaticModuleDelegate>& Element)
    {
        return (Element.First == ModuleName);
    });

    if (Index >= 0)
    {
        CInitializeStaticModuleDelegate& ModuleInitializer = StaticModuleDelegates[Index].Second;
        return &ModuleInitializer;
    }
    else
    {
        return nullptr;
    }
}
