#pragma once
#include "Core.h"
#include "Core/Ref.h"

#include "Events.h"

#include "Time/Timestamp.h"

#include "Core/Delegates/Event.h"
#include "Core/Application/Generic/GenericApplicationEventHandler.h"
#include "Core/Application/Generic/GenericWindow.h"

extern class Application* CreateApplication();

class Application : public GenericApplicationEventHandler
{
public:
    Application();
    virtual ~Application();

    void Exit()
    {
        IsRunning = false;
    }

public:
    virtual bool Init();

    virtual void Tick(Timestamp Deltatime);

    virtual bool Release();

public:
    virtual void OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnKeyPressed(EKey KeyCode, bool IsRepeat, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnKeyTyped(uint32 Character) override final;

    virtual void OnMouseMove(int32 x, int32 y) override final;
    virtual void OnMouseReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnMousePressed(EMouseButton Button, const ModifierKeyState& ModierKeyState) override final;
    virtual void OnMouseScrolled(float HorizontalDelta, float VerticalDelta) override final;

    virtual void OnWindowResized(const TRef<GenericWindow>& Window, uint16 Width, uint16 Height) override final;
    virtual void OnWindowMoved(const TRef<GenericWindow>& Window, int16 x, int16 y) override final;
    virtual void OnWindowFocusChanged(const TRef<GenericWindow>& Window, bool HasFocus) override final;
    virtual void OnWindowMouseLeft(const TRef<GenericWindow>& Window) override final;
    virtual void OnWindowMouseEntered(const TRef<GenericWindow>& Window) override final;
    virtual void OnWindowClosed(const TRef<GenericWindow>& Window) override final;
    
    virtual void OnApplicationExit(int32 ExitCode) override final;

public:
    // Key events
    DECLARE_EVENT(OnKeyReleasedEvent, Application, const KeyReleasedEvent&);
    OnKeyReleasedEvent OnKeyReleasedEvent;
    DECLARE_EVENT(OnKeyPressedEvent, Application, const KeyPressedEvent&);
    OnKeyPressedEvent OnKeyPressedEvent;
    DECLARE_EVENT(OnKeyTypedEvent, Application, const KeyTypedEvent&);
    OnKeyTypedEvent OnKeyTypedEvent;

    // Mouse events
    DECLARE_EVENT(OnMouseMoveEvent, Application, const MouseMovedEvent&);
    OnMouseMoveEvent OnMouseMoveEvent;
    DECLARE_EVENT(OnMousePressedEvent, Application, const MousePressedEvent&);
    OnMousePressedEvent OnMousePressedEvent;
    DECLARE_EVENT(OnMouseReleasedEvent, Application, const MouseReleasedEvent&);
    OnMouseReleasedEvent OnMouseReleasedEvent;
    DECLARE_EVENT(OnMouseScrolledEvent, Application, const MouseScrolledEvent&);
    OnMouseScrolledEvent OnMouseScrolledEvent;

    // Window Events
    DECLARE_EVENT(OnWindowResizedEvent, Application, const WindowResizeEvent&);
    OnWindowResizedEvent OnWindowResizedEvent;
    DECLARE_EVENT(OnWindowMovedEvent, Application, const WindowMovedEvent&);
    OnWindowMovedEvent OnWindowMovedEvent;
    DECLARE_EVENT(OnWindowFocusChangedEvent, Application, const WindowFocusChangedEvent&);
    OnWindowFocusChangedEvent OnWindowFocusChangedEvent;
    DECLARE_EVENT(OnWindowMouseEnteredEvent, Application, const WindowMouseEnteredEvent&);
    OnWindowMouseEnteredEvent OnWindowMouseEnteredEvent;
    DECLARE_EVENT(OnWindowMouseLeftEvent, Application, const WindowMouseLeftEvent&);
    OnWindowMouseLeftEvent OnWindowMouseLeftEvent;
    DECLARE_EVENT(OnWindowClosedEvent, Application, const WindowClosedEvent&);
    OnWindowClosedEvent OnWindowClosedEvent;

    // Application Events
    DECLARE_EVENT(OnApplicationExitEvent, Application, int32);
    OnApplicationExitEvent OnApplicationExitEvent;

public:
    TRef<GenericWindow> Window;
    bool IsRunning = false;

    class Scene* Scene = nullptr;
};

extern Application* GApplication;