#pragma once
#include "IEngineModule.h"

#include "Platform/PlatformLibrary.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/MulticastDelegate.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

typedef PlatformLibrary::PlatformHandle PlatformModule;

///////////////////////////////////////////////////////////////////////////////////////////////////

class CORE_API CModuleManager
{
public:

    /* Create the instance with make */
    static FORCEINLINE CModuleManager& Get()
    {
        return Instance;
    }

    /* Load a new module into the engine. ModuleName is without platform extension. */
    IEngineModule* LoadEngineModule( const char* ModuleName );

    /* Retrieve a already loaded module interface. ModuleName is without platform extension. */
    IEngineModule* GetEngineModule( const char* ModuleName );

    /* Load platform module */
    PlatformModule LoadModule( const char* ModuleName );

    /* Check if a module is already loaded */
    bool IsModuleLoaded( const char* ModuleName );

    /* Release a single module */
    void UnloadModule( const char* ModuleName );

    /* Releases all modules that are loaded */
    void ReleaseAllModules();

    /* Delegate for when a new module is loaded into the engine, name and IEngineModule pointer is the arguments */
    DECLARE_MULTICAST_DELEGATE( CModuleLoadedDelegate, const char*, IEngineModule* );
    FORCEINLINE CModuleLoadedDelegate GetModuleLoadedDelegate()
    {
        return ModuleLoadedDelegate;
    }

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

    static CModuleManager Instance;
};
