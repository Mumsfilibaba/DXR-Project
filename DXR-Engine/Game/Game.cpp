#include "Game.h"

Game::Game()
    : CurrentScene(nullptr)
    , CurrentCamera(nullptr)
{
}

Game::~Game()
{
    SafeDelete(CurrentScene);
}