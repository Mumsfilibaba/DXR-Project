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
	GlobalConsoleOutput = PlatformOutputDevice::Make();

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
	GlobalIsRunning = true;
	return true;
}

void EngineLoop::PreTick()
{
	GlobalClock.Tick();

	if (!PlatformApplication::PollPlatformEvents())
	{
		Exit();
	}
}

void EngineLoop::Tick()
{
	GlobalGame->Tick(GlobalClock.GetDeltaTime());

	Editor::Tick();
}

void EngineLoop::PostTick()
{
	GlobalRenderer->Tick(*Scene::GetCurrentScene());
}

void EngineLoop::PreRelease()
{
	CommandListExecutor::WaitForGPU();
	
	TextureFactory::Release();
}

void EngineLoop::Release()
{
	SAFEDELETE(GlobalGame);

	DebugUI::Release();

	SAFEDELETE(GlobalRenderer);

	RenderingAPI::Release();
}

void EngineLoop::PostRelease()
{
	SAFEDELETE(GlobalEventDispatcher);

	GlobalMainWindow->Release();

	SAFEDELETE(GlobalPlatformApplication);
	SAFEDELETE(GlobalConsoleOutput);
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
