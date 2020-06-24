#pragma once
#include "InputCodes.h"

class InputManager
{
public:
	EKey	ConvertFromKeyCode(Uint32 InKeyCode);
	Uint32	ConvertToKeyCode(EKey InKey);

	void RegisterKeyUp(EKey InKeyCode);
	void RegisterKeyDown(EKey InKeyCode);

	bool IsKeyUp(EKey InKeyCode)		const;
	bool IsKeyDown(EKey InKeyCode)	const;

	static InputManager& Get();

private:
	InputManager();
	~InputManager();

	void InitializeKeyTables();

private:
	bool	KeyStates[EKey::KEY_COUNT];
	EKey	ScanCodeTable[512];
	Uint16	KeyTable[512];
};