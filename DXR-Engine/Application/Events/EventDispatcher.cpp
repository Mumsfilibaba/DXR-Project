#include "EventDispatcher.h"

#include "Application/Input.h"

/*
* EventDispatcher
*/

EventDispatcher::EventDispatcher(GenericApplication* InApplication)
	: ApplicationEventHandler()
	, Application(InApplication)
	, EventHandlers()
{
}

void EventDispatcher::RegisterEventHandler(EventHandlerFunc Func, UInt8 EventCategoryMask)
{
	VALIDATE(Func != nullptr);

	for (EventHandlerPair& Pair : EventHandlers)
	{
		if (Pair.IsHandlerFunc)
		{
			if (Pair.Func == Func)
			{
				LOG_WARNING("Function is already registered as a EventHandler, will not be registered again");
				return;
			}
		}
	}

	EventHandlers.EmplaceBack(Func, EventCategoryMask);
}

void EventDispatcher::RegisterEventHandler(IEventHandler* EventHandler, UInt8 EventCategoryMask)
{
	VALIDATE(EventHandler != nullptr);

	for (EventHandlerPair& Pair : EventHandlers)
	{
		if (!Pair.IsHandlerFunc)
		{
			if (Pair.Handler == EventHandler)
			{
				LOG_WARNING("Handler is already registered as a EventHandler, will not be registered again");
				return;
			}
		}
	}

	EventHandlers.EmplaceBack(EventHandler, EventCategoryMask);
}

void EventDispatcher::UnregisterEventHandler(IEventHandler* EventHandler)
{
	VALIDATE(EventHandler != nullptr);

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
	VALIDATE(Func != nullptr);

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
					return true;
				}
			}
			else
			{
				if (Pair.Handler->OnEvent(Event))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void EventDispatcher::OnWindowResized(TSharedRef<GenericWindow> InWindow, UInt16 Width, UInt16 Height)
{
	WindowResizeEvent Event(InWindow, Width, Height);
	SendEvent(Event);
}

void EventDispatcher::OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState)
{
	Input::RegisterKeyUp(KeyCode);

	KeyReleasedEvent Event(KeyCode, ModierKeyState);
	SendEvent(Event);
}

void EventDispatcher::OnKeyPressed(EKey KeyCode, const ModifierKeyState& ModierKeyState)
{
	Input::RegisterKeyDown(KeyCode);

	KeyPressedEvent Event(KeyCode, ModierKeyState);
	SendEvent(Event);
}

void EventDispatcher::OnMouseMove(Int32 x, Int32 y)
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

void EventDispatcher::OnMouseScrolled(Float HorizontalDelta, Float VerticalDelta)
{
	MouseScrolledEvent Event(HorizontalDelta, VerticalDelta);
	SendEvent(Event);
}

void EventDispatcher::OnCharacterInput(UInt32 Character)
{
	KeyTypedEvent Event(Character);
	SendEvent(Event);
}