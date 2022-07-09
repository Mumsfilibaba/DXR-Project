#pragma once
#include "Component.h"

#include "Core/Containers/SharedPtr.h"

#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FMeshComponent

class ENGINE_API FMeshComponent : public CComponent
{
    CORE_OBJECT(FMeshComponent, CComponent);

public:

    FMeshComponent(FActor* InActorOwner)
        : CComponent(InActorOwner, false, false)
        , Material(nullptr)
        , Mesh(nullptr)
    {
        CORE_OBJECT_INIT();
    }

    TSharedPtr<FMaterial> Material;
    TSharedPtr<FMesh>     Mesh;
};
