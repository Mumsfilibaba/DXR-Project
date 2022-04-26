#pragma once
#include "Core/Core.h"
#include "Core/Logging/Log.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Optional.h"
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
// IEngineModule

typedef class IEngineModule* (*PFNLoadEngineModule)();

class IEngineModule
{
public:

    virtual ~IEngineModule() = default;

    /** @return: Returns true if the load is successful */
    virtual bool Load() = 0;

    /** @return: Returns true if the unload is successful */
    virtual bool Unload() = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CDefaultEngineModule

class CDefaultEngineModule : public IEngineModule
{
public:

    /** @return: Returns true if the load is successful */
    virtual bool Load() override { return true; }

    /** @return: Returns true if the unload is successful */
    virtual bool Unload() override { return true; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CModuleManager

typedef PlatformLibrary::PlatformHandle PlatformModule;

class CORE_API CModuleManager
{
private:

    friend class TOptional<CModuleManager>;

    struct SModule
    {
        SModule() = default;

        SModule(const String& InName, IEngineModule* InInterface)
            : Name(InName)
            , Interface(InInterface)
            , Handle(0)
        { }

        String         Name;
        IEngineModule* Interface;
        PlatformModule Handle;
    };

public:

    // Delegate for when a new module is loaded into the engine, name and IEngineModule pointer is the arguments
    DECLARE_RETURN_DELEGATE(CInitializeStaticModuleDelegate, IEngineModule*);

    /** @return: Returns a reference to the ModuleManager */
    static CModuleManager& Get();

    /** @brief: Releases all modules that are loaded */
    static void ReleaseAllLoadedModules();

    /** @brief: Destroy the module manager, after this no more modules can be loaded */
    static void Destroy();

    /**
     * @brief: Load a new module into the engine
     * 
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a pointer to a IEngineModule interface if the load is successful, otherwise nullptr
     */
    IEngineModule* LoadEngineModule(const char* ModuleName);

    /**
     * @brief: Retrieve a already loaded module interface
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a pointer to a IEngineModule interface if the interface is present, otherwise nullptr
     */
    IEngineModule* GetEngineModule(const char* ModuleName);

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
    void RegisterStaticModule(const char* ModuleName, CInitializeStaticModuleDelegate InitDelegate);

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

    /** Delegate for when a new module is loaded into the engine, name and IEngineModule pointer is the arguments */
    DECLARE_MULTICAST_DELEGATE(CModuleLoadedDelegate, const char*, IEngineModule*);
    CModuleLoadedDelegate GetModuleLoadedDelegate() { return ModuleLoadedDelegate; }

    /**
     * @brief: Load a new module into the engine
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
     * @brief: Load a new module into the engine
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
     * @brief: Load a new module into the engine
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
     * @brief: Retrieve a already loaded module interface
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
     * @brief: Load a new module into the engine
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
     * @brief: Load a new module into the engine
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

    static TOptional<CModuleManager>& GetModuleManagerInstance();

    CInitializeStaticModuleDelegate* GetStaticModuleDelegate(const char* ModuleName);

    int32 GetModuleIndex(const char* ModuleName);

    void ReleaseAllModules();

    CModuleLoadedDelegate ModuleLoadedDelegate;

    TArray<SModule> Modules;
    TArray<TPair<String, CInitializeStaticModuleDelegate>> StaticModuleDelegates;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStaticModuleInitializer

template<typename ModuleClass>
class TStaticModuleInitializer
{
    using CInitializeDelegate = CModuleManager::CInitializeStaticModuleDelegate;

public:

    /**
     * @brief: Constructor that registers the module to the ModuleManager
     * 
     * @param ModuleName: Name of the module
     */
    TStaticModuleInitializer(const char* ModuleName)
    {
        CInitializeDelegate InitializeDelegate = CInitializeDelegate::CreateRaw(this, &TStaticModuleInitializer<ModuleClass>::MakeModuleInterface);
        CModuleManager::Get().RegisterStaticModule(ModuleName, InitializeDelegate);
    }

    /** @return: The newly created module interface */
    IEngineModule* MakeModuleInterface()
    {
        return dbg_new ModuleClass();
    }
};
