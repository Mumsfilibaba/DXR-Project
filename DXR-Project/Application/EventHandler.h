#pragma once
#include "InputCodes.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

class WindowsWindow;

class EventHandler
{
public:
	virtual ~EventHandler() = default;

	virtual void OnWindowResize(std::shared_ptr<WindowsWindow>& Window, Uint16 Width, Uint16 Height)
	{
	}

	virtual void OnKeyUp(EKey KeyCode)
	{
	}

	virtual void OnKeyDown(EKey KeyCode)
	{
	}

	virtual void OnMouseMove(Int32 X, Int32 Y)
	{
	}

protected:
	EventHandler() = default;
};

#pragma warning(pop)