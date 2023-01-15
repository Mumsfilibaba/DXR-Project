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

/** 
 * Macro for implementing a new engine module based on monolithic or dynamic build
 */

#if MONOLITHIC_BUILD
#define IMPLEMENT_ENGINE_MODULE(ModuleClassType, ModuleName)                                                                                  \
    /** @brief - Self registering object for static modules */                                                                                \
    static TStaticModuleInitializer<ModuleClassType> GModuleInitializer( #ModuleName );                                                       \
    /** @brief - This function is force-included by the linker in order to not strip out the translation unit that contains the initializer*/ \
    extern "C" void LinkModule_##ModuleName() { }
#else
#define IMPLEMENT_ENGINE_MODULE(ModuleClassType, ModuleName)                                                                                  \
    extern "C"                                                                                                                                \
    {                                                                                                                                         \
        MODULE_EXPORT FModuleInterface* LoadEngineModule()                                                                                    \
        {                                                                                                                                     \
            return new ModuleClassType();                                                                                                     \
        }                                                                                                                                     \
    }                                                                                                                                         \
                                                                                                                                              \
    /** @brief - This function is force-included by the linker in order to not strip out the translation unit that contains the initializer*/ \
    extern "C" MODULE_EXPORT void LinkModule_##ModuleName() { }                                                                               \
    IMPLEMENT_NEW_AND_DELETE_OPERATORS()
#endif

struct FModuleInterface;

typedef FModuleInterface* (*PFNLoadEngineModule)();
typedef void* PlatformModule;


struct FModuleInterface
{
    virtual ~FModuleInterface() = default;

    /** @return - Returns true if the load is successful */
    virtual bool Load() { return true; }

    /** @return - Returns true if the unload is successful */
    virtual bool Unload() { return true; }
};


class CORE_API FModuleManager
{
    struct FModuleData
    {
        FModuleData() = default;

        FModuleData(const FString& InName, FModuleInterface* InInterface)
            : Name(InName)
            , Interface(InInterface)
            , Handle(nullptr)
        { }

        FString           Name;
        FModuleInterface* Interface;
        PlatformModule    Handle;
    };

    friend class TOptional<FModuleManager>;

    FModuleManager()  = default;
    ~FModuleManager() = default;

public:
    
    /**
     * @brief - Delegate for when a static module is loaded into the engine 
     */
    DECLARE_RETURN_DELEGATE(FInitializeStaticModuleDelegate, FModuleInterface*);

    /** 
     * @brief - Delegate for when a new module is loaded into the engine, name and IModule pointer is the arguments 
     */
    DECLARE_EVENT(FModuleLoadedDelegate, FModuleManager, const CHAR*, FModuleInterface*);
    FModuleLoadedDelegate GetModuleLoadedDelegate() { return ModuleLoadedDelegate; }

    /** 
     * @return - Returns a reference to the ModuleManager 
     */
    static FModuleManager& Get();

    /**
     * @brief - Releases all modules that are loaded 
     */
    static void ReleaseAllLoadedModules();

    /**
     * @brief - ReleaseAllLoadedModules and Destroy the module manager, after this no more modules can be accessed 
     */
    static void Shutdown();

    /**
     * @brief            - Load a new module into the engine
     * @param ModuleName - Name of the module without platform extension or prefix
     * @return           - Returns a pointer to a IModule interface if the load is successful, otherwise nullptr
     */
    FModuleInterface* LoadModule(const CHAR* ModuleName);

    /**
     * @brief            - Retrieve a already loaded module interface
     * @param ModuleName - Name of the module without platform extension or prefix
     * @return           - Returns a pointer to a IModule interface if the interface is present, otherwise nullptr
     */
    FModuleInterface* GetModule(const CHAR* ModuleName);

    /**
     * @brief            - Retrieve a already loaded module's native handle
     * @param ModuleName - Name of the module without platform extension or prefix
     * @return           - Returns a native handle to a module if the module is present otherwise a platform-defined invalid handle
     */
    PlatformModule GetModuleHandle(const CHAR* ModuleName);

    /**
     * @brief              - Registers a static module
     * @param ModuleName   - Name of the module to load without platform extension or prefix
     * @param InitDelegate - Delegate to initialize the static delegate
     */
    void RegisterStaticModule(const CHAR* ModuleName, FInitializeStaticModuleDelegate InitDelegate);

    /**
     * @brief            - Check if a module is already loaded
     * @param ModuleName - Name of the module to load without platform extension or prefix
     * @return           - Returns true if the module is loaded, otherwise false
     */
    bool IsModuleLoaded(const CHAR* ModuleName);

    /**
     * @brief            - Release a single module
     * @param ModuleName - Name of the module to load without platform extension or prefix
     */
    void UnloadModule(const CHAR* ModuleName);

    /**
     * @return - Returns the number of loaded modules 
     */
    uint32 GetLoadedModuleCount();

    /**
     * @brief            - Load a new module into the engine
     * @param ModuleName - Name of the module to load without platform extension or prefix
     * @return           - A reference to the IModule interface, on fail an assert is triggered
     */
    FORCEINLINE FModuleInterface& LoadModuleRef(const CHAR* ModuleName)
    {
        FModuleInterface* Module = LoadModule(ModuleName);
        CHECK(Module != nullptr);
        return *Module;
    }

    /**
     * @brief            - Load a new module into the engine
     * @param ModuleName - Name of the module without platform extension or prefix
     * @return           - Returns a typed pointer to if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType* LoadModule(const TCHAR* ModuleName)
    {
        return static_cast<ModuleType*>(LoadModule(ModuleName));
    }

    /**
     * @brief            - Load a new module into the engine
     * @param ModuleName - Name of the module without platform extension or prefix
     * @return           - Returns a typed reference to if the load is successful, on fail an assert is triggered
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType& LoadModuleRef(const TCHAR* ModuleName)
    {
        return static_cast<ModuleType&>(LoadModuleRef(ModuleName));
    }

    /**
     * @brief            - Retrieve a already loaded module interface
     * @param ModuleName - Name of the module without platform extension or prefix
     * @return           - Returns a reference to a typed interface if the interface is present, on fail an assert is triggered
     */
    FORCEINLINE FModuleInterface& GetModuleRef(const TCHAR* ModuleName)
    {
        FModuleInterface* Module = GetModule(ModuleName);
        CHECK(Module != nullptr);
        return *Module;
    }

    /**
     * @brief            - Load a new module into the engine
     * @param ModuleName - Name of the module without platform extension or prefix
     * @return           - Returns a typed pointer to a interface if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType* GetModule(const TCHAR* ModuleName)
    {
        return static_cast<ModuleType*>(GetModule(ModuleName));
    }

    /**
     * @brief            - Load a new module into the engine
     * @param ModuleName - Name of the module without platform extension or prefix
     * @return           - Returns a typed pointer to a interface if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType& GetModuleRef(const TCHAR* ModuleName)
    {
        return static_cast<ModuleType&>(GetModuleRef(ModuleName));
    }

protected:
    void HandleModuleLoaded(const CHAR* ModuleName, FModuleInterface* Module) { ModuleLoadedDelegate.Broadcast(ModuleName, Module); }
    void ReleaseAllModules();

    FInitializeStaticModuleDelegate* GetStaticModuleDelegate(const CHAR* ModuleName);

    int32 GetModuleIndexUnlocked(const CHAR* ModuleName);

    FORCEINLINE uint32 GetLoadedModuleCountUnlocked()
    {
        return static_cast<uint32>(Modules.GetSize());
    }

    FModuleLoadedDelegate     ModuleLoadedDelegate;

    typedef TPair<FString, FInitializeStaticModuleDelegate> FStaticModulePair;
    TArray<FStaticModulePair> StaticModuleDelegates;
    FCriticalSection          StaticModuleDelegatesCS;

    TArray<FModuleData>       Modules;
    FCriticalSection          ModulesCS;
};


template<typename ModuleClassType>
class TStaticModuleInitializer
{
    using FInitializeDelegate = FModuleManager::FInitializeStaticModuleDelegate;

public:

    /**
     * @brief            - Constructor that registers the module to the ModuleManager
     * @param ModuleName - Name of the module
     */
    TStaticModuleInitializer(const TCHAR* ModuleName)
    {
        FInitializeDelegate InitializeDelegate = FInitializeDelegate::CreateRaw(this, &TStaticModuleInitializer::CreateModuleInterface);
        FModuleManager::Get().RegisterStaticModule(ModuleName, InitializeDelegate);
    }

    /** 
     * @return - The newly created module interface 
     */
    FModuleInterface* CreateModuleInterface()
    {
        return new ModuleClassType();
    }
};


DISABLE_UNREFERENCED_VARIABLE_WARNING

class CORE_API FGameModule
    : public FModuleInterface
{
public:
    virtual ~FGameModule() = default;

    /** 
     * @return - Returns true if the initialization is successful 
     */
    virtual bool Init();

    /**
     * @brief           - Tick the application module
     * @param DeltaTime - Time since last time the application was ticked
     */
    virtual void Tick(FTimespan Deltatime) { }

    /** 
     * @return - Returns true if the release is successful 
     */
    virtual bool Release();

    /** 
     * @return - Returns true if the load is successful
     */
    virtual bool Load() override;

    /**
     * @return - Returns true if the unload is successful
     */
    virtual bool Unload() override;

protected:
    FDelegateHandle TickHandle;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING