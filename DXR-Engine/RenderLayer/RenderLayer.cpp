#include "RenderLayer.h"
#include "CommandList.h"
#include "Shader.h"

#include "D3D12/D3D12RenderLayer.h"
#include "D3D12/D3D12ShaderCompiler.h"

/*
* RenderLayer
*/

bool RenderLayer::Init(ERenderLayerApi InRenderApi)
{
	// Select RenderLayer
	if (InRenderApi == ERenderLayerApi::RenderLayerApi_D3D12)
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
	const bool EnableDebug =
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
	SAFEDELETE(gRenderLayer);
	SAFEDELETE(gShaderCompiler);
}
