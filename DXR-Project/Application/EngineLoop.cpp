#include "EngineLoop.h"
#include "Clock.h"
#include "Application.h"

#include "Windows/WindowsConsoleOutput.h"

#include "Rendering/DebugUI.h"
#include "Rendering/Renderer.h"

#include "Editor/Editor.h"

static Clock	GlobalClock;
static bool		GlobalIsRunning = false;

bool EngineLoop::CoreInitialize()
{
	GlobalOutputHandle = new WindowsConsoleOutput();
	return true;
}

bool EngineLoop::Initialize()
{
	Application* App = Application::Make();
	if (!App)
	{
		::MessageBox(0, "Failed to create Application", "ERROR", MB_ICONERROR);
		return false;
	}

	GlobalIsRunning = true;
	return true;
}

void EngineLoop::Tick()
{
	GlobalClock.Tick();
	Application::Get()->Tick();

	Renderer::Get()->Tick(*Scene::GetCurrentScene());

	Editor::Tick();

	//DebugUI::DrawUI([]
	//	{
	//		ImGui::ShowDemoWindow();
	//	});
}

void EngineLoop::Release()
{
	Application::Get()->Release();
}

void EngineLoop::CoreRelease()
{
	//RenderingAPI::Release();

	SAFEDELETE(GlobalOutputHandle);
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
