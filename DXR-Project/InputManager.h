#pragma once
#include "Defines.h"
#include "Types.h"

enum Key : Uint16
{

};

class InputManager
{
public:
	InputManager();
	~InputManager();

	void RegisterKeyUp();
	void RegisterKeyDown();

	bool IsKeyUp(Key KeyCode);
	bool IsKeyDown(Key KeyCode);

	static InputManager& Get();

private:
	bool KeyState[];
};