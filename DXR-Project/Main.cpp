#include "Application/Application.h"

#include "Rendering/Renderer.h"
#include "Rendering/GuiContext.h"

#include "Containers/TArray.h"

#include <crtdbg.h>

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// Application
	GlobalOutputHandle = new WindowsConsoleOutput();

	Application* App = Application::Make();
	if (!App)
	{
		::MessageBox(0, "Failed to create Application", "ERROR", MB_ICONERROR);
		return -1;
	}
	else
	{
		App->Run();
		App->Release();

		SAFEDELETE(GlobalOutputHandle);
		
		return 0;
	}
}

#pragma warning(pop)