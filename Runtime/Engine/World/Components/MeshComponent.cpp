#include "MeshComponent.h"
#include "ProxySceneComponent.h"

FOBJECT_IMPLEMENT_CLASS(FMeshComponent);

FMeshComponent::FMeshComponent(const FObjectInitializer& ObjectInitializer)
    : FSceneComponent(ObjectInitializer)
    , Material(nullptr)
    , Mesh(nullptr)
{
}

FProxySceneComponent* FMeshComponent::CreateProxyComponent()
{
    FProxySceneComponent* NewComponent = new FProxySceneComponent();
    NewComponent->CurrentActor = GetActorOwner();
    NewComponent->Geometry     = Mesh->RTGeometry.Get();
    NewComponent->VertexBuffer = Mesh->VertexBuffer.Get();
    NewComponent->NumVertices  = Mesh->VertexCount;
    NewComponent->IndexBuffer  = Mesh->IndexBuffer.Get();
    NewComponent->NumIndices   = Mesh->IndexCount;
    NewComponent->IndexFormat  = Mesh->IndexFormat;
    NewComponent->Material     = Material.Get();
    NewComponent->Mesh         = Mesh.Get();
    return NewComponent;
}