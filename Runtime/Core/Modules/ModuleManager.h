#pragma once
#include "Core/CoreModule.h"

#include "Platform/PlatformLibrary.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Delegates/MulticastDelegate.h"

// Macro for implementing a new engine module based on monolithic or dynamic build
#if MONOLITHIC_BUILD
	#define IMPLEMENT_ENGINE_MODULE( ModuleClassType )                                     \
	static TStaticModuleInitializer<ModuleClassType> ModuleInitializer( #ModuleClassType );
#else
	#define IMPLEMENT_ENGINE_MODULE( ModuleClassType )  \
	extern "C"                                          \
	{                                                   \
		MODULE_EXPORT IEngineModule* LoadEngineModule() \
		{                                               \
			return dbg_new ModuleClassType();           \
		}                                               \
	}
#endif

/* Interface that all engine modules must implement */
class IEngineModule
{
public:

	virtual ~IEngineModule() = default;

	/* Called when the module is first loaded into the application */
	virtual bool Load() = 0;

	/* Called before the module is unloaded by the application */
	virtual bool Unload() = 0;

	/* The name of the module */
	virtual const char* GetName() const = 0;
};

// Function for loading a module
typedef IEngineModule* (*PFNLoadEngineModule)();

/* Default EngineModule that implements empty Load and Unload functions for modules that do not require these */
class CDefaultEngineModule : public IEngineModule
{
public:

	/* Called when the module is first loaded into the application */
	virtual bool Load() override
	{
		return true;
	}

	/* Called before the module is unloaded by the application */
	virtual bool Unload() override
	{
		return true;
	}
};

/* Handle for platform module-handle */

typedef PlatformLibrary::PlatformHandle PlatformModule;

/* ModuleManager that managers the modules used by the engine */
class CORE_API CModuleManager
{
public:
	
	/* Delegate for when a new module is loaded into the engine, name and IEngineModule pointer is the arguments */
	DECLARE_RETURN_DELEGATE( CLoadStaticModuleDelegate, IEngineModule* );

    /* Create the instance with make */
    static FORCEINLINE CModuleManager& Get() { return Instance; }

    /* Load a new module into the engine. ModuleName is without platform extension. */
    IEngineModule* LoadEngineModule( const char* ModuleName );

    /* Retrieve a already loaded module interface. ModuleName is without platform extension. */
    IEngineModule* GetEngineModule( const char* ModuleName );

    /* Load platform module */
    PlatformModule LoadModule( const char* ModuleName );
	
	/* Registers a static module */
	void RegisterStaticModule( const char* ModuleName, CLoadStaticModuleDelegate InitDelegate );

    /* Check if a module is already loaded */
    bool IsModuleLoaded( const char* ModuleName );

    /* Release a single module */
    void UnloadModule( const char* ModuleName );

    /* Releases all modules that are loaded */
    void ReleaseAllModules();
	
    /* Delegate for when a new module is loaded into the engine, name and IEngineModule pointer is the arguments */
    DECLARE_MULTICAST_DELEGATE( CModuleLoadedDelegate, const char*, IEngineModule* );
    CModuleLoadedDelegate GetModuleLoadedDelegate() { return ModuleLoadedDelegate; }

    FORCEINLINE IEngineModule& LoadEngineModuleRef( const char* ModuleName )
    {
        IEngineModule* Module = LoadEngineModule( ModuleName );
        Assert( Module != nullptr );

        return *Module;
    } 

    template<typename ModuleType>
    FORCEINLINE ModuleType* LoadEngineModule( const char* ModuleName )
    {
        return static_cast<ModuleType*>(LoadEngineModule( ModuleName ));
    }

    template<typename ModuleType>
    FORCEINLINE ModuleType& LoadEngineModuleRef( const char* ModuleName )
    {
        return static_cast<ModuleType&>(LoadEngineModuleRef( ModuleName ));
    }

    FORCEINLINE IEngineModule& GetEngineModuleRef(  const char* ModuleName )
    {
        IEngineModule* Module = GetEngineModule( ModuleName );
        Assert( Module != nullptr );

        return *Module;
    }

    template<typename ModuleType>
    FORCEINLINE ModuleType* GetEngineModule( const char* ModuleName )
    {
        return static_cast<ModuleType*>(GetEngineModule( ModuleName ));
    }

    template<typename ModuleType>
    FORCEINLINE ModuleType& GetEngineModuleRef( const char* ModuleName )
    {
        return static_cast<ModuleType&>(GetEngineModuleRef( ModuleName ));
    }

private:

    CModuleManager()
        : ModuleNames()
        , Modules()
    {
    }

    ~CModuleManager() = default;

    /* Returns the index of the specified module, if not found it returns -1 */
    int32 GetModuleIndex( const char* ModuleName );

    /* Delegate that is fired when a new module is loaded */ 
    CModuleLoadedDelegate ModuleLoadedDelegate;

    /* Platform handles to modules that are loaded */
    TArray<CString> ModuleNames;

    /* Array of all the loaded modules, the string is the name used to load the module from disk */
    TArray<TPair<IEngineModule*, PlatformModule>> Modules;
	
	// Array of all statically loaded modules

    static CModuleManager Instance;
};

/* Class that registers a static engine module */
template<typename ModuleClass>
class TStaticModuleInitializer
{
public:

	/* Constructor that registers the module to the modulemanager */
	TStaticModuleInitializer( const char* ModuleName )
	{
		CModuleManager::CLoadStaticModuleDelegate InitializeDelegate = CModuleManager::CLoadStaticModuleDelegate::CreateRaw(
				this, &TStaticModuleInitializer<ModuleClass>::MakeModuleInterface );

		CModuleManager::Get().RegisterStaticModule( ModuleName, InitializeDelegate );
	}
	
	/* Creates the ModuleInterface */
	IEngineModule* MakeModuleInterface()
	{
		return new ModuleClass();
	}
};
