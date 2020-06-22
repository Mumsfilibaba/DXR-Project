#pragma once
#include "InputCodes.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

class WindowsWindow;
class ModifierKeyState;

class EventHandler
{
public:
	virtual ~EventHandler() = default;

	virtual void OnWindowResized(std::shared_ptr<WindowsWindow>& InWindow, Uint16 InWidth, Uint16 InHeight)
	{
	}

	virtual void OnKeyReleased(EKey InKeyCode, const ModifierKeyState& InModierKeyState)
	{
	}

	virtual void OnKeyPressed(EKey InKeyCode, const ModifierKeyState& InModierKeyState)
	{
	}

	virtual void OnCharacterInput(Uint32 Character)
	{
	}

	virtual void OnMouseMove(Int32 InX, Int32 InY)
	{
	}

	virtual void OnMouseButtonReleased(EMouseButton InButton, const ModifierKeyState& InModierKeyState)
	{
	}

	virtual void OnMouseButtonPressed(EMouseButton InButton, const ModifierKeyState& InModierKeyState)
	{
	}

	virtual void OnMouseScrolled(Float32 InHorizontalDelta, Float32 InVerticalDelta)
	{
	}

protected:
	EventHandler() = default;
};

#pragma warning(pop)