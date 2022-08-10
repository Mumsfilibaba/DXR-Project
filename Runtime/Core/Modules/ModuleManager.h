#pragma once
#include "Platform/PlatformLibrary.h"

#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Optional.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Delegates/MulticastDelegate.h"
#include "Core/Debug/Debug.h"
#include "Core/Threading/Platform/CriticalSection.h"

#ifdef GetModuleHandle
    #undef GetModuleHandle
#endif

struct IModule;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Macro for implementing a new engine module based on monolithic or dynamic build

#if MONOLITHIC_BUILD
#define IMPLEMENT_ENGINE_MODULE(ModuleClassType, ModuleName)                                                                        \
    /* Self registering object for static modules */                                                                                \
    static TStaticModuleInitializer<ModuleClassType> GModuleInitializer( #ModuleName );                                             \
                                                                                                                                    \
    /* This function is force-included by the linker in order to not strip out the translation unit that contains the initializer*/ \
    extern "C" void LinkModule_##ModuleName() { }
#else
#define IMPLEMENT_ENGINE_MODULE(ModuleClassType, ModuleName)                                                                        \
    extern "C"                                                                                                                      \
    {                                                                                                                               \
        MODULE_EXPORT IModule* LoadEngineModule()                                                                                   \
        {                                                                                                                           \
            return dbg_new ModuleClassType();                                                                                       \
        }                                                                                                                           \
    }                                                                                                                               \
                                                                                                                                    \
    /* This function is force-included by the linker in order to not strip out the translation unit that contains the initializer*/ \
    extern "C" MODULE_EXPORT void LinkModule_##ModuleName() { }
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IModule

typedef IModule* (*PFNLoadEngineModule)();

struct IModule
{
    virtual ~IModule() = default;

    /** @return: Returns true if the load is successful */
    virtual bool Load() = 0;

    /** @return: Returns true if the unload is successful */
    virtual bool Unload() = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDefaultModule

struct FDefaultModule
    : public IModule
{
    /** @return: Returns true if the load is successful */
    virtual bool Load() override { return true; }

    /** @return: Returns true if the unload is successful */
    virtual bool Unload() override { return true; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FModuleManager

typedef void* PlatformModule;

class CORE_API FModuleManager
{
    friend class TOptional<FModuleManager>;

    struct FModule
    {
        FModule() = default;

        FModule(const FString& InName, IModule* InInterface)
            : Name(InName)
            , Interface(InInterface)
            , Handle(0)
        { }

        FString        Name;
        IModule*       Interface;
        PlatformModule Handle;
    };

public:
    // Delegate for when a new module is loaded into the engine, name and IModule pointer is the arguments
    DECLARE_RETURN_DELEGATE(FInitializeStaticModuleDelegate, IModule*);

    /** @return: Returns a reference to the ModuleManager */
    static FModuleManager& Get();

    /** @brief: Releases all modules that are loaded */
    static void ReleaseAllLoadedModules();

    /** @brief: ReleaseAllLoadedModules and Destroy the module manager, after this no more modules can be accessed */
    static void Shutdown();

    /**
     * @brief: Load a new module into the engine
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a pointer to a IModule interface if the load is successful, otherwise nullptr
     */
    IModule* LoadModule(const char* ModuleName);

    /**
     * @brief: Retrieve a already loaded module interface
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a pointer to a IModule interface if the interface is present, otherwise nullptr
     */
    IModule* GetModule(const char* ModuleName);

    /**
     * @brief: Retrieve a already loaded module's native handle
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a native handle to a module if the module is present otherwise a platform-defined invalid handle
     */
    PlatformModule GetModuleHandle(const char* ModuleName);

    /*
     * Registers a static module
     *
     * @param ModuleName: Name of the module to load without platform extension or prefix
     * @param InitDelegate: Delegate to initialize the static delegate
     */
    void RegisterStaticModule(const char* ModuleName, FInitializeStaticModuleDelegate InitDelegate);

    /**
     * @brief: Check if a module is already loaded
     *
     * @param ModuleName: Name of the module to load without platform extension or prefix
     * @return: Returns true if the module is loaded, otherwise false
     */
    bool IsModuleLoaded(const char* ModuleName);

    /**
     * @brief: Release a single module
     *
     * @param ModuleName: Name of the module to load without platform extension or prefix
     */
    void UnloadModule(const char* ModuleName);

    /** @return: Returns the number of loaded modules */
    uint32 GetLoadedModuleCount();

    /** Delegate for when a new module is loaded into the engine, name and IModule pointer is the arguments */
    DECLARE_MULTICAST_DELEGATE(FModuleLoadedDelegate, const char*, IModule*);
    FModuleLoadedDelegate GetModuleLoadedDelegate() { return ModuleLoadedDelegate; }

    /**
     * @brief: Load a new module into the engine
     *
     * @param ModuleName: Name of the module to load without platform extension or prefix
     * @return: A reference to the IModule interface, on fail an assert is triggered
     */
    FORCEINLINE IModule& LoadModuleRef(const char* ModuleName)
    {
        IModule* Module = LoadModule(ModuleName);
        Check(Module != nullptr);
        return *Module;
    }

    /**
     * @brief: Load a new module into the engine
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a typed pointer to if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType* LoadModule(const tchar* ModuleName)
    {
        return static_cast<ModuleType*>(LoadModule(ModuleName));
    }

    /**
     * @brief: Load a new module into the engine
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a typed reference to if the load is successful, on fail an assert is triggered
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType& LoadModuleRef(const tchar* ModuleName)
    {
        return static_cast<ModuleType&>(LoadModuleRef(ModuleName));
    }

    /**
     * @brief: Retrieve a already loaded module interface
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a reference to a typed interface if the interface is present, on fail an assert is triggered
     */
    FORCEINLINE IModule& GetModuleRef(const tchar* ModuleName)
    {
        IModule* Module = GetModule(ModuleName);
        Check(Module != nullptr);
        return *Module;
    }

    /**
     * @brief: Load a new module into the engine
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a typed pointer to a interface if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType* GetModule(const tchar* ModuleName)
    {
        return static_cast<ModuleType*>(GetModule(ModuleName));
    }

    /**
     * @brief: Load a new module into the engine
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a typed pointer to a interface if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType& GetModuleRef(const tchar* ModuleName)
    {
        return static_cast<ModuleType&>(GetModuleRef(ModuleName));
    }

private:
    FInitializeStaticModuleDelegate* GetStaticModuleDelegate(const char* ModuleName);

    int32 GetModuleIndexUnlocked(const char* ModuleName);

    void ReleaseAllModules();

    FORCEINLINE uint32 GetLoadedModuleCountUnlocked()
    {
        return static_cast<uint32>(Modules.GetSize());
    }

private:
    FModuleLoadedDelegate     ModuleLoadedDelegate;

    typedef TPair<FString, FInitializeStaticModuleDelegate> FStaticModulePair;
    TArray<FStaticModulePair> StaticModuleDelegates;
    FCriticalSection          StaticModuleDelegatesCS;

    TArray<FModule>           Modules;
    FCriticalSection          ModulesCS;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStaticModuleInitializer

template<typename ModuleClass>
class TStaticModuleInitializer
{
    using FInitializeDelegate = FModuleManager::FInitializeStaticModuleDelegate;

public:

    /**
     * @brief: Constructor that registers the module to the ModuleManager
     *
     * @param ModuleName: Name of the module
     */
    TStaticModuleInitializer(const tchar* ModuleName)
    {
        FInitializeDelegate InitializeDelegate = FInitializeDelegate::CreateRaw(this, &TStaticModuleInitializer::CreateModuleInterface);
        FModuleManager::Get().RegisterStaticModule(ModuleName, InitializeDelegate);
    }

    /** @return: The newly created module interface */
    IModule* CreateModuleInterface()
    {
        return dbg_new ModuleClass();
    }
};