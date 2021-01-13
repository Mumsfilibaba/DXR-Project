#include "RenderingAPI.h"
#include "CommandList.h"
#include "Shader.h"

#include "D3D12/D3D12RenderingAPI.h"
#include "D3D12/D3D12ShaderCompiler.h"

/*
* RenderingAPI
*/

bool RenderingAPI::Init(ERenderingAPI InRenderAPI)
{
	// Select RenderingAPI
	if (InRenderAPI == ERenderingAPI::RenderingAPI_D3D12)
	{
		GlobalRenderingAPI = DBG_NEW D3D12RenderingAPI();
		
		D3D12ShaderCompiler* Compiler = DBG_NEW D3D12ShaderCompiler();
		if (!Compiler->Init())
		{
			return false;
		}

		GlobalShaderCompiler = Compiler;
	}
	else
	{
		LOG_ERROR("[RenderingAPI::Init] Invalid RenderingAPI enum");
		
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
	if (GlobalRenderingAPI->Init(EnableDebug))
	{
		ICommandContext* CmdContext = GlobalRenderingAPI->GetDefaultCommandContext();
		CommandListExecutor::SetContext(CmdContext);

		return true;
	}
	else
	{
		return false;
	}
}

void RenderingAPI::Release()
{
	SAFEDELETE(GlobalRenderingAPI);
	SAFEDELETE(GlobalShaderCompiler);
}
