#include "Application/Application.h"
#include "Application/EngineLoop.h"

#include <crtdbg.h>

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	if (!EngineLoop::CoreInitialize())
	{
		::MessageBox(0, "Failed to initalize core modules", "ERROR", MB_ICONERROR);
		return -1;
	}

	if (!EngineLoop::Initialize())
	{
		::MessageBox(0, "Failed to initalize core modules", "ERROR", MB_ICONERROR);
		return -1;
	}

	while (EngineLoop::IsRunning())
	{
		EngineLoop::Tick();
	}

	EngineLoop::Release();
	EngineLoop::CoreRelease();

	return 0;
}

#pragma warning(pop)