#include "EngineLoop.h"
#include "EngineGlobals.h"

#include "Time/Clock.h"

#include "Application/Application.h"
#include "Application/Generic/GenericOutputDevice.h"
#include "Application/Generic/GenericCursor.h"
#include "Application/Platform/PlatformApplication.h"

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

	EngineGlobals::PlatformApplication = TSharedPtr(PlatformApplication::Make());
	if (!EngineGlobals::PlatformApplication->Initialize())
	{
		::MessageBox(0, "Failed to create Platform Application", "ERROR", MB_ICONERROR);
		return false;
	}

	return true;
}

bool EngineLoop::Initialize()
{
	// Application
	Application* App = Application::Make();
	if (!App->Initialize(EngineGlobals::PlatformApplication))
	{
		::MessageBox(0, "Failed to create Application", "ERROR", MB_ICONERROR);
		return false;
	}

	// Cursors
	GlobalCursors::Initialize();

	// RenderAPI
	if (!RenderingAPI::Initialize(ERenderingAPI::RenderingAPI_D3D12, App->GetMainWindow()))
	{
		return false;
	}

	// TextureFactory
	if (!TextureFactory::Initialize())
	{
		return false;
	}

	// Renderer
	Renderer* Renderer = Renderer::Make();
	if (!Renderer)
	{
		::MessageBox(0, "FAILED to create Renderer", "ERROR", MB_ICONERROR);
		return false;
	}

	// ImGui
	if (!DebugUI::Initialize())
	{
		::MessageBox(0, "FAILED to create ImGuiContext", "ERROR", MB_ICONERROR);
		return false;
	}

	// Game
	Game* GameInstance = new Game();
	if (!GameInstance->Initialize())
	{
		::MessageBox(0, "FAILED initialize Game", "ERROR", MB_ICONERROR);
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
	Renderer::Get()->Tick(*Scene::GetCurrentScene());

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

	Application::Get().Release();

	EngineGlobals::PlatformApplication.Reset();

	GlobalOutputDevices::Release();
}

void EngineLoop::Release()
{
	// Destroy game instance
	Game::GetCurrent().Destroy();
	Game::SetCurrent(nullptr);

	DebugUI::Release();

	Renderer::Release();
}

void EngineLoop::PostRelease()
{
	// Empty for now
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
