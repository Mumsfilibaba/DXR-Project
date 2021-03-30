#include "Engine.h"

#include "Core/Application/Platform/PlatformMisc.h"
#include "Core/Application/Platform/Platform.h"

#include "Debug/Console/Console.h"

ConsoleCommand GToggleFullscreen;
ConsoleCommand GExit;

Engine GEngine;

bool Engine::Init()
{
    const uint32 Style =
        WindowStyleFlag_Titled      |
        WindowStyleFlag_Closable    |
        WindowStyleFlag_Minimizable |
        WindowStyleFlag_Maximizable |
        WindowStyleFlag_Resizeable;

    MainWindow = GenericWindow::Create("DXR Engine", 1920, 1080, Style);
    if (MainWindow)
    {
        MainWindow->Show(false);

        GToggleFullscreen.OnExecute.AddObject(MainWindow.Get(), &GenericWindow::ToggleFullscreen);
        INIT_CONSOLE_COMMAND("a.ToggleFullscreen", &GToggleFullscreen);
    }
    else
    {
        PlatformMisc::MessageBox("ERROR", "Failed to create Engine");
        return false;
    }

    GExit.OnExecute.AddObject(this, &Engine::Exit);
    INIT_CONSOLE_COMMAND("a.Exit", &GExit);

    IsRunning = true;
    return true;
}

bool Engine::Release()
{
    // Empty for now
    return true;
}

void Engine::Exit()
{
    IsRunning = false;
}

void Engine::OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModfierKeyState)
{
    KeyReleasedEvent Event(KeyCode, ModfierKeyState);
    OnKeyReleasedEvent.Broadcast(Event);
}

void Engine::OnKeyPressed(EKey KeyCode, bool IsRepeat, const ModifierKeyState& ModfierKeyState)
{
    KeyPressedEvent Event(KeyCode, IsRepeat, ModfierKeyState);
    OnKeyPressedEvent.Broadcast(Event);
}

void Engine::OnKeyTyped(uint32 Character)
{
    KeyTypedEvent Event(Character);
    OnKeyTypedEvent.Broadcast(Event);
}

void Engine::OnMouseMove(int32 x, int32 y)
{
    MouseMovedEvent Event(x, y);
    OnMouseMoveEvent.Broadcast(Event);
}

void Engine::OnMouseReleased(EMouseButton Button, const ModifierKeyState& ModfierKeyState)
{
    GenericWindow* CaptureWindow = Platform::GetCapture();
    if (CaptureWindow)
    {
        Platform::SetCapture(nullptr);
    }

    MouseReleasedEvent Event(Button, ModfierKeyState);
    OnMouseReleasedEvent.Broadcast(Event);
}

void Engine::OnMousePressed(EMouseButton Button, const ModifierKeyState& ModfierKeyState)
{
    GenericWindow* CaptureWindow = Platform::GetCapture();
    if (!CaptureWindow)
    {
        GenericWindow* ActiveWindow = Platform::GetActiveWindow();
        Platform::SetCapture(ActiveWindow);
    }

    MousePressedEvent Event(Button, ModfierKeyState);
    OnMousePressedEvent.Broadcast(Event);
}

void Engine::OnMouseScrolled(float HorizontalDelta, float VerticalDelta)
{
    MouseScrolledEvent Event(HorizontalDelta, VerticalDelta);
    OnMouseScrolledEvent.Broadcast(Event);
}

void Engine::OnWindowResized(const TRef<GenericWindow>& InWindow, uint16 Width, uint16 Height)
{
    WindowResizeEvent Event(InWindow, Width, Height);
    OnWindowResizedEvent.Broadcast(Event);
}

void Engine::OnWindowMoved(const TRef<GenericWindow>& InWindow, int16 x, int16 y)
{
    WindowMovedEvent Event(InWindow, x, y);
    OnWindowMovedEvent.Broadcast(Event);
}

void Engine::OnWindowFocusChanged(const TRef<GenericWindow>& InWindow, bool HasFocus)
{
    WindowFocusChangedEvent Event(InWindow, HasFocus);
    OnWindowFocusChangedEvent.Broadcast(Event);
}

void Engine::OnWindowMouseLeft(const TRef<GenericWindow>& InWindow)
{
    WindowMouseLeftEvent Event(InWindow);
    OnWindowMouseLeftEvent.Broadcast(Event);
}

void Engine::OnWindowMouseEntered(const TRef<GenericWindow>& InWindow)
{
    WindowMouseEnteredEvent Event(InWindow);
    OnWindowMouseEnteredEvent.Broadcast(Event);
}

void Engine::OnWindowClosed(const TRef<GenericWindow>& InWindow)
{
    WindowClosedEvent Event(InWindow);
    OnWindowClosedEvent.Broadcast(Event);

    if (InWindow == MainWindow)
    {
        PlatformMisc::RequestExit(0);
    }
}

void Engine::OnApplicationExit(int32 ExitCode)
{
    IsRunning = false;
    OnApplicationExitEvent.Broadcast(ExitCode);
}
