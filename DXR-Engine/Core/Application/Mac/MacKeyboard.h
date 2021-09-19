#pragma once 

#if defined(PLATFORM_MACOS)
#include "Core/Application/IKeyboard.h"

class CMacKeyboard final : public IKeyboard
{
	friend class CMacApplication;
	
public:
		
    /* Check if the current keystate is pressed */
    virtual bool IsKeyDown( EKey KeyCode ) const override final;

    /* Check if the current keystate is released */
    virtual bool IsKeyUp( EKey KeyCode ) const override final;
	
	FORCEINLINE EKey GetKeyFromKeyCode( uint32 ScanCode ) const
	{
		return KeyCodeFromScanCode[ScanCode];
	}

private:
	
	CMacKeyboard()  = default;
	~CMacKeyboard() = default;

	bool InitKeyTables();

	FORCEINLINE void RegisterKeyState( EKey Key, bool State )
	{
		KeyState[Key] = State;
	}
	
	/* Lookup table for converting from scancode to enum */
	EKey KeyCodeFromScanCode[256];
	
	/* state of the keys, true means pressed */
	bool KeyState[EKey::Key_Count];
};

#endif
