#include "ModuleManger.h"

#include "Core/Windows/Windows.h"
#include "Core/Windows/Windows.inl"

#include "Core/Templates/StringTraits.h"

CModuleManager CModuleManager::Instance;

IEngineModule* CModuleManager::LoadEngineModule( const char* ModuleName )
{
    Assert( ModuleName != nullptr );

    IEngineModule* ExistingModule = GetEngineModule( ModuleName );
    if ( ExistingModule )
    {
        LOG_WARNING( "Module '" + CString( ModuleName ) + "' is already loaded" );
        return ExistingModule;
    }

    // TODO: Abstract the Win-Api
    HMODULE Module = LoadLibraryA(ModuleName);
    if ( Module == NULL )
    {
        LOG_ERROR( "Failed to find module '" + CString( ModuleName ) + "'");
        return nullptr;
    }

    // Requires that the module has a LoadEngineModule function exported
    PFNLoadEngineModule LoadEngineModule = GetTypedProcAddress<PFNLoadEngineModule>( Module, "LoadEngineModule" );
    if ( !LoadEngineModule )
    {
        return nullptr;
    }

    IEngineModule* NewModule = LoadEngineModule();
    if ( !NewModule || (NewModule && !NewModule->Load()))
    {
        LOG_ERROR( "Failed to load module '" + CString( ModuleName ) + "', resulting interface was nullptr" );
        return nullptr;
    }
    else
    {
        LOG_INFO( "Loaded module'" + CString( ModuleName ) + "'" );
        
        TPair<IEngineModule*, PlatformModule> NewPair = MakePair<IEngineModule*, PlatformModule>( NewModule, Module );
        Modules.Emplace( NewPair );
        ModuleNames.Emplace( ModuleName );
        return NewModule;
    }
}

IEngineModule* CModuleManager::GetEngineModule( const char* ModuleName )
{
    int32 Index = GetModuleIndex( ModuleName );
    if ( Index >= 0 )
    {
        const TPair<IEngineModule*, PlatformModule>& Pair = Modules[Index];
        
        IEngineModule* EngineModule = Pair.First;
        if ( EngineModule )
        {
            LOG_WARNING("Module is loaded but does not contain an EngineModule interface");
            return nullptr;
        }
        else
        {
            return EngineModule;
        }
    }
    else
    {
        return nullptr;
    }
}

PlatformModule CModuleManager::LoadModule( const char* ModuleName )
{
    int32 Index = GetModuleIndex( ModuleName );
    if ( Index >= 0 )
    {
        const TPair<IEngineModule*, PlatformModule>& Pair = Modules[Index];
        return Pair.Second;
    }
    else
    {
        return NULL;
    }
}

bool CModuleManager::IsModuleLoaded( const char* ModuleName )
{
    int32 Index = GetModuleIndex( ModuleName );
    return (Index >= 0);
}

void CModuleManager::UnloadModule( const char* ModuleName )
{
    int32 Index = GetModuleIndex(ModuleName);
    if ( Index >= 0 )
    {
        const TPair<IEngineModule*, PlatformModule>& Pair = Modules[Index];
        
        IEngineModule* EngineModule = Pair.First;
        if ( EngineModule )
        {
            EngineModule->Unload();
        }

        HMODULE Module = Pair.Second;
        FreeLibrary( Module );

        Modules.RemoveAt(Index);
        ModuleNames.RemoveAt(Index);
    }
}

void CModuleManager::ReleaseAllModules()
{
    for ( int32 Index = 0; Index < Modules.Size(); Index++ )
    {
        const TPair<IEngineModule*, PlatformModule>& Pair = Modules[Index];

        IEngineModule* EngineModule = Pair.First;
        if ( EngineModule )
        {
            EngineModule->Unload();
        }

        HMODULE Module = Pair.Second;
        FreeLibrary( Module );
    }

    Modules.Clear();
    ModuleNames.Clear();
}

int32 CModuleManager::GetModuleIndex( const char* ModuleName )
{
    for ( int32 Index = 0; Index < ModuleNames.Size(); Index++ )
    {
        const CString& Name = ModuleNames[Index];
        if ( Name == ModuleName )
        {
            return Index;
        }
    }

    return -1;
}
