#include "Application/Application.h"
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

	if (!EngineLoop::PreInitialize())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Pre-Initialize Failed");
		return -1;
	}

	if (!EngineLoop::Initialize())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Initialize Failed");
		return -1;
	}

	if (!EngineLoop::PostInitialize())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Post-Initialize Failed");
		return -1;
	}

	// Run loop
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