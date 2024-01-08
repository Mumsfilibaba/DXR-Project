#pragma once
#include "Component.h"
#include "Core/Containers/SharedPtr.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

class ENGINE_API FMeshComponent : public FComponent
{
public:
    FOBJECT_DECLARE_CLASS(FMeshComponent, FComponent, ENGINE_API);

    FMeshComponent(const FObjectInitializer& ObjectInitializer);
    ~FMeshComponent() = default;

    TSharedPtr<FMaterial> Material;
    TSharedPtr<FMesh>     Mesh;
};
