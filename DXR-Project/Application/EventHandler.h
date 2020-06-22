#pragma once
#include "InputCodes.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

class WindowsWindow;

class EventHandler
{
public:
	virtual ~EventHandler() = default;

	virtual void OnWindowResize(std::shared_ptr<WindowsWindow>& InWindow, Uint16 InWidth, Uint16 InHeight)
	{
	}

	virtual void OnKeyUp(EKey InKeyCode)
	{
	}

	virtual void OnKeyDown(EKey InKeyCode)
	{
	}

	virtual void OnMouseMove(Int32 InX, Int32 InY)
	{
	}

	virtual void OnMouseButtonReleased(EMouseButton InButton)
	{
	}

	virtual void OnMouseButtonPressed(EMouseButton InButton)
	{
	}

protected:
	EventHandler() = default;
};

#pragma warning(pop)