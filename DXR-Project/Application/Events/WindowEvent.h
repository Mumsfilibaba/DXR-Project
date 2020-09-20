#pragma once
#include "Event.h"

#include "Containers/TSharedPtr.h"

class GenericWindow;

/*
* WindowResizeEvent 
*/
struct WindowResizeEvent : public Event
{
public:
	WindowResizeEvent(TSharedPtr<GenericWindow> InWindow, Uint16 InWidth, Uint16 InHeight)
		: Window(InWindow)
		, Width(InWidth)
		, Height(InHeight)
	{
	}

	DECLARE_EVENT_CLASS(WINDOW_RESIZED_EVENT);

	DECLARE_EVENT_CATEGORY(EEventCategory::EVENT_CATEGORY_WINDOW);

	virtual std::string ToString() const override
	{
		return std::string("WindowResizeEvent=[") + std::to_string(Width) + ", " + std::to_string(Height) + "]";
	}

	FORCEINLINE TSharedPtr<GenericWindow> GetWindow() const
	{
		return Window;
	}

	FORCEINLINE Uint16 GetWidth() const
	{
		return Width;
	}

	FORCEINLINE Uint16 GetHeight() const
	{
		return Height;
	}

private:
	TSharedPtr<GenericWindow> Window;
	Uint16 Width;
	Uint16 Height;
};