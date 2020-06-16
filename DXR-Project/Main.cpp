#include "Rendering/Renderer.h"

#include "Application/Application.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	// Application
	Application* App = Application::Create();
	if (!App)
	{
		::MessageBox(0, "Failed to create Application", "ERROR", MB_ICONERROR);
		return -1;
	}

	// Renderer
	Renderer* Renderer = Renderer::Create(App->GetWindow());

	// Run-Loop
	bool IsRunning = true;
	while (IsRunning)
	{
		IsRunning = App->Tick();
		Renderer::Get()->Tick();
	}

	return 0;
}

#pragma warning(pop)