#include "Game.h"

/*
* Game
*/

Game::Game()
	: CurrentScene(nullptr)
	, CurrentCamera(nullptr)
{
}

Game::~Game()
{
	SAFEDELETE(CurrentScene);
}