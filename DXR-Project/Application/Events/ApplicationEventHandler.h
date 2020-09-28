#pragma once
#include "Application/InputCodes.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

class GenericWindow;
struct ModifierKeyState;

/*
* ApplicationEventHandler
*/
class ApplicationEventHandler
{
public:
	virtual ~ApplicationEventHandler() = default;

	virtual void OnWindowResized(TSharedRef<GenericWindow> Window, Uint16 Width, Uint16 Height)
	{
	}

	virtual void OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState)
	{
	}

	virtual void OnKeyPressed(EKey KeyCode, const ModifierKeyState& ModierKeyState)
	{
	}

	virtual void OnCharacterInput(Uint32 Character)
	{
	}

	virtual void OnMouseMove(Int32 x, Int32 y)
	{
	}

	virtual void OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState)
	{
	}

	virtual void OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState)
	{
	}

	virtual void OnMouseScrolled(Float32 HorizontalDelta, Float32 VerticalDelta)
	{
	}

protected:
	ApplicationEventHandler() = default;
};

#pragma warning(pop)