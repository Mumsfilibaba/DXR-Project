#pragma once

class FScene;

class FSceneObject
{
public:
    FSceneObject(FScene* InScene);
    virtual ~FSceneObject();

    FScene* GetScene() const
    {
        return Scene;
    }

private:
    FScene* Scene;
};