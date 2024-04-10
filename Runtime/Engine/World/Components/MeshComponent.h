#pragma once
#include "SceneComponent.h"
#include "Core/Containers/SharedPtr.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"

class ENGINE_API FMeshComponent : public FSceneComponent
{
public:
    FOBJECT_DECLARE_CLASS(FMeshComponent, FSceneComponent);

    FMeshComponent(const FObjectInitializer& ObjectInitializer);
    ~FMeshComponent() = default;

    virtual FProxySceneComponent* CreateProxyComponent() override final;

    TSharedPtr<FMesh> GetMesh() const
    {
        return Mesh;
    } 
    
    TSharedPtr<FMaterial> GetMaterial() const
    {
        return Material;
    }

    void SetMesh(const TSharedPtr<FMesh>& InMesh)
    {
        Mesh = InMesh;
    }
    
    void SetMaterial(const TSharedPtr<FMaterial>& InMaterial)
    {
        Material = InMaterial;
    }

private:
    TSharedPtr<FMesh>     Mesh;
    TSharedPtr<FMaterial> Material;
};
