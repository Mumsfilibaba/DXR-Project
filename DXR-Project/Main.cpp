#include "Application/Platform/PlatformDialogMisc.h"

#include "Engine/EngineLoop.h"

#include "Memory/Memory.h"

#include <crtdbg.h>

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
#ifdef _DEBUG
	Memory::SetDebugFlags(EMemoryDebugFlag::MemoryDebugFlag_LeakCheck);
#endif

	if (!EngineLoop::PreInit())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Pre-Initialize Failed");
		return -1;
	}

	if (!EngineLoop::Init())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Initialize Failed");
		return -1;
	}

	if (!EngineLoop::PostInit())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Post-Initialize Failed");
		return -1;
	}

	while (EngineLoop::IsRunning())
	{
		EngineLoop::PreTick();
	
		EngineLoop::Tick();
	
		EngineLoop::PostTick();
	}

	EngineLoop::PreRelease();
	
	EngineLoop::Release();
	
	EngineLoop::PostRelease();

	return 0;
}

#pragma warning(pop)