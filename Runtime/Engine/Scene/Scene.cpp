#include "Scene.h"

#include "Components/MeshComponent.h"

#include "Engine/Assets/MeshFactory.h"
#include "Engine/Resources/TextureFactory.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Mesh.h"

#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Scene

FScene::FScene()
    : Actors()
{
}

FScene::~FScene()
{
    for (FActor* CurrentActor : Actors)
    {
        SafeDelete(CurrentActor);
    }
    Actors.Clear();

    for (CLight* CurrentLight : Lights)
    {
        SafeDelete(CurrentLight);
    }
    Lights.Clear();

    SafeDelete(CurrentCamera);
}

FActor* FScene::MakeActor()
{
    FActor* NewActor = dbg_new FActor(this);
    AddActor(NewActor);
    return NewActor;
}

void FScene::Start()
{
    for (FActor* Actor : Actors)
    {
        if (Actor->IsStartable())
        {
            Actor->Start();
        }
    }
}

void FScene::Tick(FTimestamp DeltaTime)
{
    for (FActor* Actor : Actors)
    {
        if (Actor->IsTickable())
        {
            Actor->Tick(DeltaTime);
        }
    }
}

void FScene::AddCamera(CCamera* InCamera)
{
    if (CurrentCamera)
    {
        SafeDelete(CurrentCamera);
    }

    CurrentCamera = InCamera;
}

void FScene::AddActor(FActor* InActor)
{
    Check(InActor != nullptr);
    Actors.Emplace(InActor);

    FMeshComponent* Component = InActor->GetComponentOfType<FMeshComponent>();
    if (Component)
    {
        AddMeshComponent(Component);
    }
}

void FScene::AddLight(CLight* InLight)
{
    Check(InLight != nullptr);
    Lights.Emplace(InLight);
}

void FScene::OnAddedComponent(CComponent* NewComponent)
{
    FMeshComponent* Component = Cast<FMeshComponent>(NewComponent);
    if (Component && Component->Mesh)
    {
        AddMeshComponent(Component);
    }
}

void FScene::AddMeshComponent(FMeshComponent* Component)
{
    SMeshDrawCommand Command;
    Command.CurrentActor = Component->GetActor();
    Command.Geometry = Component->Mesh->RTGeometry.Get();
    Command.VertexBuffer = Component->Mesh->VertexBuffer.Get();
    Command.IndexBuffer = Component->Mesh->IndexBuffer.Get();
    Command.Material = Component->Material.Get();
    Command.Mesh = Component->Mesh.Get();
    MeshDrawCommands.Push(Command);
}
