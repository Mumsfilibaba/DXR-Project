#include "EngineLoop.h"

#include "Time/Clock.h"

#include "Application/Application.h"
#include "Application/Generic/GenericOutputDevice.h"
#include "Application/Generic/GenericCursor.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"

#include "Editor/Editor.h"

static Clock GlobalClock;
static bool GlobalIsRunning = false;

/*
* EngineLoop
*/
bool EngineLoop::CoreInitialize()
{
	GlobalOutputDevices::Initialize();

	return true;
}

bool EngineLoop::Initialize()
{
	// Application
	Application* App = Application::Make();
	if (App->Initialize())
	{
		::MessageBox(0, "Failed to create Application", "ERROR", MB_ICONERROR);
		return false;
	}

	GlobalCursors::Initialize();

	// Renderer
	Renderer* Renderer = Renderer::Make(App->GetWindow());
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

	GlobalIsRunning = true;
	return true;
}

void EngineLoop::Tick()
{
	GlobalClock.Tick();

	Application::Get().Tick();

	Renderer::Get()->Tick(*Scene::GetCurrentScene());

	Editor::Tick();

	//DebugUI::DrawUI([]
	//	{
	//		ImGui::ShowDemoWindow();
	//	});
}

void EngineLoop::Release()
{
	Application::Get().Release();

	DebugUI::Release();

	Renderer::Release();
}

void EngineLoop::CoreRelease()
{
	RenderingAPI::Release();

	GlobalOutputDevices::Initialize();
}

Timestamp EngineLoop::GetDeltaTime()
{
	return GlobalClock.GetDeltaTime();
}

Timestamp EngineLoop::GetTotalElapsedTime()
{
	return GlobalClock.GetTotalTime();
}

bool EngineLoop::IsRunning()
{
	return GlobalIsRunning;
}

void EngineLoop::Exit()
{
	GlobalIsRunning = false;
}
