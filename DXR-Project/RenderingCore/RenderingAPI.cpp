#include "RenderingAPI.h"
#include "CommandList.h"

#include "D3D12/D3D12RenderingAPI.h"

/*
* RenderingAPI
*/

bool RenderingAPI::Initialize(ERenderingAPI InRenderAPI)
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

	// Init
	if (EngineGlobals::RenderingAPI->Init())
	{
		ICommandContext* CmdContext = EngineGlobals::RenderingAPI->GetCommandContext();
		EngineGlobals::CmdListExecutor->SetContext(CmdContext);

		return true;
	}
	else
	{
		return false;
	}
}

void RenderingAPI::Release()
{
	// TODO: Fix so that there is not crash when exiting
	//SAFEDELETE(CurrentRenderAPI);
}
