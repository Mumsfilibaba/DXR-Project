#include "Application.h"
#include "InputManager.h"

#include "Rendering/Renderer.h"

std::shared_ptr<Application> Application::ApplicationInstance = nullptr;

Application::Application()
{
}

Application::~Application()
{
	SAFEDELETE(PlatformApplication);
}

bool Application::Tick()
{
	return PlatformApplication->Tick();
}

std::shared_ptr<WindowsWindow> Application::GetWindow()
{
	return Window;
}

Application* Application::Create()
{
	ApplicationInstance = std::make_unique<Application>();
	if (ApplicationInstance->Initialize())
	{
		return ApplicationInstance.get();
	}
	else
	{
		return nullptr;
	}
}

Application* Application::Get()
{
	return ApplicationInstance.get();
}

void Application::OnWindowResize(std::shared_ptr<WindowsWindow>& Window, Uint16 Width, Uint16 Height)
{
	if (Renderer::Get())
	{
		Renderer::Get()->OnResize(Width, Height);
	}
}

void Application::OnKeyUp(EKey KeyCode)
{
	InputManager::Get().RegisterKeyUp(KeyCode);
}

void Application::OnKeyDown(EKey KeyCode)
{
	InputManager::Get().RegisterKeyDown(KeyCode);

	if (Renderer::Get())
	{
		Renderer::Get()->OnKeyDown(KeyCode);
	}
}

void Application::OnMouseMove(Int32 X, Int32 Y)
{
	if (Renderer::Get())
	{
		Renderer::Get()->OnMouseMove(X, Y);
	}
}

bool Application::Initialize()
{
	// Application
	HINSTANCE hInstance = static_cast<HINSTANCE>(GetModuleHandle(NULL));
	PlatformApplication = WindowsApplication::Create(hInstance);
	if (PlatformApplication)
	{
		PlatformApplication->SetEventHandler(std::shared_ptr<EventHandler>(ApplicationInstance));
	}
	else
	{
		return false;
	}

	// Window
	Window = std::shared_ptr<WindowsWindow>(PlatformApplication->CreateWindow(1280, 720));
	if (Window)
	{
		Window->Show();
	}
	else
	{
		return false;
	}

	return true;
}
