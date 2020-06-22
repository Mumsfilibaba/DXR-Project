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

void Application::SetCursor(std::shared_ptr<WindowsCursor> InCursor)
{
	PlatformApplication->SetCursor(InCursor);
}

void Application::SetActiveWindow(std::shared_ptr<WindowsWindow>& InActiveWindow)
{
	PlatformApplication->SetActiveWindow(InActiveWindow);
}

void Application::SetCapture(std::shared_ptr<WindowsWindow> InCapture)
{
	PlatformApplication->SetCapture(InCapture);
}

void Application::SetCursorPos(std::shared_ptr<WindowsWindow>& InRelativeWindow, Int32 InX, Int32 InY)
{
	PlatformApplication->SetCursorPos(InRelativeWindow, InX, InY);
}

ModifierKeyState Application::GetModifierKeyState() const
{
	return PlatformApplication->GetModifierKeyState();
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

void Application::GetCursorPos(std::shared_ptr<WindowsWindow>& InRelativeWindow, Int32& OutX, Int32& OutY) const
{
	PlatformApplication->GetCursorPos(InRelativeWindow, OutX, OutY);
}

Application* Application::Create()
{
	Instance = std::shared_ptr<Application>(new Application());
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

void Application::OnWindowResized(std::shared_ptr<WindowsWindow>& InWindow, Uint16 InWidth, Uint16 InHeight)
{
	if (Renderer::Get())
	{
		Renderer::Get()->OnResize(InWidth, InHeight);
	}
}

void Application::OnKeyReleased(EKey InKeyCode, const ModifierKeyState& InModierKeyState)
{
	InputManager::Get().RegisterKeyUp(InKeyCode);

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnKeyReleased(InKeyCode);
	}
}

void Application::OnKeyPressed(EKey InKeyCode, const ModifierKeyState& InModierKeyState)
{
	InputManager::Get().RegisterKeyDown(InKeyCode);

	if (Renderer::Get())
	{
		Renderer::Get()->OnKeyPressed(InKeyCode);
	}

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnKeyPressed(InKeyCode);
	}
}

void Application::OnMouseMove(Int32 InX, Int32 InY)
{
	if (Renderer::Get())
	{
		Renderer::Get()->OnMouseMove(InX, InY);
	}
}

void Application::OnMouseButtonReleased(EMouseButton InButton, const ModifierKeyState& InModierKeyState)
{
	std::shared_ptr<WindowsWindow> CaptureWindow = GetCapture();
	if (CaptureWindow)
	{
		SetCapture(nullptr);
	}

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnMouseButtonReleased(InButton);
	}
}

void Application::OnMouseButtonPressed(EMouseButton InButton, const ModifierKeyState& InModierKeyState)
{
	std::shared_ptr<WindowsWindow> CaptureWindow = GetCapture();
	if (!CaptureWindow)
	{
		std::shared_ptr<WindowsWindow> ActiveWindow = GetActiveWindow();
		SetCapture(ActiveWindow);
	}

	if (GuiContext::Get())
	{
		GuiContext::Get()->OnMouseButtonPressed(InButton);
	}
}

void Application::OnMouseScrolled(Float32 InHorizontalDelta, Float32 InVerticalDelta)
{
	if (GuiContext::Get())
	{
		GuiContext::Get()->OnMouseScrolled(InHorizontalDelta, InVerticalDelta);
	}
}

void Application::OnCharacterInput(Uint32 Character)
{
	if (GuiContext::Get())
	{
		GuiContext::Get()->OnCharacterInput(Character);
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
