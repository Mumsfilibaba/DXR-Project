#include "EventDispatcher.h"

#include "Application/Input.h"

#include <imgui.h>

EventDispatcher::EventDispatcher(GenericApplication* InApplication)
    : GenericApplicationEventHandler()
    , Application(InApplication)
    , EventHandlers()
{
}

void EventDispatcher::RegisterEventHandler(EventHandlerFunc Func, uint8 EventCategoryMask)
{
    Assert(Func != nullptr);

    for (EventHandlerPair& Pair : EventHandlers)
    {
        if (Pair.IsHandlerFunc)
        {
            if (Pair.Func == Func)
            {
                LOG_WARNING("Function is already registered as a EventHandler and will not be registered again");
                return;
            }
        }
    }

    EventHandlers.EmplaceBack(Func, EventCategoryMask);
}

void EventDispatcher::RegisterEventHandler(IEventHandler* EventHandler, uint8 EventCategoryMask)
{
    Assert(EventHandler != nullptr);

    for (EventHandlerPair& Pair : EventHandlers)
    {
        if (!Pair.IsHandlerFunc)
        {
            if (Pair.Handler == EventHandler)
            {
                LOG_WARNING("Handler is already registered as a EventHandler and will not be registered again");
                return;
            }
        }
    }

    EventHandlers.EmplaceBack(EventHandler, EventCategoryMask);
}

void EventDispatcher::UnregisterEventHandler(IEventHandler* EventHandler)
{
    Assert(EventHandler != nullptr);

    for (TArray<EventHandlerPair>::Iterator It = EventHandlers.Begin(); It != EventHandlers.End(); It++)
    {
        if (!It->IsHandlerFunc)
        {
            if (It->Handler != EventHandler)
            {
                It = EventHandlers.Erase(It);
                return;
            }
        }
    }

    LOG_WARNING("Handler is NOT registered as a EventHandler");
}

void EventDispatcher::UnregisterEventHandler(EventHandlerFunc Func)
{
    Assert(Func != nullptr);

    for (TArray<EventHandlerPair>::Iterator It = EventHandlers.Begin(); It != EventHandlers.End(); It++)
    {
        if (It->IsHandlerFunc)
        {
            if (It->Func != Func)
            {
                It = EventHandlers.Erase(It);
                return;
            }
        }
    }

    LOG_WARNING("Handler is NOT registered as a EventHandler");
}

bool EventDispatcher::SendEvent(const Event& Event)
{
    for (EventHandlerPair& Pair : EventHandlers)
    {
        if (Pair.CategoryMask & Event.GetCategoryFlags())
        {
            if (Pair.IsHandlerFunc)
            {
                if (Pair.Func(Event))
                {
                    Event.HasBeenHandled = true;
                }
            }
            else
            {
                if (Pair.Handler->OnEvent(Event))
                {
                    Event.HasBeenHandled = true;
                }
            }
        }
    }

    return Event.HasBeenHandled;
}

void EventDispatcher::OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState)
{
    Input::RegisterKeyUp(KeyCode);

    KeyReleasedEvent Event(KeyCode, ModierKeyState);
    SendEvent(Event);
}

void EventDispatcher::OnKeyPressed(EKey KeyCode, bool IsRepeat, const ModifierKeyState& ModierKeyState)
{
    // TODO: Maybe a better solution that this?
    ImGuiIO& IO = ImGui::GetIO();
    if (!IO.WantCaptureKeyboard)
    {
        Input::RegisterKeyDown(KeyCode);
    }

    KeyPressedEvent Event(KeyCode, IsRepeat, ModierKeyState);
    SendEvent(Event);
}

void EventDispatcher::OnCharacterInput(uint32 Character)
{
    KeyTypedEvent Event(Character);
    SendEvent(Event);
}

void EventDispatcher::OnMouseMove(int32 x, int32 y)
{
    MouseMovedEvent Event(x, y);
    SendEvent(Event);
}

void EventDispatcher::OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState)
{
    GenericWindow* CaptureWindow = Application->GetCapture();
    if (CaptureWindow)
    {
        SetCapture(nullptr);
    }

    MouseReleasedEvent Event(Button, ModierKeyState);
    SendEvent(Event);
}

void EventDispatcher::OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState)
{
    GenericWindow* CaptureWindow = Application->GetCapture();
    if (!CaptureWindow)
    {
        GenericWindow* ActiveWindow = Application->GetActiveWindow();
        Application->SetCapture(ActiveWindow);
    }

    MousePressedEvent Event(Button, ModierKeyState);
    SendEvent(Event);
}

void EventDispatcher::OnMouseScrolled(float HorizontalDelta, float VerticalDelta)
{
    MouseScrolledEvent Event(HorizontalDelta, VerticalDelta);
    SendEvent(Event);
}

void EventDispatcher::OnWindowResized(const TRef<GenericWindow>& InWindow, uint16 Width, uint16 Height)
{
    WindowResizeEvent Event(InWindow, Width, Height);
    SendEvent(Event);
}

void EventDispatcher::OnWindowMoved(const TRef<GenericWindow>& Window, int16 x, int16 y)
{
    WindowMovedEvent Event(Window, x, y);
    SendEvent(Event);
}

void EventDispatcher::OnWindowFocusChanged(const TRef<GenericWindow>& Window, bool HasFocus)
{
    // TODO: What if other windows loose focus?
    if (!HasFocus)
    {
        Input::ClearState();
    }

    WindowFocusChangedEvent Event(Window, HasFocus);
    SendEvent(Event);
}

void EventDispatcher::OnWindowMouseLeft(const TRef<GenericWindow>& Window)
{
    WindowMouseLeftEvent Event(Window);
    SendEvent(Event);
}

void EventDispatcher::OnWindowMouseEntered(const TRef<GenericWindow>& Window)
{
    WindowMouseEnteredEvent Event(Window);
    SendEvent(Event);
}

void EventDispatcher::OnWindowClosed(const TRef<GenericWindow>& Window)
{
    WindowClosedEvent Event(Window);
    SendEvent(Event);
}
