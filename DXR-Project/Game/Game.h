#pragma once
#include "Scene/Scene.h"

/*
* Game
*/

class Game
{
public:
	Game();
	~Game();

	Bool Init();

	void Tick(Timestamp DeltaTime);

private:
	Scene*	CurrentScene	= nullptr;
	Camera* CurrentCamera	= nullptr;
};