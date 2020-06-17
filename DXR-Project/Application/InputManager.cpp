#include "InputManager.h"

InputManager::InputManager()
{
	for (Uint32 i = 0; i < EKey::KEY_COUNT; i++)
	{
		KeyStates[i] = false;
	}

	InitializeKeyMappings();
}

InputManager::~InputManager()
{
}

void InputManager::InitializeKeyMappings()
{
}

EKey InputManager::ConvertFromKeyCode(Uint32 KeyCode)
{
	return EKey();
}

Uint32 InputManager::ConvertToKeyCode(EKey Key)
{
	return Uint32();
}

void InputManager::RegisterKeyUp(EKey KeyCode)
{
	KeyStates[KeyCode] = false;
}

void InputManager::RegisterKeyDown(EKey KeyCode)
{
	KeyStates[KeyCode] = true;
}

bool InputManager::IsKeyUp(EKey KeyCode)
{
	return KeyStates[KeyCode] == false;
}

bool InputManager::IsKeyDown(EKey KeyCode)
{
	return KeyStates[KeyCode] == true;
}

InputManager& InputManager::Get()
{
	static InputManager Instance;
	return Instance;
}
