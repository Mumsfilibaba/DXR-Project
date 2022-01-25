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

    /**
     * Called when the module is first loaded into the application
     *
     * @return: Returns true if the load is successful
     */
    virtual bool Load() = 0;

    /**
     * Called before the module is unloaded by the application
     *
     * @return: Returns true if the unload is successful
     */
    virtual bool Unload() = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Default engine module that returns true for Load and Unload by default 

class CDefaultEngineModule : public IEngineModule
{
public:

    /**
     * Called when the module is first loaded into the application
     *
     * @return: Returns true if the load is successful
     */
    virtual bool Load() override { return true; }

    /**
     * Called before the module is unloaded by the application
     *
     * @return: Returns true if the unload is successful
     */
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

    /**
     * Retrieve the ModuleManager instance
     *
     * @return: Returns a reference to the ModuleManager
     */
    static CModuleManager& Get();

    /**
     * Releases all modules that are loaded
     */
    static void Release();

    /**
     * Load a new module into the engine
     * 
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a pointer to a IEngineModule interface if the load is successful, otherwise nullptr
     */
    IEngineModule* LoadEngineModule(const char* ModuleName);

    /**
     * Retrieve a already loaded module interface
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a pointer to a IEngineModule interface if the interface is present, otherwise nullptr
     */
    IEngineModule* GetEngineModule(const char* ModuleName);

    /**
     * Retrieve a already loaded module's native handle
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
    void RegisterStaticModule(const char* ModuleName, CInitializeStaticModuleDelegate InitDelegate);

    /**
     * Check if a module is already loaded
     * 
     * @param ModuleName: Name of the module to load without platform extension or prefix
     * @return: Returns true if the module is loaded, otherwise false
     */
    bool IsModuleLoaded(const char* ModuleName);

    /**
     * Release a single module
     *
     * @param ModuleName: Name of the module to load without platform extension or prefix
     */
    void UnloadModule(const char* ModuleName);

    /** Delegate for when a new module is loaded into the engine, name and IEngineModule pointer is the arguments */
    DECLARE_MULTICAST_DELEGATE(CModuleLoadedDelegate, const char*, IEngineModule*);
    CModuleLoadedDelegate GetModuleLoadedDelegate() { return ModuleLoadedDelegate; }

    /**
     * Load a new module into the engine
     *
     * @param ModuleName: Name of the module to load without platform extension or prefix
     * @return: A reference to the IEngineModule interface, on fail an assert is triggered
     */
    FORCEINLINE IEngineModule& LoadEngineModuleRef(const char* ModuleName)
    {
        IEngineModule* Module = LoadEngineModule(ModuleName);
        Assert(Module != nullptr);

        return *Module;
    }

    /**
     * Load a new module into the engine
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a typed pointer to if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType* LoadEngineModule(const char* ModuleName)
    {
        return static_cast<ModuleType*>(LoadEngineModule(ModuleName));
    }

    /**
     * Load a new module into the engine
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a typed reference to if the load is successful, on fail an assert is triggered
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType& LoadEngineModuleRef(const char* ModuleName)
    {
        return static_cast<ModuleType&>(LoadEngineModuleRef(ModuleName));
    }

    /**
     * Retrieve a already loaded module interface
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a reference to a typed interface if the interface is present, on fail an assert is triggered
     */
    FORCEINLINE IEngineModule& GetEngineModuleRef(const char* ModuleName)
    {
        IEngineModule* Module = GetEngineModule(ModuleName);
        Assert(Module != nullptr);

        return *Module;
    }

    /**
     * Load a new module into the engine
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a typed pointer to a interface if the load is successful, otherwise nullptr
     */
    template<typename ModuleType>
    FORCEINLINE ModuleType* GetEngineModule(const char* ModuleName)
    {
        return static_cast<ModuleType*>(GetEngineModule(ModuleName));
    }

    /**
     * Load a new module into the engine
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a typed pointer to a interface if the load is successful, otherwise nullptr
     */
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

        /** Name of the module */
        CString Name;
        /** The actual interface */
        IEngineModule* Interface;
        /** Platform Handle, this is zero if the module is loaded statically and is the only time it should be zero */
        PlatformModule Handle;
    };

    CModuleManager() = default;
    ~CModuleManager() = default;

    /** Returns the index of the specified module, if not found it returns -1 */
    int32 GetModuleIndex(const char* ModuleName);

    /** Returns a delegate to initialize a static module, if not found it returns nullptr */ 
    CInitializeStaticModuleDelegate* GetStaticModuleDelegate(const char* ModuleName);

    CModuleLoadedDelegate ModuleLoadedDelegate;

    TArray<SModule> Modules;

    /** Array of all modules that can be loaded statically */
    TArray<TPair<CString, CInitializeStaticModuleDelegate>> StaticModuleDelegates;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class that registers a static engine module */

template<typename ModuleClass>
class TStaticModuleInitializer
{
    using CInitializeDelegate = CModuleManager::CInitializeStaticModuleDelegate;

public:

    /**
     * Constructor that registers the module to the ModuleManager
     * 
     * @param ModuleName: Name of the module
     */
    TStaticModuleInitializer(const char* ModuleName)
    {
        CInitializeDelegate InitializeDelegate = CInitializeDelegate::CreateRaw(this, &TStaticModuleInitializer<ModuleClass>::MakeModuleInterface);
        CModuleManager::Get().RegisterStaticModule(ModuleName, InitializeDelegate);
    }

    /**
     * Creates the ModuleInterface
     * 
     * @return: The newly created module interface
     */
    IEngineModule* MakeModuleInterface()
    {
        return dbg_new ModuleClass();
    }
};
