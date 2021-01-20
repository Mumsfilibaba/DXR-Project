#include "EngineLoop.h"

#include "Engine/EngineGlobals.h"

#include "Time/Clock.h"

#include "Application/Generic/GenericOutputDevice.h"
#include "Application/Generic/GenericCursor.h"

#include "Application/Platform/PlatformApplication.h"
#include "Application/Platform/PlatformDialogMisc.h"
#include "Application/Events/EventDispatcher.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"
#include "Rendering/TextureFactory.h"

#include "RenderingCore/Texture.h"
#include "RenderingCore/CommandList.h"

#include "Editor/Editor.h"

#include "Game/Game.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include "Memory/Memory.h"

/*
* EngineMain 
*/

Int32 EngineMain(const TArrayView<const Char*> Args)
{
	UNREFERENCED_VARIABLE(Args);

#ifdef _DEBUG
	Memory::SetDebugFlags(EMemoryDebugFlag::MemoryDebugFlag_LeakCheck);
#endif

	if (!GlobalEngineLoop.PreInit())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Pre-Initialize Failed");
		return -1;
	}

	if (!GlobalEngineLoop.Init())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Initialize Failed");
		return -1;
	}

	if (!GlobalEngineLoop.PostInit())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Post-Initialize Failed");
		return -1;
	}

	while (GlobalEngineLoop.IsRunning())
	{
		TRACE_SCOPE("Tick");
			
		GlobalEngineLoop.PreTick();
	
		GlobalEngineLoop.Tick();
	
		GlobalEngineLoop.PostTick();
	}

	GlobalEngineLoop.PreRelease();
	
	GlobalEngineLoop.Release();
	
	GlobalEngineLoop.PostRelease();

	return 0;
}

/*
* EngineLoop
*/

Bool EngineLoop::PreInit()
{
	TRACE_FUNCTION_SCOPE();

	GlobalConsoleOutput = PlatformOutputDevice::Make();
	if (!GlobalConsoleOutput)
	{
		return false;
	}
	else
	{
		GlobalConsoleOutput->SetTitle("DXR-Engine Error Output");
	}

	GlobalPlatformApplication = PlatformApplication::Make();
	if (!GlobalPlatformApplication->Init())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Failed to create Platform Application");
		return false;
	}

	return true;
}

Bool EngineLoop::Init()
{
	TRACE_FUNCTION_SCOPE();

	GlobalEventDispatcher = DBG_NEW EventDispatcher(GlobalPlatformApplication);
	GlobalPlatformApplication->SetEventHandler(GlobalEventDispatcher);

	const UInt32 Style =
		WindowStyleFlag_Titled		|
		WindowStyleFlag_Closable	|
		WindowStyleFlag_Minimizable |
		WindowStyleFlag_Maximizable |
		WindowStyleFlag_Resizeable;

	WindowCreateInfo WinCreateInfo("DXR Engine", 1920, 1080, Style);
	GlobalMainWindow = GlobalPlatformApplication->MakeWindow();
	
	if (!GlobalMainWindow->Init(WinCreateInfo))
	{
		PlatformDialogMisc::MessageBox("ERROR", "Failed to create Application");
		return false;
	}
	else
	{
		GlobalMainWindow->Show(false);
	}

	GlobalCursors::Init();

	// RenderAPI
	if (!RenderLayer::Init(ERenderLayerApi::RenderLayerApi_D3D12))
	{
		return false;
	}

	if (!TextureFactory::Init())
	{
		return false;
	}

	GlobalRenderer = DBG_NEW Renderer();
	if (!GlobalRenderer->Init())
	{
		PlatformDialogMisc::MessageBox("ERROR", "FAILED to create Renderer");
		return false;
	}

	if (!DebugUI::Init())
	{
		PlatformDialogMisc::MessageBox("ERROR", "FAILED to create ImGuiContext");
		return false;
	}

	GlobalGame = MakeGameInstance();
	if (!GlobalGame->Init())
	{
		PlatformDialogMisc::MessageBox("ERROR", "FAILED initialize Game");
		return false;
	}

	return true;
}

Bool EngineLoop::PostInit()
{
	TRACE_FUNCTION_SCOPE();

	GlobalConsole.Init();

	ShouldRun = true;
	return true;
}

void EngineLoop::PreTick()
{
	TRACE_FUNCTION_SCOPE();

	if (!PlatformApplication::CheckWaitingPlatformEvents())
	{
		Exit();
	}

	GlobalPlatformApplication->Tick();
}

void EngineLoop::Tick()
{
	TRACE_FUNCTION_SCOPE();

	EngineClock.Tick();
	
	GlobalGame->Tick(EngineClock.GetDeltaTime());

	GlobalConsole.Tick();
}

void EngineLoop::PostTick()
{
	TRACE_FUNCTION_SCOPE();

	GlobalProfiler.Tick();

	GlobalRenderer->Tick(*Scene::GetCurrentScene());
}

Bool EngineLoop::PreRelease()
{
	TRACE_FUNCTION_SCOPE();

	GlobalCmdListExecutor.WaitForGPU();
	
	TextureFactory::Release();

	return true;
}

Bool EngineLoop::Release()
{
	TRACE_FUNCTION_SCOPE();

	SAFEDELETE(GlobalGame);

	DebugUI::Release();

	SAFEDELETE(GlobalRenderer);

	RenderLayer::Release();

	return true;
}

Bool EngineLoop::PostRelease()
{
	TRACE_FUNCTION_SCOPE();

	SAFEDELETE(GlobalEventDispatcher);

	GlobalMainWindow->Release();

	SAFEDELETE(GlobalPlatformApplication);
	SAFEDELETE(GlobalConsoleOutput);

	return true;
}

void EngineLoop::Exit()
{
	ShouldRun = false;
}

Bool EngineLoop::IsRunning() const
{
	return ShouldRun;
}

Timestamp EngineLoop::GetDeltaTime() const
{
	return EngineClock.GetDeltaTime();
}

Timestamp EngineLoop::GetTotalElapsedTime() const
{
	return EngineClock.GetTotalTime();
}
