#include "RenderingAPI.h"
#include "CommandList.h"
#include "Shader.h"

#include "D3D12/D3D12RenderingAPI.h"
#include "D3D12/D3D12ShaderCompiler.h"

#define ENABLE_API_DEBUGGING 1

/*
* RenderingAPI
*/

CommandListExecutor				RenderingAPI::CmdExecutor			= CommandListExecutor();
TSharedPtr<GenericRenderingAPI>	RenderingAPI::ActiveRenderingAPI	= nullptr;

bool RenderingAPI::Initialize(ERenderingAPI InRenderAPI, TSharedRef<GenericWindow> RenderWindow)
{
	// Select RenderingAPI
	if (InRenderAPI == ERenderingAPI::RenderingAPI_D3D12)
	{
		ActiveRenderingAPI = new D3D12RenderingAPI();
		
		D3D12ShaderCompiler* Compiler = new D3D12ShaderCompiler();
		if (!Compiler->Initialize())
		{
			return false;
		}

		ShaderCompiler::Instance = TSharedPtr<D3D12ShaderCompiler>(Compiler);
	}
	else
	{
		LOG_ERROR("[RenderingAPI::Initialize] Invalid RenderingAPI enum");
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
	if (ActiveRenderingAPI->Init(RenderWindow, EnableDebug))
	{
		ICommandContext* CmdContext = ActiveRenderingAPI->GetDefaultCommandContext();
		CmdExecutor.SetContext(CmdContext);

		return true;
	}
	else
	{
		return false;
	}
}
