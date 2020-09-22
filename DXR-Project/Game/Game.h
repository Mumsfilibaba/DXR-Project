#pragma once
#include "Defines.h"
#include "Types.h"

#include "Scene/Scene.h"

/*
* Game
*/

class Game
{
public:
	Game();
	~Game();

	bool Initialize();

	void Tick(Timestamp DeltaTime);

	static Game& Get();

private:
	Scene* CurrentScene = nullptr;
	Camera* CurrentCamera = nullptr;

	static Game* CurrentGame;
};