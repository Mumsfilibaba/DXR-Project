#include "ModuleManger.h"

#include "Core/Windows/Windows.h"
#include "Core/Windows/Windows.inl"

#include "Core/Templates/StringTraits.h"

CORE_API CModuleManger GModuleManager;

CModuleManger::CModuleManger()
    : Modules()
{
}

IEngineModule* CModuleManger::LoadModule( const char* ModuleName )
{
    Assert( ModuleName != nullptr );

    // TODO: Abstract the Win-Api
    HMODULE Module = LoadLibraryA(ModuleName);
    if ( Module == NULL )
    {
        LOG_ERROR( "Failed to find module '" + CString( ModuleName ) + "'");
        return nullptr;
    }

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

    TPair<CString, IEngineModule*> NewPair( ModuleName, NewModule );
    int32 Index = Modules.Find( NewPair, []( const TPair<CString, IEngineModule*>& LHS, const TPair<CString, IEngineModule*>& RHS ) -> bool
    {
        return (LHS.First == RHS.First) && (CStringTraits::Compare( LHS.Second->GetName(), RHS.Second->GetName() ) == 0);
    } );

    if ( Index >= 0 )
    {
        LOG_WARNING( "Module'" + CString( ModuleName ) + "' is already loaded" );
        return Modules[Index].Second;
    }
    else
    {
        LOG_INFO( "Loaded module'" + CString( ModuleName ) + "'" );
        
        Modules.Emplace( NewPair );
        return NewPair.Second;
    }
}

IEngineModule* CModuleManger::GetModule( const char* ModuleName )
{
    return nullptr;
}

void CModuleManger::ReleaseModule( const char* ModuleName )
{
}

void CModuleManger::ReleaseAllModule()
{
}
