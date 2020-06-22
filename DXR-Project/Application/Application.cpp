#include "Application.h"
#include "InputManager.h"

#include "Rendering/Renderer.h"
#include "Rendering/GuiContext.h"

std::shared_ptr<Application> Application::Instance = nullptr;

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

void Application::SetActiveWindow(std::shared_ptr<WindowsWindow>& InActiveWindow)
{
	PlatformApplication->SetActiveWindow(InActiveWindow);
}

void Application::SetCapture(std::shared_ptr<WindowsWindow>& InCapture)
{
	PlatformApplication->SetCapture(InCapture);
}

std::shared_ptr<WindowsWindow> Application::GetWindow() const
{
	return Window;
}

std::shared_ptr<WindowsWindow> Application::GetActiveWindow() const
{
	return PlatformApplication->GetActiveWindow();
}

std::shared_ptr<WindowsWindow> Application::GetCapture() const
{
	return PlatformApplication->GetCapture();
}

Application* Application::Create()
{
	Instance = std::make_unique<Application>();
	if (Instance->Initialize())
	{
		return Instance.get();
	}
	else
	{
		return nullptr;
	}
}

Application* Application::Get()
{
	return Instance.get();
}

void Application::OnWindowResize(std::shared_ptr<WindowsWindow>& InWindow, Uint16 InWidth, Uint16 InHeight)
{
	if (Renderer::Get())
	{
		Renderer::Get()->OnResize(InWidth, InHeight);
	}
}

void Application::OnKeyUp(EKey InKeyCode)
{
	InputManager::Get().RegisterKeyUp(InKeyCode);

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnKeyUp(InKeyCode);
	}
}

void Application::OnKeyDown(EKey InKeyCode)
{
	InputManager::Get().RegisterKeyDown(InKeyCode);

	if (Renderer::Get())
	{
		Renderer::Get()->OnKeyDown(InKeyCode);
	}

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnKeyDown(InKeyCode);
	}
}

void Application::OnMouseMove(Int32 InX, Int32 InY)
{
	if (Renderer::Get())
	{
		Renderer::Get()->OnMouseMove(InX, InY);
	}

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnMouseMove(InX, InY);
	}
}

void Application::OnMouseButtonReleased(EMouseButton InButton)
{
	if (GuiContext::Get())
	{
		GuiContext::Get()->OnMouseButtonReleased(InButton);
	}
}

void Application::OnMouseButtonPressed(EMouseButton InButton)
{
	if (GuiContext::Get())
	{
		GuiContext::Get()->OnMouseButtonPressed(InButton);
	}
}

bool Application::Initialize()
{
	// Application
	HINSTANCE hInstance = static_cast<HINSTANCE>(GetModuleHandle(NULL));
	PlatformApplication = WindowsApplication::Create(hInstance);
	if (PlatformApplication)
	{
		PlatformApplication->SetEventHandler(std::shared_ptr<EventHandler>(Instance));
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
