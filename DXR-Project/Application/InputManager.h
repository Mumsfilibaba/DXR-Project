#pragma once
#include "InputCodes.h"

class InputManager
{
public:
	EKey	ConvertFromKeyCode(Uint32 KeyCode);
	Uint32	ConvertToKeyCode(EKey Key);

	void RegisterKeyUp(EKey KeyCode);
	void RegisterKeyDown(EKey KeyCode);

	bool IsKeyUp(EKey KeyCode);
	bool IsKeyDown(EKey KeyCode);

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