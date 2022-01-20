#pragma once
#include "Core/Core.h"
#include "Core/Logging/Log.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Delegates/MulticastDelegate.h"
#include "Core/Debug/Debug.h"

#include "Platform/PlatformLibrary.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Macro for implementing a new engine module based on monolithic or dynamic build

#if MONOLITHIC_BUILD
#define IMPLEMENT_ENGINE_MODULE( ModuleClassType, ModuleName )                                                                  \
    /* Self registering object for static modules */                                                                                \
    static TStaticModuleInitializer<ModuleClassType> GModuleInitializer( #ModuleName );                                             \
                                                                                                                                    \
    /* This function is force-included by the linker in order to not strip out the translation unit that contains the initializer*/ \
    extern "C" void LinkModule_##ModuleName() { }
#else
#define IMPLEMENT_ENGINE_MODULE( ModuleClassType, ModuleName )                                                                  \
    extern "C"                                                                                                                      \
    {                                                                                                                               \
        MODULE_EXPORT IEngineModule* LoadEngineModule()                                                                             \
        {                                                                                                                           \
            return dbg_new ModuleClassType();                                                                                       \
        }                                                                                                                           \
    }                                                                                                                               \
                                                                                                                                    \
    /* This function is force-included by the linker in order to not strip out the translation unit that contains the initializer*/ \
    extern "C" MODULE_EXPORT void LinkModule_##ModuleName() { }
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Interface that all engine modules must implement

typedef class IEngineModule* (*PFNLoadEngineModule)();

class IEngineModule
{
public:

    virtual ~IEngineModule() = default;

    // Called when the module is first loaded into the application
    virtual bool Load() = 0;

    // Called before the module is unloaded by the application
    virtual bool Unload() = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Default engine module that returns true for Load and Unload by default 

class CDefaultEngineModule : public IEngineModule
{
public:

    // Called when the module is first loaded into the application
    virtual bool Load() override { return true; }

    // Called before the module is unloaded by the application
    virtual bool Unload() override { return true; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ModuleManager that manages the modules used by the engine

typedef PlatformLibrary::PlatformHandle PlatformModule;

class CORE_API CModuleManager
{
public:

    // Delegate for when a new module is loaded into the engine, name and IEngineModule pointer is the arguments
    DECLARE_RETURN_DELEGATE(CInitializeStaticModuleDelegate, IEngineModule*);

    // Create the instance with make
    static CModuleManager& Get();

    // Releases all modules that are loaded
    static void Release();

    // Load a new module into the engine. ModuleName is without platform extension.
    IEngineModule* LoadEngineModule(const char* ModuleName);

    // Retrieve a already loaded module interface. ModuleName is without platform extension.
    IEngineModule* GetEngineModule(const char* ModuleName);

    // Load platform module
    PlatformModule GetModule(const char* ModuleName);

    // Registers a static module
    void RegisterStaticModule(const char* ModuleName, CInitializeStaticModuleDelegate InitDelegate);

    // Check if a module is already loaded
    bool IsModuleLoaded(const char* ModuleName);

    // Release a single module
    void UnloadModule(const char* ModuleName);

    // Delegate for when a new module is loaded into the engine, name and IEngineModule pointer is the arguments
    DECLARE_MULTICAST_DELEGATE(CModuleLoadedDelegate, const char*, IEngineModule*);
    CModuleLoadedDelegate GetModuleLoadedDelegate() { return ModuleLoadedDelegate; }

    FORCEINLINE IEngineModule& LoadEngineModuleRef(const char* ModuleName)
    {
        IEngineModule* Module = LoadEngineModule(ModuleName);
        Assert(Module != nullptr);

        return *Module;
    }

    template<typename ModuleType>
    FORCEINLINE ModuleType* LoadEngineModule(const char* ModuleName)
    {
        return static_cast<ModuleType*>(LoadEngineModule(ModuleName));
    }

    template<typename ModuleType>
    FORCEINLINE ModuleType& LoadEngineModuleRef(const char* ModuleName)
    {
        return static_cast<ModuleType&>(LoadEngineModuleRef(ModuleName));
    }

    FORCEINLINE IEngineModule& GetEngineModuleRef(const char* ModuleName)
    {
        IEngineModule* Module = GetEngineModule(ModuleName);
        Assert(Module != nullptr);

        return *Module;
    }

    template<typename ModuleType>
    FORCEINLINE ModuleType* GetEngineModule(const char* ModuleName)
    {
        return static_cast<ModuleType*>(GetEngineModule(ModuleName));
    }

    template<typename ModuleType>
    FORCEINLINE ModuleType& GetEngineModuleRef(const char* ModuleName)
    {
        return static_cast<ModuleType&>(GetEngineModuleRef(ModuleName));
    }

private:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Stores information about a loaded engine module

    struct SModule
    {
        SModule() = default;

        SModule(const CString& InName, IEngineModule* InInterface)
            : Name(InName)
            , Interface(InInterface)
            , Handle(0)
        {
        }

        // Name of the module
        CString Name;

        // The actual interface 
        IEngineModule* Interface;

        // Platform Handle, this is zero if the module is loaded statically and is the only time it should be zero
        PlatformModule Handle;
    };

    CModuleManager() = default;
    ~CModuleManager() = default;

    // Returns the index of the specified module, if not found it returns -1
    int32 GetModuleIndex(const char* ModuleName);

    // Returns a delegate to initialize a static module, if not found it returns nullptr
    CInitializeStaticModuleDelegate* GetStaticModuleDelegate(const char* ModuleName);

    // Delegate that is fired when a new module is loaded
    CModuleLoadedDelegate ModuleLoadedDelegate;

    // Array of all the loaded modules, the string is the name used to load the module from disk
    TArray<SModule> Modules;

    // Array of all modules that can be loaded statically
    TArray<TPair<CString, CInitializeStaticModuleDelegate>> StaticModuleDelegates;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class that registers a static engine module */

template<typename ModuleClass>
class TStaticModuleInitializer
{
    using CInitializeDelegate = CModuleManager::CInitializeStaticModuleDelegate;

public:

    // Constructor that registers the module to the ModuleManager
    TStaticModuleInitializer(const char* ModuleName)
    {
        CInitializeDelegate InitializeDelegate = CInitializeDelegate::CreateRaw(this, &TStaticModuleInitializer<ModuleClass>::MakeModuleInterface);
        CModuleManager::Get().RegisterStaticModule(ModuleName, InitializeDelegate);
    }

    // Creates the ModuleInterface
    IEngineModule* MakeModuleInterface()
    {
        return new ModuleClass();
    }
};
