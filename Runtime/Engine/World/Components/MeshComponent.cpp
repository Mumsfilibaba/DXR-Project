#include "Engine/World/Components/MeshComponent.h"

FOBJECT_IMPLEMENT_CLASS(FMeshComponent);

FMeshComponent::FMeshComponent(const FObjectInitializer& ObjectInitializer)
    : FSceneComponent(ObjectInitializer)
    , Mesh(nullptr)
    , Materials()
{
}

FMeshComponent::~FMeshComponent()
{
}

void FMeshComponent::SetMesh(const TSharedPtr<FMesh>& InMesh)
{
    Mesh = InMesh;
    
    const int32 NumSubMeshes = Mesh->GetNumSubMeshes();
    if (Materials.Size() < NumSubMeshes)
    {
        Materials.Resize(NumSubMeshes);
    }
}

void FMeshComponent::SetMaterial(const TSharedPtr<FMaterial>& InMaterial, int32 Index)
{
    if (Materials.Size() <= Index)
    {
        Materials.Resize(Index + 1);
    }
    
    CHECK(InMaterial != nullptr);
    Materials[Index] = InMaterial;
}
