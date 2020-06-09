#pragma once
#include "Types.h"

class WindowsWindow;

class EventHandler
{
	EventHandler(EventHandler&& Other)		= delete;
	EventHandler(const EventHandler& Other) = delete;

	EventHandler& operator=(EventHandler&& Other)		= delete;
	EventHandler& operator=(const EventHandler& Other)	= delete;

public:
	virtual void OnWindowResize(WindowsWindow* Window, Uint16 Width, Uint16 Height)
	{
	}

protected:
	EventHandler()	= default;
	~EventHandler()	= default;
};