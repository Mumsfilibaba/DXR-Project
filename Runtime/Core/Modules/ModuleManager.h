#pragma once
#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Optional.h"
#include "Core/Delegates/Event.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Misc/Debug.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Time/Timespan.h"
#include "Core/Memory/NewOperators.h"

struct IModule;

typedef IModule* (*PFNLoadEngineModule)();
typedef void* PlatformModule;

struct IModule
{
    virtual ~IModule() = default;

    /** @return Returns true if the load is successful */
    virtual bool Load() { return true; }

    /** @return Returns true if the unload is successful */
    virtual bool Unload() { return true; }
};

class CORE_API FModuleManager
{
    struct FModuleData
    {
        FModuleData() = default;

        FModuleData(const FString& InName, IModule* InInterface)
            : Name(InName)
            , Interface(InInterface)
            , Handle(nullptr)
        {
        }

        FString        Name;
        IModule*       Interface;
        PlatformModule Handle;
    };

    friend class TOptional<FModuleManager>;

public:

    /** @brief The symbol a module of a modular build exports its ENGINE_MODULE_BUILD_STAMP as, see ModuleImplementation.h */
    static constexpr const CHAR_T* BuildStampSymbol = TEXT("GEngineModuleBuildStamp");

    /** 
     * @return Returns a reference to the ModuleManager 
     */
    static FModuleManager& Get();

    /** @return Returns the build stamp of this engine, which a dynamic module's has to equal for it to be loaded */
    static FString GetBuildStamp();

    /**
     * @brief Releases all modules that are loaded 
     */
    static void ReleaseAllLoadedModules();

    /**
     * @brief ReleaseAllLoadedModules and Destroy the module manager, after this no more modules can be accessed 
     */
    static void Shutdown();

public:

    /**
     * @brief Delegate for when a static module is loaded into the engine 
     */
    DECLARE_RETURN_DELEGATE(FInitializeStaticModuleDelegate, IModule*);

    /** 
     * @brief Delegate for when a new module is loaded into the engine, name and IModule pointer is the arguments 
     */
    DECLARE_EVENT(FModuleLoadedDelegate, FModuleManager, const CHAR_T*, IModule*);
    FModuleLoadedDelegate GetModuleLoadedDelegate() { return ModuleLoadedDelegate; }

    /**
     * @brief Load a new module into the engine, logging why when it fails
     * @param ModuleName Name of the module without platform extension or prefix
     * @return Returns a pointer to a IModule interface if the load is successful, otherwise nullptr
     */
    IModule* LoadModule(const CHAR_T* ModuleName);

    /**
     * @brief Load a new module into the engine. A dynamic module found in a binaries directory or next to the executable is
     * only loaded when its build stamp equals this engine's, which is checked before any of its code runs where the platform
     * allows it (see FPlatformLibrary::ReadStringExport).
     * @param ModuleName Name of the module without platform extension or prefix
     * @param OutError Receives why loading failed, such as a module built by another engine build
     * @return Returns a pointer to a IModule interface if the load is successful, otherwise nullptr
     */
    IModule* LoadModule(const CHAR_T* ModuleName, FString& OutError);

    /**
     * @brief Add a directory to look for dynamic modules in before the platform's own search path, such as the project's
     * binaries. A module whose file is found in one of them is only loaded from there.
     * @param Directory Directory holding module libraries, named with the platform's prefix and extension
     */
    void AddBinariesDirectory(const FString& Directory);

    /**
     * @brief Retrieve a already loaded module interface
     * @param ModuleName Name of the module without platform extension or prefix
     * @return Returns a pointer to a IModule interface if the interface is present, otherwise nullptr
     */
    IModule* GetModule(const CHAR_T* ModuleName);

    /**
     * @brief Retrieve a already loaded module's native handle
     * @param ModuleName Name of the module without platform extension or prefix
     * @return Returns a native handle to a module if the module is present otherwise a platform-defined invalid handle
     */
    PlatformModule GetModuleHandle(const CHAR_T* ModuleName);

    /**
     * @brief Registers a static module
     * @param ModuleName Name of the module to load without platform extension or prefix
     * @param InitDelegate Delegate to initialize the static delegate
     */
    void RegisterStaticModule(const CHAR_T* ModuleName, FInitializeStaticModuleDelegate InitDelegate);

    /**
     * @brief Check if a module is already loaded
     * @param ModuleName Name of the module to load without platform extension or prefix
     * @return Returns true if the module is loaded, otherwise false
     */
    bool IsModuleLoaded(const CHAR_T* ModuleName);

