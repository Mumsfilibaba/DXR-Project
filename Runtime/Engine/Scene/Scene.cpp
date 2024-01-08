#include "Scene.h"
#include "Components/MeshComponent.h"
#include "Engine/Assets/MeshFactory.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Mesh.h"
#include "RHI/RHIResources.h"

FScene::FScene()
    : Actors()
    , RendererScene(nullptr)
{
    RendererScene = new FRendererScene(this);
}

FScene::~FScene()
{
    for (FActor* CurrentActor : Actors)
    {
        SAFE_DELETE(CurrentActor);
    }
    
    for (FLight* CurrentLight : Lights)
    {
        SAFE_DELETE(CurrentLight);
    }

    // TODO: Fix crash on exit
    SAFE_DELETE(CurrentCamera);
    SAFE_DELETE(RendererScene);
}

FActor* FScene::CreateActor()
{
    FActor* NewActor = NewObject<FActor>();
    if (NewActor)
    {
        AddActor(NewActor);
        return NewActor;
    }
    
    return nullptr;
}

void FScene::Start()
{
    // Setup the input components for the PlayerControllers
    for (FPlayerController* PlayerController : PlayerControllers)
    {
        PlayerController->SetupInputComponent();
    }

    // Start all the actors
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
    CHECK(InActor->GetSceneOwner() == nullptr);
    
    // Set this scene to be the owner of the added actor
    InActor->SetSceneOwner(this);
    Actors.Emplace(InActor);

    if (IsSubClassOf<FPlayerController>(InActor))
    {
        PlayerControllers.Emplace(Cast<FPlayerController>(InActor));
    }

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
    Command.CurrentActor = Component->GetActorOwner();
    Command.Geometry     = Component->Mesh->RTGeometry.Get();
    Command.VertexBuffer = Component->Mesh->VertexBuffer.Get();
    Command.NumVertices  = Component->Mesh->VertexCount;
    Command.IndexBuffer  = Component->Mesh->IndexBuffer.Get();
    Command.NumIndices   = Component->Mesh->IndexCount;
    Command.IndexFormat  = Component->Mesh->IndexFormat;
    Command.Material     = Component->Material.Get();
    Command.Mesh         = Component->Mesh.Get();
    MeshDrawCommands.Add(Command);
}

FRendererScene::FRendererScene(FScene* InScene)
    : Scene(InScene)
{
    CHECK(Scene != nullptr);
}

FRendererScene::~FRendererScene()
{
    Scene = nullptr;
}

void FRendererScene::AddPrimitive(FScenePrimitive* InPrimitive)
{
    ScenePrimitives.Add(InPrimitive);
}
