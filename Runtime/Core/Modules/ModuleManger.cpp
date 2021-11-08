#include "ModuleManger.h"

#include "Core/Windows/Windows.h"
#include "Core/Windows/Windows.inl"
#include "Core/Templates/StringTraits.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

CModuleManager CModuleManager::Instance;

///////////////////////////////////////////////////////////////////////////////////////////////////

IEngineModule* CModuleManager::LoadEngineModule( const char* ModuleName )
{
    Assert( ModuleName != nullptr );

    IEngineModule* ExistingModule = GetEngineModule( ModuleName );
    if ( ExistingModule )
    {
        LOG_WARNING( "Module '" + CString( ModuleName ) + "' is already loaded" );
        return ExistingModule;
    }

    PlatformModule Module = PlatformLibrary::LoadDynamicLib( ModuleName );
    if ( !Module )
    {
        LOG_ERROR( "Failed to find module '" + CString( ModuleName ) + "'" );
        return nullptr;
    }

    // Requires that the module has a LoadEngineModule function exported
    PFNLoadEngineModule LoadEngineModule = PlatformLibrary::LoadSymbolAddress<PFNLoadEngineModule>( "LoadEngineModule", Module );
    if ( !LoadEngineModule )
    {
        PlatformLibrary::FreeDynamicLib( Module );
        return nullptr;
    }

    // The pointer is owned by the ModuleManager and should not be released anywhere else
    IEngineModule* NewModule = LoadEngineModule();
    if ( !NewModule || (NewModule && !NewModule->Load()) )
    {
        LOG_ERROR( "Failed to load module '" + CString( ModuleName ) + "', resulting interface was nullptr" );
        PlatformLibrary::FreeDynamicLib( Module );

        return nullptr;
    }
    else
    {
        LOG_INFO( "Loaded module'" + CString( ModuleName ) + "'" );

        // Broadcast to engine systems that a new module was loaded
        ModuleLoadedDelegate.Broadcast( ModuleName, NewModule );

        // Add module in the module list
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
            LOG_WARNING( "Module is loaded but does not contain an EngineModule interface" );
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
    int32 Index = GetModuleIndex( ModuleName );
    if ( Index >= 0 )
    {
        const TPair<IEngineModule*, PlatformModule>& Pair = Modules[Index];

        IEngineModule* EngineModule = Pair.First;
        if ( EngineModule )
        {
            EngineModule->Unload();
        }

        PlatformModule Module = Pair.Second;
        PlatformLibrary::FreeDynamicLib( Module );

        Modules.RemoveAt( Index );
        ModuleNames.RemoveAt( Index );
    }
}

void CModuleManager::ReleaseAllModules()
{
    const int32 NumModules = Modules.Size();
    for ( int32 Index = 0; Index < NumModules; Index++ )
    {
        const TPair<IEngineModule*, PlatformModule>& Pair = Modules[Index];

        IEngineModule* EngineModule = Pair.First;
        if ( EngineModule )
        {
            EngineModule->Unload();

            // The pointer is owned by the ModuleManager and should not be released anywhere else
            SafeDelete( EngineModule );
        }

        PlatformModule Module = Pair.Second;
        PlatformLibrary::FreeDynamicLib( Module );
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
