#pragma once
#include "Defines.h"

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
	void Destroy();

	void Tick(Timestamp DeltaTime);

	static Game& GetCurrent();
	static void SetCurrent(Game* InCurrentGame);

private:
	Scene*	CurrentScene = nullptr;
	Camera* CurrentCamera = nullptr;

	static Game* CurrentGame;
};