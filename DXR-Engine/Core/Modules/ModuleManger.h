#pragma once
#include "IEngineModule.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/String.h"

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

    /* Release a single module */
    void ReleaseModule( const char* ModuleName );

    /* Releases all modules that are loaded */
    void ReleaseAllModule();

    template<typename ModuleType>
    FORCEINLINE ModuleType* LoadEngineModule( const char* ModuleName )
    {
        return static_cast<ModuleType*>(LoadEngineModule( ModuleName ));
    }

    template<typename ModuleType>
    FORCEINLINE ModuleType* GetEngineModule( const char* ModuleName )
    {
        return static_cast<ModuleType*>(GetEngineModule( ModuleName ));
    }

private:
    
    CModuleManager() = default;
    ~CModuleManager() = default;

    /* Array of all the loaded modules, the string is the name used to load the module from disk */
    TArray<TPair<CString, IEngineModule*>> Modules;

    static CModuleManager Instance;
};
