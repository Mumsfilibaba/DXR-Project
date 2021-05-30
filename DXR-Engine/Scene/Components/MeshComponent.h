#pragma once
#include "Scene/Actor.h"

class MeshComponent : public Component
{
    CORE_OBJECT(MeshComponent, Component);

public:
    MeshComponent(Actor* InOwningActor)
        : Component(InOwningActor)
        , Material(nullptr)
        , Mesh(nullptr)
    {
        CORE_OBJECT_INIT();
    }

    TSharedPtr<class CMaterial> Material;
    TSharedPtr<class Mesh>     Mesh;
};