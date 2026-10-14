#include "Engine/Engine.h"
#include "Engine/Assets/PrimitiveShapeMeshCache.h"
#include "Engine/World/World.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/PrimitiveShapeComponent.h"

FOBJECT_IMPLEMENT_CLASS(FPrimitiveShapeComponent);

FPrimitiveShapeComponent::FPrimitiveShapeComponent(const FObjectInitializer& ObjectInitializer)
    : FStaticMeshComponent(ObjectInitializer)
    , Shape(PrimitiveShapes::MakeDefault(EPrimitiveShape::Cube))
    , BuiltShape(Shape)
    , PendingRequest(0)
    , OnMeshChanged()
{
}

FPrimitiveShapeComponent::~FPrimitiveShapeComponent()
{
    if (PendingRequest)
    {
        FPrimitiveShapeMeshCache::Get().Cancel(PendingRequest);
    }
}

void FPrimitiveShapeComponent::SetShape(const FPrimitiveShape& InShape)
{
    const FPrimitiveShape NewShape = PrimitiveShapes::Clamp(InShape);
    if (NewShape == Shape && (IsBuilding() || GetMesh()))
    {
        return;
    }

    if (PendingRequest)
    {
        FPrimitiveShapeMeshCache::Get().Cancel(PendingRequest);
        PendingRequest = 0;
    }

    Shape = NewShape;

    uint64            Request = 0;
    TSharedPtr<FMesh> NewMesh = FPrimitiveShapeMeshCache::Get().Request(Shape, FOnShapeMeshBuilt::CreateRaw(this, &FPrimitiveShapeComponent::OnMeshBuilt, NewShape), Request);

    if (Request)
    {
        PendingRequest = Request;
        return;
    }

    if (NewMesh)
    {
        BuiltShape = Shape;
        ApplyMesh(NewMesh);
    }
    else
    {
        Shape = BuiltShape;
    }

    OnMeshChanged.Broadcast();
}

void FPrimitiveShapeComponent::OnMeshBuilt(const TSharedPtr<FMesh>& InMesh, FPrimitiveShape InShape)
{
    PendingRequest = 0;

    if (InMesh)
    {
        BuiltShape = InShape;
        ApplyMesh(InMesh);
    }
    else
    {
        Shape = BuiltShape;
    }

    OnMeshChanged.Broadcast();
}

void FPrimitiveShapeComponent::ApplyMesh(const TSharedPtr<FMesh>& InMesh)
{
    FWorld* World = GetActorOwner() ? GetActorOwner()->GetWorld() : nullptr;
    if (World)
    {
        World->RemoveSceneComponent(this);
    }

    SetMesh(InMesh);

    const TSharedPtr<FMaterial> BaseMaterial = FEngine::Get() ? FEngine::Get()->BaseMaterial : nullptr;
    for (int32 Index = 0; Index < GetNumMaterials(); ++Index)
    {
        if (!GetMaterial(Index) && BaseMaterial)
        {
            SetMaterial(BaseMaterial, Index);
        }
    }

    if (World)
    {
        World->AddSceneComponent(this);
    }
}
