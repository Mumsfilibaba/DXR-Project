#include "Application/Application.h"

#include "Engine/EngineLoop.h"

#include "Memory/Memory.h"

#include <crtdbg.h>

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
#ifdef _DEBUG
	Memory::SetDebugFlags(EMemoryDebugFlag::MEMORY_DEBUG_FLAGS_LEAK_CHECK);
#endif

	if (!EngineLoop::CoreInitialize())
	{
		::MessageBox(0, "Failed to initalize core modules", "ERROR", MB_ICONERROR);
		return -1;
	}

	if (!EngineLoop::Initialize())
	{
		::MessageBox(0, "Failed to initalize", "ERROR", MB_ICONERROR);
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