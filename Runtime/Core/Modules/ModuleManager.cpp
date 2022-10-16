#include "ModuleManager.h"
#include "Core/Misc/EngineLoopTicker.h"

CORE_API FApplicationModule* GApplicationModule;


// This needs to be initialized with static variable behavior since we don't
// know when FModuleManager::Get() is called from the TStaticModuleInitializers
static auto& GetModuleManagerInstance()
{
    static TOptional<FModuleManager> GInstance(InPlace);
    return GInstance;
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


FModuleInterface* FModuleManager::LoadModule(const CHAR* ModuleName)
{
    CHECK(ModuleName != nullptr);

    if (FModuleInterface* ExistingModule = GetModule(ModuleName))
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
        SCOPED_LOCK(ModulesCS);

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

FModuleInterface* FModuleManager::GetModule(const CHAR* ModuleName)
{
    SCOPED_LOCK(ModulesCS);

    const int32 Index = GetModuleIndexUnlocked(ModuleName);
    if (Index >= 0)
    {
        FModuleInterface* EngineModule = Modules[Index].Interface;
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

PlatformModule FModuleManager::GetModuleHandle(const CHAR* ModuleName)
{
    SCOPED_LOCK(ModulesCS);

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
    SCOPED_LOCK(StaticModuleDelegatesCS);

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
    SCOPED_LOCK(ModulesCS);

    const int32 Index = GetModuleIndexUnlocked(ModuleName);
    if (Index >= 0)
    {
        FModuleData& Module = Modules[Index];
        if (FModuleInterface* EngineModule = Module.Interface)
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
    SCOPED_LOCK(ModulesCS);
    return GetLoadedModuleCountUnlocked();
}

void FModuleManager::ReleaseAllModules()
{
    SCOPED_LOCK(ModulesCS);

    const int32 NumModules = Modules.GetSize();
    for (int32 Index = 0; Index < NumModules; Index++)
    {
        FModuleData& Module = Modules[Index];
        if (FModuleInterface* EngineModule = Module.Interface)
        {
            EngineModule->Unload();
            SAFE_DELETE(EngineModule);
        }

        FPlatformLibrary::FreeDynamicLib(Module.Handle);
        Module.Handle = nullptr;
    }

    Modules.Clear();
}

FModuleManager::FInitializeStaticModuleDelegate* FModuleManager::GetStaticModuleDelegate(const CHAR* ModuleName)
{
    SCOPED_LOCK(StaticModuleDelegatesCS);

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

int32 FModuleManager::GetModuleIndexUnlocked(const CHAR* ModuleName)
{
    const auto Index = Modules.FindWithPredicate([=](const FModuleData& Element)
    {
        return (Element.Name == ModuleName);
    });

    return static_cast<int32>(Index);
}


bool FApplicationModule::Init()
{
    FTickDelegate TickDelegate = FTickDelegate::CreateRaw(this, &FApplicationModule::Tick);
    TickHandle = TickDelegate.GetHandle();

    FEngineLoopTicker::Get().AddDelegate(TickDelegate);
    return true;
}

bool FApplicationModule::Release()
{
    FEngineLoopTicker::Get().RemoveDelegate(TickHandle);
    return true;
}

// TODO: Remove init and release? 
bool FApplicationModule::Load()
{
    return Init();
}

bool FApplicationModule::Unload()
{
    return Release();
}
