#include "Windows/WindowsApplication.h"
#include "Windows/WindowsWindow.h"

#include "D3D12/D3D12GraphicsDevice.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	WindowsApplication* App = WindowsApplication::Create(hInstance);

	WindowsWindow* Window = App->CreateWindow(1280, 720);
	Window->Show();

	D3D12GraphicsDevice* Device = D3D12GraphicsDevice::Create();

	bool IsRunning = true;
	while (IsRunning)
	{
		IsRunning = App->Tick();
	}

	return 0;
}