    /**
     * @brief Release a single module
     * @param ModuleName Name of the module to load without platform extension or prefix
     */
    void UnloadModule(const CHAR_T* ModuleName);

    /**
     * @return Returns the number of loaded modules 
     */
    uint32 GetLoadedModuleCount();

    /**
     * @brief Load a new module into the engine
     * @param ModuleName Name of the module to load without platform extension or prefix
     * @return A reference to the IModule interface, on fail an assert is triggered
     */
    FORCEINLINE IModule& LoadModuleRef(const CHAR_T* ModuleName)
    {
        IModule* Module = LoadModule(ModuleName);
        CHECK(Module != nullptr);
        return *Module;
    }

    /**
     * @brief Load a new module into the engine
     * @param ModuleName Name of the module without platform extension or prefix
     * @return Returns a typed pointer to if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType* LoadModule(const CHAR_T* ModuleName)
    {
        return static_cast<ModuleType*>(LoadModule(ModuleName));
    }

    /**
     * @brief Load a new module into the engine
     * @param ModuleName Name of the module without platform extension or prefix
     * @return Returns a typed reference to if the load is successful, on fail an assert is triggered
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType& LoadModuleRef(const CHAR_T* ModuleName)
    {
        return static_cast<ModuleType&>(LoadModuleRef(ModuleName));
    }

    /**
     * @brief Retrieve a already loaded module interface
     * @param ModuleName Name of the module without platform extension or prefix
     * @return Returns a reference to a typed interface if the interface is present, on fail an assert is triggered
     */
    FORCEINLINE IModule& GetModuleRef(const CHAR_T* ModuleName)
    {
        IModule* Module = GetModule(ModuleName);
        CHECK(Module != nullptr);
        return *Module;
    }

    /**
     * @brief Load a new module into the engine
     * @param ModuleName Name of the module without platform extension or prefix
     * @return Returns a typed pointer to a interface if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType* GetModule(const CHAR_T* ModuleName)
    {
        return static_cast<ModuleType*>(GetModule(ModuleName));
    }

    /**
     * @brief Load a new module into the engine
     * @param ModuleName Name of the module without platform extension or prefix
     * @return Returns a typed pointer to a interface if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType& GetModuleRef(const CHAR_T* ModuleName)
    {
        return static_cast<ModuleType&>(GetModuleRef(ModuleName));
    }

protected:
    void HandleModuleLoaded(const CHAR_T* ModuleName, IModule* Module) 
    {
        ModuleLoadedDelegate.Broadcast(ModuleName, Module);
    }
    
    void ReleaseAllModules();

    FInitializeStaticModuleDelegate* GetStaticModuleDelegate(const CHAR_T* ModuleName);

    // Finds the module's library in a binaries directory or next to the executable, or returns an empty string
    FString FindModuleFile(const CHAR_T* ModuleName, bool& bOutInBinariesDirectory);

    // Used to retrieve a module, does not break on error
    IModule* GetModuleInternal(const CHAR_T* ModuleName);
    
    int32 GetModuleIndexUnlocked(const CHAR_T* ModuleName);

    uint32 GetLoadedModuleCountUnlocked()
    {
        return static_cast<uint32>(Modules.Size());
    }

    typedef TPair<FString, FInitializeStaticModuleDelegate> FStaticModulePair;

    TArray<FStaticModulePair> StaticModuleDelegates;
    FCriticalSection          StaticModuleDelegatesCS;
    FModuleLoadedDelegate     ModuleLoadedDelegate;
    TArray<FModuleData>       Modules;
    TArray<FString>           BinariesDirectories;
    FCriticalSection          ModulesCS;
};

template<typename ModuleClassType>
class TStaticModuleInitializer
{
    using FInitializeDelegate = FModuleManager::FInitializeStaticModuleDelegate;

public:

    /**
     * @brief Constructor that registers the module to the ModuleManager
     * @param ModuleName Name of the module
     */
    TStaticModuleInitializer(const CHAR_T* ModuleName)
    {
        FInitializeDelegate InitializeDelegate = FInitializeDelegate::CreateRaw(this, &TStaticModuleInitializer::CreateModuleInterface);
        FModuleManager::Get().RegisterStaticModule(ModuleName, InitializeDelegate);
    }

    /** 
     * @return The newly created module interface 
     */
    IModule* CreateModuleInterface()
    {
        return new ModuleClassType();
    }
};

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FGameModule : public IModule
{
    virtual ~FGameModule() = default;

    /** @return Returns true if the initialization is successful */
    virtual bool Init() { return true; }

    /** @brief Tick the application module */
    virtual void Tick(float DeltaTime) { }

    /** @return Returns true if the release is successful */
    virtual void Release() { }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
