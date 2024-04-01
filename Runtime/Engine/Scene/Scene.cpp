#include "Scene.h"
#include "Components/MeshComponent.h"
#include "Engine/Assets/MeshFactory.h"
#include "Engine/Resources/Material.h"
#include "Engine/Resources/Mesh.h"
#include "RHI/RHIResources.h"

FScene::FScene()
    : Actors()
    , CurrentCamera(nullptr)
    , RendererScene(nullptr)
{
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

    SAFE_DELETE(CurrentCamera);
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

    if (RendererScene)
    {
        RendererScene->AddCamera(CurrentCamera);
    }
}

void FScene::AddActor(FActor* InActor)
{
    if (InActor)
    {
        // Set this scene to be the owner of the added actor
        CHECK(InActor->GetSceneOwner() == nullptr);
        InActor->SetSceneOwner(this);
        Actors.Emplace(InActor);
    }
    else
    {
        DEBUG_BREAK();
    }

    if (FPlayerController* PlayerController = Cast<FPlayerController>(InActor))
    {
        AddPlayerController(PlayerController);
    }

    if (FRendererComponent* RendererComponent = InActor->GetComponentOfType<FRendererComponent>())
    {
        AddRendererComponent(RendererComponent);
    }
}

void FScene::AddPlayerController(FPlayerController* InPlayerController)
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

void FScene::AddLight(FLight* InLight)
{
    if (InLight)
    {
        Lights.Emplace(InLight);
    }
    else
    {
        DEBUG_BREAK();
    }

    if (RendererScene)
    {
        RendererScene->AddLight(InLight);
    }
}

void FScene::AddLightProbe(FLightProbe* InLightProbe)
{
    if (InLightProbe)
    {
        LightProbes.Emplace(InLightProbe);
    }
    else
    {
        DEBUG_BREAK();
    }
}

void FScene::AddRendererComponent(FRendererComponent* RendererComponent)
{
    if (RendererScene && RendererComponent)
    {
        if (FProxyRendererComponent* ProxyComponent = RendererComponent->CreateProxyComponent())
        {
            RendererScene->AddProxyComponent(ProxyComponent);
        }
    }
}

void FScene::SetRendererScene(IRendererScene* InRendererScene)
{
    if (InRendererScene)
    {
        RendererScene = InRendererScene;
    }
    else
    {
        LOG_WARNING("Trying to add a null RendererScene");
    }
}
