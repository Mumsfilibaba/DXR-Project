#include "EngineLoop.h"
#include "EngineGlobals.h"

#include "Time/Clock.h"

#include "Application/Application.h"
#include "Application/Generic/GenericOutputDevice.h"
#include "Application/Generic/GenericCursor.h"

#include "Application/Platform/PlatformApplication.h"
#include "Application/Platform/PlatformDialogMisc.h"

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
static bool		GlobalIsRunning = false;

/*
* EngineLoop
*/

bool EngineLoop::PreInitialize()
{
	GlobalOutputDevices::Initialize();

	GlobalPlatformApplication = PlatformApplication::Make();
	if (!GlobalPlatformApplication->Initialize())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Failed to create Platform Application");
		return false;
	}

	return true;
}

bool EngineLoop::Initialize()
{
	// Application
	Application* App = Application::Make(GlobalPlatformApplication);
	if (!App->Initialize())
	{
		PlatformDialogMisc::MessageBox("ERROR", "Failed to create Application");
		return false;
	}

	// Cursors
	GlobalCursors::Initialize();

	// RenderAPI
	if (!RenderingAPI::Initialize(ERenderingAPI::RenderingAPI_D3D12))
	{
		return false;
	}

	// TextureFactory
	if (!TextureFactory::Initialize())
	{
		return false;
	}

	// Renderer
	GlobalRenderer = new Renderer();
	if (!GlobalRenderer->Init())
	{
		PlatformDialogMisc::MessageBox("ERROR", "FAILED to create Renderer");
		return false;
	}

	// ImGui
	if (!DebugUI::Initialize())
	{
		PlatformDialogMisc::MessageBox("ERROR", "FAILED to create ImGuiContext");
		return false;
	}

	// Game
	Game* GameInstance = new Game();
	if (!GameInstance->Initialize())
	{
		PlatformDialogMisc::MessageBox("ERROR", "FAILED initialize Game");
		return false;
	}
	else
	{
		Game::SetCurrent(GameInstance);
	}

	GlobalIsRunning = true;
	return true;
}

bool EngineLoop::PostInitialize()
{
	// Empty for now
	return true;
}

void EngineLoop::PreTick()
{
	// Empty for now
}

void EngineLoop::Tick()
{
	GlobalClock.Tick();

	// Application
	Application::Get().Tick();

	// Update Game
	Game::GetCurrent().Tick(GlobalClock.GetDeltaTime());

	// Update renderer
	GlobalRenderer->Tick(*Scene::GetCurrentScene());

	// Update editor
	Editor::Tick();
}

void EngineLoop::PostTick()
{
	// Empty for now
}

void EngineLoop::PreRelease()
{
	TextureFactory::Release();
}

void EngineLoop::Release()
{
	CommandListExecutor::WaitForGPU();

	// Destroy game instance
	Game::GetCurrent().Destroy();
	Game::SetCurrent(nullptr);

	DebugUI::Release();

	SAFEDELETE(GlobalRenderer);
}

void EngineLoop::PostRelease()
{
	SAFEDELETE(GlobalPlatformApplication);

	GlobalOutputDevices::Release();
}

void EngineLoop::Exit()
{
	GlobalIsRunning = false;
}

bool EngineLoop::IsRunning()
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
