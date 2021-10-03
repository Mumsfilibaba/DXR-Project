#include "RHIModule.h"
#include "RHICommandList.h"

#if defined(PLATFORM_WINDOWS)
#include "D3D12/D3D12RHICore.h"
#include "D3D12/D3D12ShaderCompiler.h"
#else
#include "RHICore/ShaderCompiler.h"
#endif

bool CRHIModule::Init( ERenderLayerApi InRenderApi )
{
#if defined(PLATFORM_WINDOWS)
    // Select RenderLayer
    if ( InRenderApi == ERenderLayerApi::D3D12 )
    {
        GRHICore = DBG_NEW CD3D12RHICore();

        CD3D12ShaderCompiler* Compiler = DBG_NEW CD3D12ShaderCompiler();
        if ( !Compiler->Init() )
        {
            return false;
        }

        GShaderCompiler = Compiler;
    }
    else
    #elif defined(PLATFORM_MACOS)
    if ( InRenderApi == ERenderLayerApi::Unknown )
    {
        LOG_WARNING( "[RenderLayer::Init] No RenderAPI available for MacOS" );
        return true;
    }
    else
    #endif
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
    if ( GRHICore->Init( EnableDebug ) )
    {
        IRHICommandContext* CmdContext = GRHICore->GetDefaultCommandContext();
        GCommandQueue.SetContext( CmdContext );

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
