#pragma once
#include "Core/Core.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Optional.h"
#include "Core/Delegates/Event.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Debug/Debug.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Platform/PlatformLibrary.h"

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

class CORE_API FModuleInterface
{
public: 
    /** Delegate for when a static module is loaded into the engine */
    DECLARE_RETURN_DELEGATE(FInitializeStaticModuleDelegate, IModule*);

    /** Delegate for when a new module is loaded into the engine, name and IModule pointer is the arguments */
    DECLARE_EVENT(FModuleLoadedDelegate, FModuleInterface, const CHAR*, IModule*);
    FModuleLoadedDelegate GetModuleLoadedDelegate() { return ModuleLoadedDelegate; }

    /** @return: Returns a reference to the ModuleManager */
    static FModuleInterface& Get();

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
    virtual IModule* LoadModule(const CHAR* ModuleName) = 0;

    /**
     * @brief: Retrieve a already loaded module interface
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a pointer to a IModule interface if the interface is present, otherwise nullptr
     */
    virtual IModule* GetModule(const CHAR* ModuleName) = 0;

    /**
     * @brief: Retrieve a already loaded module's native handle
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a native handle to a module if the module is present otherwise a platform-defined invalid handle
     */
    virtual PlatformModule GetModuleHandle(const CHAR* ModuleName) = 0;

    /**
     * @brief: Registers a static module
     *
     * @param ModuleName: Name of the module to load without platform extension or prefix
     * @param InitDelegate: Delegate to initialize the static delegate
     */
    virtual void RegisterStaticModule(const CHAR* ModuleName, FInitializeStaticModuleDelegate InitDelegate) = 0;

    /**
     * @brief: Check if a module is already loaded
     *
     * @param ModuleName: Name of the module to load without platform extension or prefix
     * @return: Returns true if the module is loaded, otherwise false
     */
    virtual bool IsModuleLoaded(const CHAR* ModuleName) = 0;

    /**
     * @brief: Release a single module
     *
     * @param ModuleName: Name of the module to load without platform extension or prefix
     */
    virtual void UnloadModule(const CHAR* ModuleName) = 0;

    /** @return: Returns the number of loaded modules */
    virtual uint32 GetLoadedModuleCount() = 0;

    /**
     * @brief: Load a new module into the engine
     *
     * @param ModuleName: Name of the module to load without platform extension or prefix
     * @return: A reference to the IModule interface, on fail an assert is triggered
     */
    FORCEINLINE IModule& LoadModuleRef(const CHAR* ModuleName)
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
    FORCEINLINE ModuleType* LoadModule(const TCHAR* ModuleName)
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
    FORCEINLINE ModuleType& LoadModuleRef(const TCHAR* ModuleName)
    {
        return static_cast<ModuleType&>(LoadModuleRef(ModuleName));
    }

    /**
     * @brief: Retrieve a already loaded module interface
     *
     * @param ModuleName: Name of the module without platform extension or prefix
     * @return: Returns a reference to a typed interface if the interface is present, on fail an assert is triggered
     */
    FORCEINLINE IModule& GetModuleRef(const TCHAR* ModuleName)
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
    FORCEINLINE ModuleType* GetModule(const TCHAR* ModuleName)
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
    FORCEINLINE ModuleType& GetModuleRef(const TCHAR* ModuleName)
    {
        return static_cast<ModuleType&>(GetModuleRef(ModuleName));
    }

protected:
    void HandleModuleLoaded(const CHAR* ModuleName, IModule* Module) { ModuleLoadedDelegate.Broadcast(ModuleName, Module); }

    FModuleLoadedDelegate ModuleLoadedDelegate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TStaticModuleInitializer

template<typename ModuleClassType>
class TStaticModuleInitializer
{
    using FInitializeDelegate = FModuleInterface::FInitializeStaticModuleDelegate;

public:

    /**
     * @brief: Constructor that registers the module to the ModuleManager
     *
     * @param ModuleName: Name of the module
     */
    TStaticModuleInitializer(const TCHAR* ModuleName)
    {
        FInitializeDelegate InitializeDelegate = FInitializeDelegate::CreateRaw(this, &TStaticModuleInitializer::CreateModuleInterface);
        FModuleInterface::Get().RegisterStaticModule(ModuleName, InitializeDelegate);
    }

    /** @return: The newly created module interface */
    IModule* CreateModuleInterface()
    {
        return dbg_new ModuleClassType();
    }
};