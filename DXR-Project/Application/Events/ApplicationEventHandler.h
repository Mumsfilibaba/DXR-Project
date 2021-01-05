#pragma once
#include "Application/InputCodes.h"

#ifdef COMPILER_VISUAL_STUDIO
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class GenericWindow;
struct ModifierKeyState;

/*
* ApplicationEventHandler
*/

class ApplicationEventHandler
{
public:
	virtual ~ApplicationEventHandler() = default;

	virtual void OnWindowResized(TSharedRef<GenericWindow> Window, UInt16 Width, UInt16 Height)
	{
	}

	virtual void OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState)
	{
	}

	virtual void OnKeyPressed(EKey KeyCode, const ModifierKeyState& ModierKeyState)
	{
	}

	virtual void OnCharacterInput(UInt32 Character)
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

	virtual void OnMouseScrolled(Float HorizontalDelta, Float VerticalDelta)
	{
	}

protected:
	ApplicationEventHandler() = default;
};

#ifdef COMPILER_VISUAL_STUDIO
#pragma warning(pop)
#endif