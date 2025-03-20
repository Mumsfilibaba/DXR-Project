#pragma once

struct ISceneObject
{
    virtual ~ISceneObject() = default;

    // Tick syncs with the world and updates the renderer representation of the object
    virtual void Tick() = 0;
};