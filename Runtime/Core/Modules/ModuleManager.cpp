#include "Core/Misc/ConsoleManager.h"
#include "Core/Modules/ModuleImplementation.h"
#include "Core/Filesystem/File.h"
#include "Core/Platform/PlatformFile.h"

static TAutoConsoleVariable<bool> CVarModuleManagerBreakOnError(
    TEXT("Core.ModuleManagerBreakOnError"),
    TEXT("Breaks when trying to retrieve a module that is not loaded"),
    true);

// This needs to be initialized with static variable behavior since we don't
// know when FModuleManager::Get() is called from the TStaticModuleInitializers
static TOptional<FModuleManager>& GetModuleManagerInstance()
{
    static TOptional<FModuleManager> StaticModuleManager(EInPlace::InPlace);
    return StaticModuleManager;
}

FModuleManager& FModuleManager::Get()
{
    TOptional<FModuleManager>& ModuleManager = GetModuleManagerInstance();
    return ModuleManager.GetValue();
}

void FModuleManager::ReleaseAllLoadedModules()
{
    TOptional<FModuleManager>& ModuleManager = GetModuleManagerInstance();
    ModuleManager->ReleaseAllModules();
}

void FModuleManager::Shutdown()
{
    ReleaseAllLoadedModules();

    TOptional<FModuleManager>& ModuleManager = GetModuleManagerInstance();
    ModuleManager.Reset();
}


static bool CheckBuildStamp(const CHAR_T* ModuleName, const FString& ModuleFile, FString& OutError)
{
    FString ModuleStamp;
    if (!FPlatformLibrary::ReadStringExport(*ModuleFile, FModuleManager::BuildStampSymbol, ModuleStamp))
    {
        OutError = FString::Printf(TEXT("Module '%s' at '%s' has no build stamp, so it was built before modules had one or not by this engine. Rebuild it with this engine."),
            ModuleName, *ModuleFile);
        return false;
    }

    const FString EngineStamp = FModuleManager::GetBuildStamp();
    if (!ModuleStamp.Equals(EngineStamp))
    {
        OutError = FString::Printf(TEXT("Module '%s' at '%s' was built by another engine build (%s) than this one (%s). Rebuild it with this engine."),
            ModuleName, *ModuleFile, *ModuleStamp, *EngineStamp);
        return false;
    }

    return true;
}

FString FModuleManager::GetBuildStamp()
{
    return StringCast<CHAR_T>(ENGINE_MODULE_BUILD_STAMP).ToString();
}

IModule* FModuleManager::LoadModule(const CHAR_T* ModuleName)
{
    FString Error;
    IModule* Module = LoadModule(ModuleName, Error);
    if (!Module)
    {
        LOG_ERROR("%s", *Error);
    }

    return Module;
}

