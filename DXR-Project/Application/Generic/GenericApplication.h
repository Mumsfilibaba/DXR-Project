#pragma once
#include "Application/InputCodes.h"

/*
* ModifierKeyState
*/
class ModifierKeyState
{
public:
	inline ModifierKeyState(Uint32 InModifierMask)
		: ModifierMask(InModifierMask)
	{
	}

	FORCEINLINE bool IsCtrlDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_CTRL);
	}

	FORCEINLINE bool IsAltDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_ALT);
	}

	FORCEINLINE bool IsShiftDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_SHIFT);
	}

	FORCEINLINE bool IsCapsLockDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_CAPS_LOCK);
	}

	FORCEINLINE bool IsSuperKeyDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_SUPER);
	}

	FORCEINLINE bool IsNumPadDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_NUM_LOCK);
	}

private:
	Uint32 ModifierMask = 0;
};

/*
* GenericApplication
*/
class GenericApplication
{

};