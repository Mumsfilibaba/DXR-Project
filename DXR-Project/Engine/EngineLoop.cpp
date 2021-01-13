#include "EngineLoop.h"
#include "EngineGlobals.h"

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

/*
* Engineloop Globals
*/

static Clock	GlobalClock;
static Bool		GlobalIsRunning = false;

/*
* EngineLoop
*/

Bool EngineLoop::PreInit()
{
	GlobalProfiler = DBG_NEW Profiler();
	
	{
		TRACE_FUNCTION_SCOPE();

		GlobalConsoleOutput = PlatformOutputDevice::Make();

		GlobalPlatformApplication = PlatformApplication::Make();
		if (!GlobalPlatformApplication->Init())
		{
			PlatformDialogMisc::MessageBox("ERROR", "Failed to create Platform Application");
			return false;
		}
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
	if (!RenderingAPI::Init(ERenderingAPI::RenderingAPI_D3D12))
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

	GlobalGame = DBG_NEW Game();
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

	GlobalIsRunning = true;
	return true;
}

void EngineLoop::PreTick()
{
	TRACE_FUNCTION_SCOPE();

	GlobalProfiler->BeginFrame();

	GlobalClock.Tick();

	if (!PlatformApplication::PollPlatformEvents())
	{
		Exit();
	}

	GlobalPlatformApplication->Tick();
}

void EngineLoop::Tick()
{
	TRACE_FUNCTION_SCOPE();

	GlobalGame->Tick(GlobalClock.GetDeltaTime());

	Editor::Tick();
}

void EngineLoop::PostTick()
{
	TRACE_FUNCTION_SCOPE();

	GlobalRenderer->Tick(*Scene::GetCurrentScene());

	GlobalProfiler->EndFrame();
}

void EngineLoop::PreRelease()
{
	TRACE_FUNCTION_SCOPE();

	CommandListExecutor::WaitForGPU();
	
	TextureFactory::Release();
}

void EngineLoop::Release()
{
	TRACE_FUNCTION_SCOPE();

	SAFEDELETE(GlobalGame);

	DebugUI::Release();

	SAFEDELETE(GlobalRenderer);

	RenderingAPI::Release();
}

void EngineLoop::PostRelease()
{
	{
		TRACE_FUNCTION_SCOPE();

		SAFEDELETE(GlobalEventDispatcher);

		GlobalMainWindow->Release();

		SAFEDELETE(GlobalPlatformApplication);
		SAFEDELETE(GlobalConsoleOutput);
	}

	SAFEDELETE(GlobalProfiler);
}

void EngineLoop::Exit()
{
	GlobalIsRunning = false;
}

Bool EngineLoop::IsRunning()
{
	return GlobalIsRunning;
}

Timestamp EngineLoop::GetDeltaTime()
{
	return GlobalClock.GetDeltaTime();
}

Timestamp EngineLoop::GetTotalElapsedTime()
{
	return GlobalClock.GetTotalTime();
}
