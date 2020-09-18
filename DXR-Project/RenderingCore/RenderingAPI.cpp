#include "RenderingAPI.h"

#include "D3D12/D3D12RenderingAPI.h"

RenderingAPI* RenderingAPI::RenderAPI = nullptr;

RenderingAPI* RenderingAPI::Make(ERenderingAPI InRenderAPI, TSharedPtr<WindowsWindow> RendererWindow, bool EnableDebug)
{
	// Select RenderingAPI
	TUniquePtr<RenderingAPI> TempRenderAPI;
	if (InRenderAPI == ERenderingAPI::RENDERING_API_D3D12)
	{
		return new D3D12RenderingAPI());
	}
	else
	{
		return nullptr;
	}
}

RenderingAPI* RenderingAPI::Get()
{
	return RenderAPI;
}

void RenderingAPI::Release()
{
	// TODO: Fix so that there is not crash when exiting
	//SAFEDELETE(RenderAPI);
}
