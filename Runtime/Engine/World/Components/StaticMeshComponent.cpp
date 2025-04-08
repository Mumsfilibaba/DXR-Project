#include "Engine/World/Components/StaticMeshComponent.h"

FOBJECT_IMPLEMENT_CLASS(FStaticMeshComponent);

FStaticMeshComponent::FStaticMeshComponent(const FObjectInitializer& ObjectInitializer)
    : FSceneComponent(ObjectInitializer)
    , Mesh(nullptr)
    , Materials()
{
}

FStaticMeshComponent::~FStaticMeshComponent()
{
}

void FStaticMeshComponent::SetMesh(const TSharedPtr<FMesh>& InMesh)
{
    Mesh = InMesh;
    
    const int32 NumSubMeshes = Mesh->GetNumSubMeshes();
    if (Materials.Size() < NumSubMeshes)
    {
        Materials.Resize(NumSubMeshes);
    }
}

void FStaticMeshComponent::SetMaterial(const TSharedPtr<FMaterial>& InMaterial, int32 Index)
{
    if (Materials.Size() <= Index)
    {
        Materials.Resize(Index + 1);
    }
    
    CHECK(InMaterial != nullptr);
    Materials[Index] = InMaterial;
}
