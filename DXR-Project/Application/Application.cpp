#include "Application.h"

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
	if (ApplicationInstance->Init())
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

void Application::OnWindowResize(WindowsWindow* Window, Uint16 Width, Uint16 Height)
{
	Renderer::Get()->OnResize(Width, Height);
}

void Application::OnKeyDown(Uint32 KeyCode)
{
	Renderer::Get()->OnKeyDown(KeyCode);
}

void Application::OnMouseMove(Int32 x, Int32 y)
{
}

bool Application::Init()
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
