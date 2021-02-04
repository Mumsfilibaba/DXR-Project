#pragma once
#include "Scene/Scene.h"

extern class Game* MakeGameInstance();

class Game
{
public:
    Game();
    virtual ~Game();

    virtual Bool Init()
    {
        return true;
    }

    virtual void Tick(Timestamp DeltaTime)
    {
        UNREFERENCED_VARIABLE(DeltaTime);
    }

    Scene* GetCurrentScene() const { return CurrentScene; }

protected:
    Scene*  CurrentScene  = nullptr;
    Camera* CurrentCamera = nullptr;
};