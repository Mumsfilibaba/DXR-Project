#include "Windows/WindowsApplication.h"
#include "Windows/WindowsWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	WindowsApplication* App = WindowsApplication::Create(hInstance);

	WindowsWindow* Window = App->CreateWindow(1280, 720);
	Window->Show();

	bool IsRunning = true;
	while (IsRunning)
	{
		IsRunning = App->Tick();
	}

	return 0;
}