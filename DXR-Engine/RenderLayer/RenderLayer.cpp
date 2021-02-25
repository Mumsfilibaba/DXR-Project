#include "RenderLayer.h"
#include "CommandList.h"

#include "D3D12/D3D12RenderLayer.h"
#include "D3D12/D3D12ShaderCompiler.h"

Bool RenderLayer::Init(ERenderLayerApi InRenderApi)
{
    // Select RenderLayer
    if (InRenderApi == ERenderLayerApi::D3D12)
    {
        gRenderLayer = DBG_NEW D3D12RenderLayer();
        
        D3D12ShaderCompiler* Compiler = DBG_NEW D3D12ShaderCompiler();
        if (!Compiler->Init())
        {
            return false;
        }

        gShaderCompiler = Compiler;
    }
    else
    {
        LOG_ERROR("[RenderLayer::Init] Invalid RenderLayer enum");
        
        Debug::DebugBreak();
        return false;
    }

    // TODO: This should be in EngineConfig
    const Bool EnableDebug =
#if ENABLE_API_DEBUGGING
        true;
#else
        false;
#endif

    // Init
    if (gRenderLayer->Init(EnableDebug))
    {
        ICommandContext* CmdContext = gRenderLayer->GetDefaultCommandContext();
        gCmdListExecutor.SetContext(CmdContext);

        return true;
    }
    else
    {
        return false;
    }
}

void RenderLayer::Release()
{
    SafeDelete(gRenderLayer);
    SafeDelete(gShaderCompiler);
}
