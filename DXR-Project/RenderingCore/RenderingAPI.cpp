#include "RenderingAPI.h"
#include "CommandList.h"

#include "D3D12/D3D12RenderingAPI.h"

#define ENABLE_API_DEBUGGING 1

/*
* RenderingAPI
*/

bool RenderingAPI::Initialize(ERenderingAPI InRenderAPI, TSharedRef<GenericWindow> RenderWindow)
{
	// Select RenderingAPI
	if (InRenderAPI == ERenderingAPI::RenderingAPI_D3D12)
	{
		EngineGlobals::RenderingAPI = new D3D12RenderingAPI();
	}
	else
	{
		LOG_ERROR("[RenderingAPI::Initialize] Invalid RenderingAPI enum");
		Debug::DebugBreak();

		return false;
	}

	const bool EnableDebug =
#if ENABLE_API_DEBUGGING
		true;
#else
		false;
#endif

	// Init
	if (EngineGlobals::RenderingAPI->Init(RenderWindow, EnableDebug))
	{
		EngineGlobals::CmdListExecutor = MakeShared<CommandListExecutor>();

		ICommandContext* CmdContext = EngineGlobals::RenderingAPI->GetDefaultCommandContext();
		EngineGlobals::CmdListExecutor->SetContext(CmdContext);

		return true;
	}
	else
	{
		return false;
	}
}
