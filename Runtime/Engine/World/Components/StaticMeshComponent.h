#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Engine/Resources/Model.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Components/SceneComponent.h"

class ENGINE_API FStaticMeshComponent : public FSceneComponent
{
public:
    FOBJECT_DECLARE_CLASS(FStaticMeshComponent, FSceneComponent);

    FStaticMeshComponent(const FObjectInitializer& ObjectInitializer);
    ~FStaticMeshComponent();

    void SetMesh(const TSharedPtr<FMesh>& InMesh);
    void SetMaterial(const TSharedPtr<FMaterial>& InMaterial, int32 Index = 0);

    TSharedPtr<FMesh> GetMesh() const
    {
        return Mesh;
    }

    TSharedPtr<FMaterial> GetMaterial(int32 Index = 0) const
    {
        return (Materials.Size() > Index) ? Materials[Index] : nullptr;
    }

    const TArray<TSharedPtr<FMaterial>> GetMaterials() const 
    {
        return Materials;
    }

    int32 GetNumMaterials() const
    {
        return Materials.Size();
    }

private:
    TSharedPtr<FMesh>             Mesh;
    TArray<TSharedPtr<FMaterial>> Materials;
};
