#pragma once
#include "IEngineModule.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Containers/String.h"

class CORE_API CModuleManger
{
public:

    CModuleManger();
    ~CModuleManger() = default;

    /* Load a new module into the engine. ModuleName is without platform extension. */
    IEngineModule* LoadModule( const char* ModuleName );

    /* Retrieve a already loaded module interface. ModuleName is without platform extension. */
    IEngineModule* GetModule( const char* ModuleName );

    /* Release a single module */
    void ReleaseModule( const char* ModuleName );

    /* Releases all modules that are loaded */
    void ReleaseAllModule();

    template<typename ModuleType>
    FORCEINLINE ModuleType* LoadModule( const char* ModuleName )
    {
        return static_cast<ModuleType*>(LoadModule( ModuleName ));
    }

    template<typename ModuleType>
    FORCEINLINE ModuleType* GetModule( const char* ModuleName )
    {
        return static_cast<ModuleType*>(GetModule( ModuleName ));
    }

private:
    /* Array of all the loaded modules, the string is the name used to load the module from disk */
    TArray<TPair<CString, IEngineModule*>> Modules;
};

extern CModuleManger GModuleManager;