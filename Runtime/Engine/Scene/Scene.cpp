#include "Scene.h"

#include "Components/MeshComponent.h"

#include "Engine/Assets/MeshFactory.h"
#include "Engine/Resources/TextureFactory.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Mesh.h"

#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Scene

CScene::CScene()
    : Actors()
{
}

CScene::~CScene()
{
    for (CActor* CurrentActor : Actors)
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

CActor* CScene::MakeActor()
{
    CActor* NewActor = dbg_new CActor(this);
    AddActor(NewActor);
    return NewActor;
}

void CScene::Start()
{
    for (CActor* Actor : Actors)
    {
        if (Actor->IsStartable())
        {
            Actor->Start();
        }
    }
}

void CScene::Tick(CTimestamp DeltaTime)
{
    for (CActor* Actor : Actors)
    {
        if (Actor->IsTickable())
        {
            Actor->Tick(DeltaTime);
        }
    }
}

void CScene::AddCamera(CCamera* InCamera)
{
    if (CurrentCamera)
    {
        SafeDelete(CurrentCamera);
    }

    CurrentCamera = InCamera;
}

void CScene::AddActor(CActor* InActor)
{
    Assert(InActor != nullptr);
    Actors.Emplace(InActor);

    CMeshComponent* Component = InActor->GetComponentOfType<CMeshComponent>();
    if (Component)
    {
        AddMeshComponent(Component);
    }
}

void CScene::AddLight(CLight* InLight)
{
    Assert(InLight != nullptr);
    Lights.Emplace(InLight);
}

void CScene::OnAddedComponent(CComponent* NewComponent)
{
    CMeshComponent* Component = Cast<CMeshComponent>(NewComponent);
    if (Component && Component->Mesh)
    {
        AddMeshComponent(Component);
    }
}

void CScene::AddMeshComponent(CMeshComponent* Component)
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
