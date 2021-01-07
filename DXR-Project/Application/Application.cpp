#include "Application.h"
#include "Input.h"

#include "Events/EventQueue.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Events/WindowEvent.h"

#include "Platform/PlatformApplication.h"

// TODO: Maybe should handle this in a different way
#include "Engine/EngineLoop.h"

/*
* Application
*/

TSharedPtr<Application> Application::Instance;

Application::Application(GenericApplication* InPlatformApplication)
	: PlatformApplication(InPlatformApplication)
{
	VALIDATE(PlatformApplication != nullptr);
}

bool Application::Initialize()
{
	// Creating main Window
	const UInt32 Style =
		WindowStyleFlag_Titled		|
		WindowStyleFlag_Closable	|
		WindowStyleFlag_Minimizable |
		WindowStyleFlag_Maximizable |
		WindowStyleFlag_Resizeable;

	WindowCreateInfo WinCreateInfo("DXR", 1920, 1080, Style);
	MainWindow = PlatformApplication->MakeWindow();
	if (MainWindow->Initialize(WinCreateInfo))
	{
		MainWindow->Show(false);
	}
	else
	{
		return false;
	}

	return true;
}

void Application::Tick()
{
	// Tick OS
	if (!PlatformApplication->Tick())
	{
		EngineLoop::Exit();
	}
}

void Application::SetPlatformApplication(GenericApplication* InPlatformApplication)
{
	if (PlatformApplication)
	{
		MainWindow.Reset();
	}

	PlatformApplication = InPlatformApplication;
	if (PlatformApplication)
	{
		PlatformApplication->SetEventHandler(Instance);
	}
}

Application* Application::Make(GenericApplication* InPlatformApplication)
{
	Instance = TSharedPtr(new Application(InPlatformApplication));
	if (Instance)
	{
		InPlatformApplication->SetEventHandler(Instance);
		return Instance.Get();
	}
	else
	{
		return nullptr;
	}
}

void Application::OnWindowResized(TSharedRef<GenericWindow> InWindow, UInt16 Width, UInt16 Height)
{
	WindowResizeEvent Event(InWindow, Width, Height);
	EventQueue::SendEvent(Event);
}

void Application::OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState)
{
	Input::RegisterKeyUp(KeyCode);

	KeyReleasedEvent Event(KeyCode, ModierKeyState);
	EventQueue::SendEvent(Event);
}

void Application::OnKeyPressed(EKey KeyCode, const ModifierKeyState& ModierKeyState)
{
	Input::RegisterKeyDown(KeyCode);

	KeyPressedEvent Event(KeyCode, ModierKeyState);
	EventQueue::SendEvent(Event);
}

void Application::OnMouseMove(Int32 x, Int32 y)
{
	MouseMovedEvent Event(x, y);
	EventQueue::SendEvent(Event);
}

void Application::OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState)
{
	TSharedRef<GenericWindow> CaptureWindow = GetCapture();
	if (CaptureWindow)
	{
		SetCapture(nullptr);
	}

	MouseReleasedEvent Event(Button, ModierKeyState);
	EventQueue::SendEvent(Event);
}

void Application::OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState)
{
	TSharedRef<GenericWindow> CaptureWindow = GetCapture();
	if (!CaptureWindow)
	{
		TSharedRef<GenericWindow> ActiveWindow = GetActiveWindow();
		SetCapture(ActiveWindow);
	}

	MousePressedEvent Event(Button, ModierKeyState);
	EventQueue::SendEvent(Event);
}

void Application::OnMouseScrolled(Float HorizontalDelta, Float VerticalDelta)
{
	MouseScrolledEvent Event(HorizontalDelta, VerticalDelta);
	EventQueue::SendEvent(Event);
}

void Application::OnCharacterInput(UInt32 Character)
{
	KeyTypedEvent Event(Character);
	EventQueue::SendEvent(Event);
}