IModule* FModuleManager::LoadModule(const CHAR_T* ModuleName, FString& OutError)
{
    CHECK(ModuleName != nullptr);

    {
        SCOPED_LOCK(ModulesCS);

        if (IModule* ExistingModule = GetModuleInternal(ModuleName))
        {
            LOG_WARNING("Module '%s' is already loaded", ModuleName);
            return ExistingModule;
        }
    }

    FModuleData NewModule;
    FString     ModuleFile;
    bool        bInBinariesDirectory = false;

    if (FInitializeStaticModuleDelegate* ModuleInitializer = GetStaticModuleDelegate(ModuleName))
    {
        if (ModuleInitializer->IsBound())
        {
            NewModule.Interface = ModuleInitializer->Execute();
            if (!NewModule.Interface)
            {
                OutError = FString::Printf(TEXT("Failed to load static module '%s'"), ModuleName);
                return nullptr;
            }
        }
        else
        {
            OutError = FString::Printf(TEXT("No initializer bound when trying to load module '%s'"), ModuleName);
            return nullptr;
        }
    }
    else
    {
        ModuleFile = FindModuleFile(ModuleName, bInBinariesDirectory);

        // Checked before the module is loaded, since even its static initializers call into this engine
        if (!ModuleFile.IsEmpty() && !CheckBuildStamp(ModuleName, ModuleFile, OutError))
        {
            return nullptr;
        }

        PlatformModule Module = ModuleFile.IsEmpty() ? FPlatformLibrary::LoadDynamicLib(ModuleName) : FPlatformLibrary::LoadDynamicLibFromFile(*ModuleFile);
        if (!Module)
        {
            if (ModuleFile.IsEmpty())
            {
                OutError = FString::Printf(TEXT("Failed to find module '%s'"), ModuleName);
            }
            else
            {
                OutError = FString::Printf(TEXT("Failed to load module '%s' from '%s'"), ModuleName, *ModuleFile);
            }

            return nullptr;
        }

        PFNLoadEngineModule LoadEngineModule = FPlatformLibrary::LoadSymbol<PFNLoadEngineModule>(TEXT("LoadEngineModule"), Module);
        if (!LoadEngineModule)
        {
            OutError = FString::Printf(TEXT("Module '%s' does not export LoadEngineModule"), ModuleName);
            FPlatformLibrary::FreeDynamicLib(Module);
            return nullptr;
        }

        NewModule.Interface = LoadEngineModule();
        if (!NewModule.Interface)
        {
            OutError = FString::Printf(TEXT("Failed to load module '%s', resulting interface was nullptr"), ModuleName);
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
        if (bInBinariesDirectory)
        {
            LOG_INFO("Loaded module '%s' from '%s'", ModuleName, *ModuleFile);
        }
        else
        {
            LOG_INFO("Loaded module '%s'", ModuleName);
        }

        NewModule.Name = ModuleName;
        Modules.Emplace(NewModule);

        HandleModuleLoaded(ModuleName, NewModule.Interface);
        return NewModule.Interface;
    }
    else
    {
        OutError = FString::Printf(TEXT("Failed to load module '%s'"), ModuleName);
        SAFE_DELETE(NewModule.Interface);
        return nullptr;
    }
}

FString FModuleManager::FindModuleFile(const CHAR_T* ModuleName, bool& bOutInBinariesDirectory)
{
    TArray<FString> Directories;
    {
        SCOPED_LOCK(ModulesCS);
        Directories = BinariesDirectories;
    }

    const FString LibraryName = FPlatformLibrary::GetRealName(ModuleName);
    for (const FString& Directory : Directories)
    {
        const FString Filepath = Directory + '/' + LibraryName;
        if (FPlatformFile::IsFile(*Filepath))
        {
            bOutInBinariesDirectory = true;
            return Filepath;
        }
    }

    const FString NextToExecutable = File::GetDirectoryOf(FPlatformFile::GetExecutablePath()) + '/' + LibraryName;
    bOutInBinariesDirectory = false;
    return FPlatformFile::IsFile(*NextToExecutable) ? NextToExecutable : FString();
}

void FModuleManager::AddBinariesDirectory(const FString& Directory)
{
    SCOPED_LOCK(ModulesCS);

    if (!Directory.IsEmpty() && !BinariesDirectories.Contains(Directory))
    {
        BinariesDirectories.Add(Directory);
    }
}

IModule* FModuleManager::GetModule(const CHAR_T* ModuleName)
{
    SCOPED_LOCK(ModulesCS);

    if (IModule* EngineModule = GetModuleInternal(ModuleName))
    {
        return EngineModule;
    }
    else
    {
        LOG_ERROR("Failed to retrieve module '%s'", ModuleName);
        
        if (CVarModuleManagerBreakOnError.GetValue())
        {
            DEBUG_BREAK();
        }
        
        return nullptr;
    }
}

IModule* FModuleManager::GetModuleInternal(const CHAR_T* ModuleName)
{
    const int32 Index = GetModuleIndexUnlocked(ModuleName);
    if (Index >= 0)
    {
        IModule* EngineModule = Modules[Index].Interface;
        if (!EngineModule)
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

PlatformModule FModuleManager::GetModuleHandle(const CHAR_T* ModuleName)
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

void FModuleManager::RegisterStaticModule(const CHAR_T* ModuleName, FInitializeStaticModuleDelegate InitDelegate)
{
    SCOPED_LOCK(StaticModuleDelegatesCS);

    const bool bContains = StaticModuleDelegates.ContainsWithPredicate([=](const FStaticModulePair& Element)
    {
        return Element.First == ModuleName;
    });

    if (!bContains)
    {
        StaticModuleDelegates.Emplace(ModuleName, InitDelegate);
    }
}

bool FModuleManager::IsModuleLoaded(const CHAR_T* ModuleName)
{
    const int32 Index = GetModuleIndexUnlocked(ModuleName);
    return Index >= 0;
}

void FModuleManager::UnloadModule(const CHAR_T* ModuleName)
{
    SCOPED_LOCK(ModulesCS);

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
    SCOPED_LOCK(ModulesCS);
    return GetLoadedModuleCountUnlocked();
}

void FModuleManager::ReleaseAllModules()
{
    SCOPED_LOCK(ModulesCS);

    const int32 NumModules = Modules.Size();
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

FModuleManager::FInitializeStaticModuleDelegate* FModuleManager::GetStaticModuleDelegate(const CHAR_T* ModuleName)
{
    SCOPED_LOCK(StaticModuleDelegatesCS);

    const int32 Index = StaticModuleDelegates.FindWithPredicate([=](const FStaticModulePair& Element)
    {
        return Element.First == ModuleName;
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

int32 FModuleManager::GetModuleIndexUnlocked(const CHAR_T* ModuleName)
{
    const int32 Index = Modules.FindWithPredicate([=](const FModuleData& Element)
    {
        return Element.Name == ModuleName;
    });

    return static_cast<int32>(Index);
}
