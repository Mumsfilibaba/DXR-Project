#include "Application.h"

#include "Platform/PlatformApplication.h"
#include "Platform/PlatformMisc.h"

#include "Debug/Console.h"

#include "Scene/Scene.h"

Application* GApplication;

Application::Application()
    : Window(nullptr)
    , IsRunning(false)
    , Scene(nullptr)
{
}

Application::~Application()
{
    SafeDelete(Scene);
}

bool Application::Init()
{
    const uint32 Style =
        WindowStyleFlag_Titled      |
        WindowStyleFlag_Closable    |
        WindowStyleFlag_Minimizable |
        WindowStyleFlag_Maximizable |
        WindowStyleFlag_Resizeable;

    Window = PlatformApplication::Get().CreateWindow("DXR Engine", 1920, 1080, Style);
    if (Window)
    {
        Window->Show(false);

        INIT_CONSOLE_COMMAND("a.ToggleFullscreen", []()
            {
                GApplication->Window->ToggleFullscreen();
            });
    }
    else
    {
        PlatformMisc::MessageBox("ERROR", "Failed to create Application");
        return false;
    }

    INIT_CONSOLE_COMMAND("a.Quit", []()
        {
            GApplication->Exit();
        });
    
    IsRunning = true;
    return true;
}

void Application::Tick(Timestamp Deltatime)
{
    // Empty for now
    UNREFERENCED_VARIABLE(Deltatime);
}

bool Application::Release()
{
    // Empty for now
    return true;
}

void Application::OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModfierKeyState)
{
    KeyReleasedEvent Event(KeyCode, ModfierKeyState);
    OnKeyReleasedEvent.Broadcast(Event);
}

void Application::OnKeyPressed(EKey KeyCode, bool IsRepeat, const ModifierKeyState& ModfierKeyState)
{
    KeyPressedEvent Event(KeyCode, IsRepeat, ModfierKeyState);
    OnKeyPressedEvent.Broadcast(Event);
}

void Application::OnKeyTyped(uint32 Character)
{
    KeyTypedEvent Event(Character);
    OnKeyTypedEvent.Broadcast(Event);
}

void Application::OnMouseMove(int32 x, int32 y)
{
    MouseMovedEvent Event(x, y);
    OnMouseMoveEvent.Broadcast(Event);
}

void Application::OnMouseReleased(EMouseButton Button, const ModifierKeyState& ModfierKeyState)
{
    GenericWindow* CaptureWindow = PlatformApplication::Get().GetCapture();
    if (CaptureWindow)
    {
        PlatformApplication::Get().SetCapture(nullptr);
    }

    MouseReleasedEvent Event(Button, ModfierKeyState);
    OnMouseReleasedEvent.Broadcast(Event);
}

void Application::OnMousePressed(EMouseButton Button, const ModifierKeyState& ModfierKeyState)
{
    GenericWindow* CaptureWindow = PlatformApplication::Get().GetCapture();
    if (!CaptureWindow)
    {
        GenericWindow* ActiveWindow = PlatformApplication::Get().GetActiveWindow();
        PlatformApplication::Get().SetCapture(ActiveWindow);
    }

    MousePressedEvent Event(Button, ModfierKeyState);
    OnMousePressedEvent.Broadcast(Event);
}

void Application::OnMouseScrolled(float HorizontalDelta, float VerticalDelta)
{
    MouseScrolledEvent Event(HorizontalDelta, VerticalDelta);
    OnMouseScrolledEvent.Broadcast(Event);
}

void Application::OnWindowResized(const TRef<GenericWindow>& InWindow, uint16 Width, uint16 Height)
{
    WindowResizeEvent Event(InWindow, Width, Height);
    OnWindowResizedEvent.Broadcast(Event);
}

void Application::OnWindowMoved(const TRef<GenericWindow>& InWindow, int16 x, int16 y)
{
    WindowMovedEvent Event(InWindow, x, y);
    OnWindowMovedEvent.Broadcast(Event);
}

void Application::OnWindowFocusChanged(const TRef<GenericWindow>& InWindow, bool HasFocus)
{
    WindowFocusChangedEvent Event(InWindow, HasFocus);
    OnWindowFocusChangedEvent.Broadcast(Event);
}

void Application::OnWindowMouseLeft(const TRef<GenericWindow>& InWindow)
{
    WindowMouseLeftEvent Event(InWindow);
    OnWindowMouseLeftEvent.Broadcast(Event);
}

void Application::OnWindowMouseEntered(const TRef<GenericWindow>& InWindow)
{
    WindowMouseEnteredEvent Event(InWindow);
    OnWindowMouseEnteredEvent.Broadcast(Event);
}

void Application::OnWindowClosed(const TRef<GenericWindow>& InWindow)
{
    WindowClosedEvent Event(InWindow);
    OnWindowClosedEvent.Broadcast(Event);

    if (InWindow == Window)
    {
        PlatformMisc::RequestExit(0);
    }
}

void Application::OnApplicationExit(int32 ExitCode)
{
    IsRunning = false;
    OnApplicationExitEvent.Broadcast(ExitCode);
}
