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
	WindowResizeEvent(TSharedRef<GenericWindow> InWindow, uint16 InWidth, uint16 InHeight)
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

	FORCEINLINE uint16 GetWidth() const
	{
		return Width;
	}

	FORCEINLINE uint16 GetHeight() const
	{
		return Height;
	}

private:
	TSharedRef<GenericWindow> Window;
	uint16 Width;
	uint16 Height;
};