#pragma once
#include "Core.h"

#include "Core/Input/InputCodes.h"
#include "Core/Application/Events.h"
#include "Core/Application/Platform/PlatformCallbacks.h"
#include "Core/Delegates/Event.h"

#include "RenderLayer/Viewport.h"

class Engine : public PlatformCallbacks
{
public:
    bool Init();

    bool Release();

    void Exit();

public:
    virtual void OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnKeyPressed(EKey KeyCode, bool IsRepeat, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnKeyTyped(uint32 Character) override final;

    virtual void OnMouseMove(int32 x, int32 y) override final;
    virtual void OnMouseReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnMousePressed(EMouseButton Button, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnMouseScrolled(float HorizontalDelta, float VerticalDelta) override final;

    virtual void OnWindowResized(const TRef<Window>& Window, uint16 Width, uint16 Height) override final;
    virtual void OnWindowMoved(const TRef<Window>& Window, int16 x, int16 y) override final;
    virtual void OnWindowFocusChanged(const TRef<Window>& Window, bool HasFocus) override final;
    virtual void OnWindowMouseLeft(const TRef<Window>& Window) override final;
    virtual void OnWindowMouseEntered(const TRef<Window>& Window) override final;
    virtual void OnWindowClosed(const TRef<Window>& Window) override final;

    virtual void OnApplicationExit(int32 ExitCode) override final;

public:
    // Key events
    DECLARE_EVENT(OnKeyReleasedEvent, Engine, const KeyReleasedEvent&);
    OnKeyReleasedEvent OnKeyReleasedEvent;
    DECLARE_EVENT(OnKeyPressedEvent, Engine, const KeyPressedEvent&);
    OnKeyPressedEvent OnKeyPressedEvent;
    DECLARE_EVENT(OnKeyTypedEvent, Engine, const KeyTypedEvent&);
    OnKeyTypedEvent OnKeyTypedEvent;

    // Mouse events
    DECLARE_EVENT(OnMouseMoveEvent, Engine, const MouseMovedEvent&);
    OnMouseMoveEvent OnMouseMoveEvent;
    DECLARE_EVENT(OnMousePressedEvent, Engine, const MousePressedEvent&);
    OnMousePressedEvent OnMousePressedEvent;
    DECLARE_EVENT(OnMouseReleasedEvent, Engine, const MouseReleasedEvent&);
    OnMouseReleasedEvent OnMouseReleasedEvent;
    DECLARE_EVENT(OnMouseScrolledEvent, Engine, const MouseScrolledEvent&);
    OnMouseScrolledEvent OnMouseScrolledEvent;

    // Window Events
    DECLARE_EVENT(OnWindowResizedEvent, Engine, const WindowResizeEvent&);
    OnWindowResizedEvent OnWindowResizedEvent;
    DECLARE_EVENT(OnWindowMovedEvent, Engine, const WindowMovedEvent&);
    OnWindowMovedEvent OnWindowMovedEvent;
    DECLARE_EVENT(OnWindowFocusChangedEvent, Engine, const WindowFocusChangedEvent&);
    OnWindowFocusChangedEvent OnWindowFocusChangedEvent;
    DECLARE_EVENT(OnWindowMouseEnteredEvent, Engine, const WindowMouseEnteredEvent&);
    OnWindowMouseEnteredEvent OnWindowMouseEnteredEvent;
    DECLARE_EVENT(OnWindowMouseLeftEvent, Engine, const WindowMouseLeftEvent&);
    OnWindowMouseLeftEvent OnWindowMouseLeftEvent;
    DECLARE_EVENT(OnWindowClosedEvent, Engine, const WindowClosedEvent&);
    OnWindowClosedEvent OnWindowClosedEvent;

    // Engine Events
    DECLARE_EVENT(OnApplicationExitEvent, Engine, int32);
    OnApplicationExitEvent OnApplicationExitEvent;

public:
    TRef<Window>   MainWindow;
    TRef<Viewport> MainViewport;

    bool IsRunning = false;
};

extern Engine GEngine;