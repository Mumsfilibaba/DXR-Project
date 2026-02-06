#include "Engine/Resources/Material.h"
#include "Engine/Resources/Model.h"
#include "RHI/RHIResources.h"
#include "Engine/World/World.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Engine/World/Components/SkyboxComponent.h"

FWorld::FWorld()
    : Actors()
    , CurrentCamera(nullptr)
    , Scene(nullptr)
{
}

FWorld::~FWorld()
{
    for (FActor* CurrentActor : Actors)
    {
        SAFE_DELETE(CurrentActor);
    }
    
    for (FLight* CurrentLight : Lights)
    {
        SAFE_DELETE(CurrentLight);
    }

    SAFE_DELETE(CurrentCamera);
}

FActor* FWorld::CreateActor()
{
    FActor* NewActor = NewObject<FActor>();
    if (NewActor)
    {
        AddActor(NewActor);
        return NewActor;
    }
    
    return nullptr;
}

void FWorld::Start()
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

void FWorld::Tick(float DeltaTime)
{
    // Update all the actors
    for (FActor* Actor : Actors)
    {
        if (Actor->IsTickable())
        {
            Actor->Tick(DeltaTime);
        }
    }

    // Update the view-proj matrices, at this point we should have a valid view and projection matrix
    if (CurrentCamera)
    {
        // Keep matrices in sync every frame for both rendering and editor gizmos.
        CurrentCamera->UpdateViewMatrix();
        CurrentCamera->UpdateWorldToClipSpaceMatrices();
    }
}

void FWorld::AddCamera(FCamera* InCamera)
{
    if (CurrentCamera)
    {
        SAFE_DELETE(CurrentCamera);
    }

    CurrentCamera = InCamera;

    if (Scene)
    {
        Scene->AddCamera(CurrentCamera);
    }
}

void FWorld::AddActor(FActor* InActor)
{
    if (InActor)
    {
        // Set this scene to be the owner of the added actor
        CHECK(InActor->GetWorld() == nullptr);
        InActor->SetWorld(this);
        Actors.Emplace(InActor);

        if (FPlayerController* PlayerController = Cast<FPlayerController>(InActor))
        {
            AddPlayerController(PlayerController);
        }

        if (FSceneComponent* SceneComponent = InActor->GetComponentOfType<FSceneComponent>())
        {
            AddSceneComponent(SceneComponent);
        }
    }
}

void FWorld::AddPlayerController(FPlayerController* InPlayerController)
{
    if (InPlayerController)
    {
        PlayerControllers.Emplace(InPlayerController);
    }
    else
    {
        DEBUG_BREAK();
    }
}

void FWorld::AddLight(FLight* InLight)
{
    if (InLight)
    {
        Lights.Emplace(InLight);
    }
    else
    {
        DEBUG_BREAK();
    }

    if (Scene)
    {
        Scene->AddLight(InLight);
    }
}

void FWorld::AddLightProbe(FLightProbe* InLightProbe)
{
    if (InLightProbe)
    {
        LightProbes.Emplace(InLightProbe);
    }
    else
    {
        DEBUG_BREAK();
    }

    if (Scene)
    {
        Scene->AddLightProbe(InLightProbe);
    }
}

void FWorld::AddSceneComponent(FSceneComponent* SceneComponent)
{
    if (!Scene)
    {
        return;
    }

    if (SceneComponent)
    {
        if (FStaticMeshComponent* MeshComponent = Cast<FStaticMeshComponent>(SceneComponent))
        {
            Scene->AddStaticMesh(MeshComponent);
        }
        else if (FSkyboxComponent* SkyboxComponent = Cast<FSkyboxComponent>(SceneComponent))
        {
            Scene->AddSkybox(SkyboxComponent);
        }
    }
}

void FWorld::SetSceneInterface(IScene* InScene)
{
    if (InScene)
    {
        Scene = InScene;
    }
    else
    {
        LOG_WARNING("Trying to add a null SceneInterface");
    }
}
