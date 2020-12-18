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

	// Init core modules
	if (!EngineLoop::PreInitialize())
	{
		::MessageBox(0, "Pre-Initialize Failed", "ERROR", MB_ICONERROR);
		return -1;
	}

	// Main init
	if (!EngineLoop::Initialize())
	{
		::MessageBox(0, "Failed to initalize", "ERROR", MB_ICONERROR);
		return -1;
	}

	// Handle post init
	if (!EngineLoop::PostInitialize())
	{
		::MessageBox(0, "Post-Initialize Failed ", "ERROR", MB_ICONERROR);
		return -1;
	}

	// Run loop
	while (EngineLoop::IsRunning())
	{
		// Prepare for tick
		EngineLoop::PreTick();
		// Update engine
		EngineLoop::Tick();
		// Handle post tick
		EngineLoop::PostTick();
	}

	// Relrase minor resources
	EngineLoop::PreRelease();
	// Main release of resources
	EngineLoop::Release();
	// Release core features
	EngineLoop::PostRelease();

	return 0;
}

#pragma warning(pop)