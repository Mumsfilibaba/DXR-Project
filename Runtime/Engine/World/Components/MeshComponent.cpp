#include "MeshComponent.h"
#include "ProxyRendererComponent.h"

FOBJECT_IMPLEMENT_CLASS(FMeshComponent);

FMeshComponent::FMeshComponent(const FObjectInitializer& ObjectInitializer)
    : FRendererComponent(ObjectInitializer)
    , Material(nullptr)
    , Mesh(nullptr)
{
}

FProxyRendererComponent* FMeshComponent::CreateProxyComponent()
{
    FProxyRendererComponent* NewComponent = new FProxyRendererComponent();
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