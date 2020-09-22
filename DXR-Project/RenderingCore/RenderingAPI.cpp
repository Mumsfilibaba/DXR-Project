#include "RenderingAPI.h"

#include "D3D12/D3D12RenderingAPI.h"

RenderingAPI* RenderingAPI::CurrentRenderAPI = nullptr;

/*
* RenderingAPI
*/
RenderingAPI* RenderingAPI::Make(ERenderingAPI InRenderAPI)
{
	// Select RenderingAPI
	if (InRenderAPI == ERenderingAPI::RENDERING_API_D3D12)
	{
		CurrentRenderAPI = new D3D12RenderingAPI();
		return CurrentRenderAPI;
	}
	else
	{
		return nullptr;
	}
}

RenderingAPI& RenderingAPI::Get()
{
	VALIDATE(CurrentRenderAPI);
	return *CurrentRenderAPI;
}

void RenderingAPI::Release()
{
	// TODO: Fix so that there is not crash when exiting
	//SAFEDELETE(CurrentRenderAPI);
}
