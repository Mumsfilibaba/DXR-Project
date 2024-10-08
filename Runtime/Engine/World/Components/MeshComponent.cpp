#include "MeshComponent.h"
#include "ProxySceneComponent.h"

FOBJECT_IMPLEMENT_CLASS(FMeshComponent);

FMeshComponent::FMeshComponent(const FObjectInitializer& ObjectInitializer)
    : FSceneComponent(ObjectInitializer)
    , Mesh(nullptr)
    , Materials()
{
}

FProxySceneComponent* FMeshComponent::CreateProxyComponent()
{
    FProxySceneComponent* NewComponent = new FProxySceneComponent();
    NewComponent->Geometry     = Mesh->RTGeometry.Get();
    NewComponent->VertexBuffer = Mesh->VertexBuffer.Get();
    NewComponent->NumVertices  = Mesh->VertexCount;
    NewComponent->IndexBuffer  = Mesh->IndexBuffer.Get();
    NewComponent->NumIndices   = Mesh->IndexCount;
    NewComponent->IndexFormat  = Mesh->IndexFormat;

    NewComponent->CurrentActor = GetActorOwner();
    NewComponent->Mesh         = Mesh;
    NewComponent->Materials    = Materials;
    return NewComponent;
}

TSharedPtr<FMesh> FMeshComponent::GetMesh() const
{
    return Mesh;
}

TSharedPtr<FMaterial> FMeshComponent::GetMaterial(int32 Index) const
{
    return (Materials.Size() > Index) ? Materials[Index] : nullptr;
}

int32 FMeshComponent::GetNumMaterials() const
{
    return Materials.Size();
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
    
    Materials[Index] = InMaterial;
}
