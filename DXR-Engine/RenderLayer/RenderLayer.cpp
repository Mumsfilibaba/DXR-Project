#include "RenderLayer.h"
#include "CommandList.h"

#if defined(PLATFORM_WINDOWS)
#include "D3D12/D3D12RenderLayer.h"
#include "D3D12/D3D12ShaderCompiler.h"
#else
#include "RenderLayer/ShaderCompiler.h"
#endif

bool RenderLayer::Init( ERenderLayerApi InRenderApi )
{
#if defined(PLATFORM_WINDOWS)
    // Select RenderLayer
    if ( InRenderApi == ERenderLayerApi::D3D12 )
    {
        GRenderLayer = DBG_NEW D3D12RenderLayer();

        D3D12ShaderCompiler* Compiler = DBG_NEW D3D12ShaderCompiler();
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

        Debug::DebugBreak();
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
    if ( GRenderLayer->Init( EnableDebug ) )
    {
        ICommandContext* CmdContext = GRenderLayer->GetDefaultCommandContext();
        GCmdListExecutor.SetContext( CmdContext );

        return true;
    }
    else
    {
        return false;
    }
}

void RenderLayer::Release()
{
    SafeDelete( GRenderLayer );
    SafeDelete( GShaderCompiler );
}
