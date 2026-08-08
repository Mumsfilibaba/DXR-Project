#include "Engine/World/World.h"
#include "Engine/World/ActorFilter.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/Components/SceneComponent.h"

FWorld::FWorld()
    : Scene(nullptr)
    , ActiveCamera(nullptr)
    , Actors()
#if EDITOR_BUILD
    , ActorFilters()
    , CurrentFilter(nullptr)
#endif
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

    // Drop all attachment links up-front, so that no actor dereferences an already deleted parent or child
    for (FActor* CurrentActor : Actors)
    {
        CurrentActor->ClearAttachments();
    }

    for (FActor* CurrentActor : Actors)
    {
        CurrentActor->SetWorld(nullptr);
        SAFE_DELETE(CurrentActor);
    }

    Actors.Clear();
    PlayerControllers.Clear();
    ActiveCamera = nullptr;

#if EDITOR_BUILD
    for (FActorFilter* CurrentActorFilter : ActorFilters)
    {
        SAFE_DELETE(CurrentActorFilter);
    }

    ActorFilters.Clear();
    CurrentFilter = nullptr;
#endif
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
    #if EDITOR_BUILD
        InActor->SetFilter(CurrentFilter);
    #endif
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

    // Children outlive their parent, they keep their placement and become root-actors instead
    InActor->DetachAllChildren(EAttachmentRule::KeepWorld);
    InActor->DetachFromParent(EAttachmentRule::KeepWorld);

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

#if EDITOR_BUILD

FActorFilter* FWorld::CreateActorFilter(const String& InName, FActorFilter* InParent)
{
    if (FActorFilter* ExistingFilter = FindActorFilter(InName, InParent))
    {
        return ExistingFilter;
    }

    FActorFilter* NewFilter = new FActorFilter(InName);
    ActorFilters.Emplace(NewFilter);

    // ActorFilters owns every filter regardless of depth, the parent link only decides where it is presented
    if (InParent)
    {
        NewFilter->ParentFilter = InParent;
        InParent->ChildFilters.Emplace(NewFilter);
    }

    return NewFilter;
}

FActorFilter* FWorld::FindActorFilter(const String& InName, FActorFilter* InParent) const
{
    const TArray<FActorFilter*>& SearchScope = InParent ? InParent->ChildFilters : ActorFilters;

    const int32 FilterIndex = SearchScope.FindWithPredicate([&InName, InParent](FActorFilter* Filter)
    {
        // The root scope is the whole array, so filters nested deeper have to be skipped by hand
        if (!Filter || (!InParent && Filter->GetParentFilter()))
        {
            return false;
        }

        return Filter->GetName() == InName;
    });

    return (FilterIndex != SearchScope.InvalidIndex) ? SearchScope[FilterIndex] : nullptr;
}

void FWorld::SetActorFilterParent(FActorFilter* InFilter, FActorFilter* InParent)
{
    if (!InFilter || InFilter == InParent)
    {
        return;
    }

    // Nesting a filter inside its own descendant would cut the subtree loose from the hierarchy
    if (InParent && InParent->IsDescendantOf(InFilter))
    {
        return;
    }

    if (FActorFilter* OldParent = InFilter->ParentFilter)
    {
        OldParent->ChildFilters.Remove(InFilter);
    }

    InFilter->ParentFilter = InParent;

    if (InParent)
    {
        InParent->ChildFilters.Emplace(InFilter);
    }
}

void FWorld::DestroyActorFilter(FActorFilter* InFilter)
{
    if (!InFilter || !ActorFilters.Remove(InFilter))
    {
        return;
    }

    // Nothing inside a destroyed filter is lost, it all moves up one level instead
    FActorFilter* PromotedParent = InFilter->ParentFilter;

    // Unlinked before anything is promoted, so the dying filter cannot show up beside what it used to hold
    if (PromotedParent)
    {
        PromotedParent->ChildFilters.Remove(InFilter);
        InFilter->ParentFilter = nullptr;
    }

    for (FActor* Actor : Actors)
    {
        if (Actor->GetFilter() == InFilter)
        {
            Actor->SetFilter(PromotedParent);
        }
    }

    // Copied since SetActorFilterParent removes from the very array that would be iterated
    const TArray<FActorFilter*> PromotedChildren = InFilter->ChildFilters;
    for (FActorFilter* Child : PromotedChildren)
    {
        SetActorFilterParent(Child, PromotedParent);
    }

    if (CurrentFilter == InFilter)
    {
        CurrentFilter = PromotedParent;
    }

    SAFE_DELETE(InFilter);
}

void FWorld::SetCurrentFilter(FActorFilter* InFilter)
{
    CHECK(!InFilter || ActorFilters.Contains(InFilter));
    CurrentFilter = InFilter;
}

#else

FActorFilter* FWorld::CreateActorFilter(const String& InName, FActorFilter* InParent)
{
    UNREFERENCED_VARIABLE(InName);
    UNREFERENCED_VARIABLE(InParent);
    return nullptr;
}

FActorFilter* FWorld::FindActorFilter(const String& InName, FActorFilter* InParent) const
{
    UNREFERENCED_VARIABLE(InName);
    UNREFERENCED_VARIABLE(InParent);
    return nullptr;
}

void FWorld::SetActorFilterParent(FActorFilter* InFilter, FActorFilter* InParent)
{
    UNREFERENCED_VARIABLE(InFilter);
    UNREFERENCED_VARIABLE(InParent);
}

void FWorld::DestroyActorFilter(FActorFilter* InFilter)
{
    UNREFERENCED_VARIABLE(InFilter);
}

void FWorld::SetCurrentFilter(FActorFilter* InFilter)
{
    // CreateActorFilter never hands one out here, so anything but nullptr came from somewhere that kept a stale pointer
    CHECK(!InFilter);
    UNREFERENCED_VARIABLE(InFilter);
}

#endif

void FWorld::SetActiveCamera(FCameraComponent* InCamera)
{
    if (InCamera)
    {
        CHECK(InCamera->GetActorOwner() != nullptr);
        CHECK(InCamera->GetActorOwner()->GetWorld() == this);
    }

    ActiveCamera = InCamera;
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

}

void FWorld::ClearSceneInterface()
{
    Scene = nullptr;
}
