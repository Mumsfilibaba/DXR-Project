#pragma once
#include "Scene/Scene.h"

/*
* MakeGameInstance
*/

extern class Game* MakeGameInstance();

/*
* Game
*/

class Game
{
public:
	Game();
	~Game();

	virtual Bool Init()
	{
		return true;
	}

	virtual void Tick(Timestamp DeltaTime)
	{
	}

protected:
	Scene*	CurrentScene	= nullptr;
	Camera* CurrentCamera	= nullptr;
};