#pragma once
#include "Component.h"

#include "Core/Containers/SharedPtr.h"

#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FMeshComponent

class ENGINE_API FMeshComponent : public FComponent
{
    CORE_OBJECT(FMeshComponent, FComponent);

public:

    FMeshComponent(FActor* InActorOwner)
        : FComponent(InActorOwner, false, false)
        , Material(nullptr)
        , Mesh(nullptr)
    {
        CORE_OBJECT_INIT();
    }

    TSharedPtr<FMaterial> Material;
    TSharedPtr<FMesh>     Mesh;
};
