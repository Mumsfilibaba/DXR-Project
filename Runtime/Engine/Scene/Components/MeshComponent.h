#pragma once
#include "Component.h"
#include "Core/Containers/SharedPtr.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

class ENGINE_API FMeshComponent 
    : public FComponent
{
    FOBJECT_BODY(FMeshComponent, FComponent);

public:
    FMeshComponent(FActor* InActorOwner)
        : FComponent(InActorOwner, false, false)
        , Material(nullptr)
        , Mesh(nullptr)
    {
        FOBJECT_INIT();
    }

    TSharedPtr<FMaterial> Material;
    TSharedPtr<FMesh>     Mesh;
};
