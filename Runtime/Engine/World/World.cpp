#include "Engine/World/World.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/Components/SceneComponent.h"

FWorld::FWorld()
    : Scene(nullptr)
    , ActiveCamera(nullptr)
    , Actors()
    , PlayerControllers()
    , OnActorRemovedEvent()
{
}

FWorld::~FWorld()
{
    if (Scene)
    {
        for (FActor* CurrentActor : Actors)
        {
            for (FActorComponent* Component : CurrentActor->GetComponents())
            {
                if (FSceneComponent* SceneComponent = Cast<FSceneComponent>(Component))
                {
                    RemoveSceneComponent(SceneComponent);
                }
            }

            Scene->RemoveActorObjectID(CurrentActor);
        }
    }

    for (FActor* CurrentActor : Actors)
    {
        CurrentActor->SetWorld(nullptr);
        SAFE_DELETE(CurrentActor);
    }

    Actors.Clear();
    PlayerControllers.Clear();
    ActiveCamera = nullptr;
}

FActor* FWorld::CreateActor()
{
    return SpawnActor<FActor>();
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
    if (ActiveCamera)
    {
        ActiveCamera->UpdateViewMatrix();
        ActiveCamera->UpdateWorldToClipSpaceMatrices();
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

        for (FActorComponent* Component : InActor->GetComponents())
        {
            if (FSceneComponent* SceneComponent = Cast<FSceneComponent>(Component))
            {
                AddSceneComponent(SceneComponent);
            }
        }
    }
}

void FWorld::RemoveActor(FActor* InActor)
{
    if (!InActor || InActor->GetWorld() != this)
    {
        return;
    }

    const bool bRemovedActiveCamera = ActiveCamera && ActiveCamera->GetActorOwner() == InActor;
    for (FActorComponent* Component : InActor->GetComponents())
    {
        if (FSceneComponent* SceneComponent = Cast<FSceneComponent>(Component))
        {
            RemoveSceneComponent(SceneComponent);
        }
    }

    if (Scene)
    {
        Scene->RemoveActorObjectID(InActor);
    }

    if (FPlayerController* PlayerController = Cast<FPlayerController>(InActor))
    {
        PlayerControllers.Remove(PlayerController);
    }

    Actors.Remove(InActor);
    
    InActor->SetWorld(nullptr);
    
    OnActorRemovedEvent.Broadcast(InActor);
    SAFE_DELETE(InActor);

    if (bRemovedActiveCamera)
    {
        for (FActor* Actor : Actors)
        {
            if (FCameraComponent* CameraComponent = Actor->GetComponentOfType<FCameraComponent>())
            {
                SetActiveCamera(CameraComponent);
                break;
            }
        }
    }
}

void FWorld::SetActiveCamera(FCameraComponent* InCamera)
{
    if (InCamera)
    {
        CHECK(InCamera->GetActorOwner() != nullptr);
        CHECK(InCamera->GetActorOwner()->GetWorld() == this);
    }

    ActiveCamera = InCamera;
    if (Scene)
    {
        Scene->SetActiveCamera(ActiveCamera);
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

void FWorld::AddSceneComponent(FSceneComponent* SceneComponent)
{
    if (!SceneComponent)
    {
        return;
    }

    if (FCameraComponent* CameraComponent = Cast<FCameraComponent>(SceneComponent))
    {
        if (!ActiveCamera)
        {
            SetActiveCamera(CameraComponent);
        }
    }

    if (Scene)
    {
        Scene->AddSceneComponent(SceneComponent);
    }
}

void FWorld::RemoveSceneComponent(FSceneComponent* SceneComponent)
{
    if (!SceneComponent)
    {
        return;
    }

    if (SceneComponent == ActiveCamera)
    {
        SetActiveCamera(nullptr);
    }

    if (Scene)
    {
        Scene->RemoveSceneComponent(SceneComponent);
    }
}

void FWorld::SetSceneInterface(IScene* InScene)
{
    if (!InScene)
    {
        LOG_WARNING("Trying to add a null SceneInterface");
        return;
    }

    Scene = InScene;
    for (FActor* Actor : Actors)
    {
        for (FActorComponent* Component : Actor->GetComponents())
        {
            if (FSceneComponent* SceneComponent = Cast<FSceneComponent>(Component))
            {
                Scene->AddSceneComponent(SceneComponent);
            }
        }
    }

    Scene->SetActiveCamera(ActiveCamera);
}

void FWorld::ClearSceneInterface()
{
    Scene = nullptr;
}
