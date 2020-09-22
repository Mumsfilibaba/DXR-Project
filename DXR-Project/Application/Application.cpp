#include "Application.h"
#include "Input.h"

#include "Events/EventQueue.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Events/WindowEvent.h"

#include "Platform/PlatformApplication.h"

// TODO: Mayebe should handle this in a different way
#include "Engine/EngineLoop.h"

/*
* Application
*/

TSharedPtr<Application> Application::CurrentApplication = nullptr;

Application::Application()
	: ApplicationEventHandler()
	, MainWindow(nullptr)
	, PlatformApplication(nullptr)
{
}

Application::~Application()
{
}

TSharedPtr<GenericWindow> Application::MakeWindow()
{
	return PlatformApplication->MakeWindow();
}

TSharedPtr<GenericCursor> Application::MakeCursor()
{
	return PlatformApplication->MakeCursor();
}

bool Application::Initialize(TSharedPtr<GenericApplication> InPlatformApplication)
{
	// PlatformApplication
	SetPlatformApplication(InPlatformApplication);

	// Creating main Window
	Uint32 Style =
		WINDOW_STYLE_FLAG_TITLED |
		WINDOW_STYLE_FLAG_CLOSABLE |
		WINDOW_STYLE_FLAG_MINIMIZABLE |
		WINDOW_STYLE_FLAG_MAXIMIZABLE |
		WINDOW_STYLE_FLAG_RESIZEABLE;

	WindowInitializer WinInitializer("DXR", 1920, 1080, Style);
	MainWindow = PlatformApplication->MakeWindow();
	if (MainWindow->Initialize(WinInitializer))
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

void Application::Release()
{
}

void Application::SetCursor(TSharedPtr<GenericCursor> Cursor)
{
	PlatformApplication->SetCursor(Cursor);
}

void Application::SetActiveWindow(TSharedPtr<GenericWindow> Window)
{
	PlatformApplication->SetActiveWindow(Window);
}

void Application::SetCapture(TSharedPtr<GenericWindow> Window)
{
	PlatformApplication->SetCapture(Window);
}

void Application::SetCursorPos(TSharedPtr<GenericWindow> RelativeWindow, Int32 X, Int32 Y)
{
	PlatformApplication->SetCursorPos(RelativeWindow, X, Y);
}

ModifierKeyState Application::GetModifierKeyState() const
{
	return PlatformApplication->GetModifierKeyState();
}

TSharedPtr<GenericWindow> Application::GetMainWindow() const
{
	return MainWindow;
}

TSharedPtr<GenericWindow> Application::GetActiveWindow() const
{
	return PlatformApplication->GetActiveWindow();
}

TSharedPtr<GenericWindow> Application::GetCapture() const
{
	return PlatformApplication->GetCapture();
}

void Application::GetCursorPos(TSharedPtr<GenericWindow> RelativeWindow, Int32& OutX, Int32& OutY) const
{
	PlatformApplication->GetCursorPos(RelativeWindow, OutX, OutY);
}

void Application::SetPlatformApplication(TSharedPtr<GenericApplication> InPlatformApplication)
{
	// If there is a platform application, release the old mainwindow
	if (PlatformApplication)
	{
		MainWindow.Reset();
	}

	PlatformApplication = InPlatformApplication;
	if (PlatformApplication)
	{
		PlatformApplication->SetEventHandler(CurrentApplication);
	}
}

Application* Application::Make()
{
	CurrentApplication = TSharedPtr<Application>(new Application());
	if (CurrentApplication)
	{
		return CurrentApplication.Get();
	}
	else
	{
		return nullptr;
	}
}

Application& Application::Get()
{
	VALIDATE(CurrentApplication != nullptr);
	return (*CurrentApplication.Get());
}

void Application::OnWindowResized(TSharedPtr<GenericWindow> InWindow, Uint16 Width, Uint16 Height)
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
	TSharedPtr<GenericWindow> CaptureWindow = GetCapture();
	if (CaptureWindow)
	{
		SetCapture(nullptr);
	}

	MouseReleasedEvent Event(Button, ModierKeyState);
	EventQueue::SendEvent(Event);
}

void Application::OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState)
{
	TSharedPtr<GenericWindow> CaptureWindow = GetCapture();
	if (!CaptureWindow)
	{
		TSharedPtr<GenericWindow> ActiveWindow = GetActiveWindow();
		SetCapture(ActiveWindow);
	}

	MousePressedEvent Event(Button, ModierKeyState);
	EventQueue::SendEvent(Event);
}

void Application::OnMouseScrolled(Float32 HorizontalDelta, Float32 VerticalDelta)
{
	MouseScrolledEvent Event(HorizontalDelta, VerticalDelta);
	EventQueue::SendEvent(Event);
}

void Application::OnCharacterInput(Uint32 Character)
{
	KeyTypedEvent Event(Character);
	EventQueue::SendEvent(Event);
}