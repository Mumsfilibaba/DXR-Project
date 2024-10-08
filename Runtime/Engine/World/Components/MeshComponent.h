#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Material.h"
#include "Engine/World/Components/SceneComponent.h"

class ENGINE_API FMeshComponent : public FSceneComponent
{
public:
    FOBJECT_DECLARE_CLASS(FMeshComponent, FSceneComponent);

    FMeshComponent(const FObjectInitializer& ObjectInitializer);
    ~FMeshComponent() = default;

    virtual FProxySceneComponent* CreateProxyComponent() override final;

    TSharedPtr<FMesh>     GetMesh() const;
    TSharedPtr<FMaterial> GetMaterial(int32 Index = 0) const;
    
    int32 GetNumMaterials() const;

    void SetMesh(const TSharedPtr<FMesh>& InMesh);
    void SetMaterial(const TSharedPtr<FMaterial>& InMaterial, int32 Index = 0);

private:
    TSharedPtr<FMesh>             Mesh;
    TArray<TSharedPtr<FMaterial>> Materials;
};
