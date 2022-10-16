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
        SAFE_DELETE(CurrentActor);
    }
    Actors.Clear();

    for (FLight* CurrentLight : Lights)
    {
        SAFE_DELETE(CurrentLight);
    }
    Lights.Clear();

    SAFE_DELETE(CurrentCamera);
}

FActor* FScene::CreateActor()
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

void FScene::Tick(FTimespan DeltaTime)
{
    for (FActor* Actor : Actors)
    {
        if (Actor->IsTickable())
        {
            Actor->Tick(DeltaTime);
        }
    }
}

void FScene::AddCamera(FCamera* InCamera)
{
    if (CurrentCamera)
    {
        SAFE_DELETE(CurrentCamera);
    }

    CurrentCamera = InCamera;
}

void FScene::AddActor(FActor* InActor)
{
    CHECK(InActor != nullptr);
    Actors.Emplace(InActor);

    FMeshComponent* Component = InActor->GetComponentOfType<FMeshComponent>();
    if (Component)
    {
        AddMeshComponent(Component);
    }
}

void FScene::AddLight(FLight* InLight)
{
    CHECK(InLight != nullptr);
    Lights.Emplace(InLight);
}

void FScene::AddLightProbe(FLightProbe* InLightProbe)
{
    CHECK(InLightProbe != nullptr);
    LightProbes.Emplace(InLightProbe);
}

void FScene::OnAddedComponent(FComponent* NewComponent)
{
    FMeshComponent* Component = Cast<FMeshComponent>(NewComponent);
    if (Component && Component->Mesh)
    {
        AddMeshComponent(Component);
    }
}

void FScene::AddMeshComponent(FMeshComponent* Component)
{
    FMeshDrawCommand Command;
    Command.CurrentActor = Component->GetActor();
    Command.Geometry     = Component->Mesh->RTGeometry.Get();
    Command.VertexBuffer = Component->Mesh->VertexBuffer.Get();
    Command.NumVertices  = Component->Mesh->VertexCount;
    Command.IndexBuffer  = Component->Mesh->IndexBuffer.Get();
    Command.NumIndices   = Component->Mesh->IndexCount;
    Command.IndexFormat  = Component->Mesh->IndexFormat;
    Command.Material     = Component->Material.Get();
    Command.Mesh         = Component->Mesh.Get();
    MeshDrawCommands.Push(Command);
}
