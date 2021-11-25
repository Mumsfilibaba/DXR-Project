#include "RHIModule.h"
#include "RHICore.h"
#include "RHICommandList.h"
#include "RHIShaderCompiler.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

RHI_API CRHICore* GRHICore = nullptr;
RHI_API IRHIShaderCompiler* GShaderCompiler = nullptr;

///////////////////////////////////////////////////////////////////////////////////////////////////

static CRHIModule* LoadNullRHI()
{
    return CModuleManager::Get().LoadEngineModule<CRHIModule>( "NullRHI" );
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool InitRHI( ERHIModule InRenderApi )
{
    // Load Selected RHI
    CRHIModule* RHIModule = nullptr;
    if ( InRenderApi == ERHIModule::D3D12 )
    {
        RHIModule = CModuleManager::Get().LoadEngineModule<CRHIModule>( "D3D12RHI" );
    }
    else if ( InRenderApi == ERHIModule::Null )
    {
        RHIModule = LoadNullRHI();
    }

    // If the selected RHI failed, try load the NullRHI
    if ( !RHIModule )
    {
        LOG_WARNING( "[InitRHI] Failed to load RHI, trying to load NullRHI" );

        RHIModule = LoadNullRHI();
        if ( !RHIModule )
        {
            LOG_ERROR( "[InitRHI] Failed to load RHI and fallback, the application has to terminate" );
            return false;
        }
    }

    // Init RHI objects

    // TODO: This should be in EngineConfig and/or CCommandLine
    const bool EnableDebug =
    #if ENABLE_API_DEBUGGING
        true;
#else
        false;
#endif

    CRHICore* RHICore = RHIModule->CreateCore();
    if ( !(RHICore && RHICore->Init( EnableDebug )) )
    {
        LOG_ERROR( "[InitRHI] Failed to init RHICore, the application has to terminate" );
        return false;
    }

    GRHICore = RHICore;

    IRHIShaderCompiler* Compiler = RHIModule->CreateCompiler();
    if ( !Compiler )
    {
        LOG_ERROR( "[InitRHI] Failed to init RHIShaderCompiler, the application has to terminate" );
        return false;
    }

    GShaderCompiler = Compiler;

    // Set the context to the command queue
    IRHICommandContext* CmdContext = GRHICore->GetDefaultCommandContext();
    CRHICommandQueue::Get().SetContext( CmdContext );

    return true;
}

void ReleaseRHI()
{
    CRHICommandQueue::Get().SetContext( nullptr );

    SafeDelete( GRHICore );
    SafeDelete( GShaderCompiler );
}
