#include "MeshComponent.h"

FOBJECT_IMPLEMENT_CLASS(FMeshComponent);

FMeshComponent::FMeshComponent(const FObjectInitializer& ObjectInitializer)
    : FComponent(ObjectInitializer)
    , Material(nullptr)
    , Mesh(nullptr)
{
    bIsTickable  = false;
    bIsStartable = false;
}
