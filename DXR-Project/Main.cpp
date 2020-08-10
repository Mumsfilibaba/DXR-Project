#include "Application/Application.h"

#include "Rendering/Renderer.h"
#include "Rendering/GuiContext.h"

#include "STL/TArray.h"

#include <crtdbg.h>

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	
	TArray<std::string> Array0;
	TArray<std::string> Array1 = TArray<std::string>(5);
	TArray<std::string> Array2 = { "String", "HelloWorld", "Array" };
	TArray<std::string> Array3 = TArray<std::string>(Array2.begin(), Array2.end());

	// Application
	GlobalOutputHandle = new WindowsConsoleOutput();

	for (const std::string& Str : Array2)
	{
		LOG_INFO(Str);
	}

	for (const std::string& Str : Array3)
	{
		LOG_INFO(Str);
	}

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