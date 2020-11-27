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
	WindowResizeEvent(TSharedRef<GenericWindow> InWindow, UInt16 InWidth, UInt16 InHeight)
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

	FORCEINLINE TSharedRef<GenericWindow> GetWindow() const
	{
		return Window;
	}

	FORCEINLINE UInt16 GetWidth() const
	{
		return Width;
	}

	FORCEINLINE UInt16 GetHeight() const
	{
		return Height;
	}

private:
	TSharedRef<GenericWindow> Window;
	UInt16 Width;
	UInt16 Height;
};