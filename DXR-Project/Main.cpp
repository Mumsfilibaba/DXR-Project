#include "Application/Application.h"

#include "Rendering/Renderer.h"
#include "Rendering/GuiContext.h"

#include <crtdbg.h>

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// Application
	Application* App = Application::Create();
	if (!App)
	{
		::MessageBox(0, "Failed to create Application", "ERROR", MB_ICONERROR);
		return -1;
	}

	InitializeCursors();

	// Renderer
	Renderer* Renderer = Renderer::Create(App->GetWindow());
	if (!Renderer)
	{
		::MessageBox(0, "Failed to create Renderer", "ERROR", MB_ICONERROR);
		return -1;
	}

	// ImGui
	GuiContext* GUIContext = GuiContext::Create(Renderer->GetDevice());
	if (!GUIContext)
	{
		::MessageBox(0, "Failed to create ImGuiContext", "ERROR", MB_ICONERROR);
		return -1;
	}

	// Run-Loop
	bool IsRunning = true;
	while (IsRunning)
	{
		IsRunning = App->Tick();

		GuiContext::Get()->BeginFrame();

		ImGui::ShowDemoWindow();

		GuiContext::Get()->EndFrame();

		Renderer::Get()->Tick();
	}

	return 0;
}

#pragma warning(pop)