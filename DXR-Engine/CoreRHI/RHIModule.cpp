#include "RHIModule.h"
#include "RHICommandList.h"

#include "NullRHI/NullRHICore.h"

#if defined(PLATFORM_WINDOWS)
#include "D3D12RHI/D3D12RHICore.h"
#include "D3D12RHI/D3D12ShaderCompiler.h"
#else
#include "CoreRHI/ShaderCompiler.h"
#endif

CORE_API CRHICore*           GRHICore        = nullptr;
CORE_API IRHIShaderCompiler* GShaderCompiler = nullptr;

bool CRHIModule::Init( ERHIModule InRenderApi )
{
#if defined(PLATFORM_WINDOWS)
    if ( InRenderApi == ERHIModule::D3D12 )
    {
        GRHICore = CD3D12RHICore::Make();

        CD3D12RHIShaderCompiler* Compiler = dbg_new CD3D12RHIShaderCompiler();
        if ( !Compiler->Init() )
        {
            return false;
        }

        GShaderCompiler = Compiler;
    }
    else
    #endif
        if ( InRenderApi == ERHIModule::Null )
        {
            GRHICore = CNullRHICore::Make();

            LOG_WARNING( "[RenderLayer::Init] Initialized NullRHI" );
        }
        else
        {
            LOG_ERROR( "[RenderLayer::Init] Invalid RenderLayer enum" );

            CDebug::DebugBreak();
            return false;
        }

    // TODO: This should be in EngineConfig
    const bool EnableDebug =
    #if ENABLE_API_DEBUGGING
        true;
#else
        false;
#endif

    // Init
    if ( GRHICore && GRHICore->Init( EnableDebug ) )
    {
        IRHICommandContext* CmdContext = GRHICore->GetDefaultCommandContext();
        CRHICommandQueue::Get().SetContext( CmdContext );

        return true;
    }
    else
    {
        return false;
    }
}

void CRHIModule::Release()
{
    SafeDelete( GRHICore );
    SafeDelete( GShaderCompiler );
}
