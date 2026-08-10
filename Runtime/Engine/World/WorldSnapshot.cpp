#include "Engine/World/WorldSnapshot.h"
#include "Engine/World/World.h"
#include "Engine/World/Components/CameraComponent.h"

FWorldSnapshot::FWorldSnapshot()
    : CapturedWorld(nullptr)
    , Entries()
    , ActorRemovedDelegateHandle()
    , ActiveCamera(nullptr)
    , FieldOfView(0.0f)
    , NearPlane(0.0f)
    , FarPlane(0.0f)
{
}

FWorldSnapshot::~FWorldSnapshot()
{
    Reset();
}

void FWorldSnapshot::Capture(FWorld* World)
{
    Reset();

    if (!World)
    {
        return;
    }

    CapturedWorld = World;

    const TArray<FActor*>& Actors = World->GetActors();
    Entries.Reserve(Actors.Size());

    for (FActor* Actor : Actors)
    {
        FActorEntry Entry;
        Entry.Actor     = Actor;
        Entry.Transform = Actor->GetTransform();
        Entries.Emplace(Entry);
    }

    ActiveCamera = World->GetActiveCamera();
    if (ActiveCamera)
    {
        FieldOfView = ActiveCamera->GetFieldOfView();
        NearPlane   = ActiveCamera->GetNearPlane();
        FarPlane    = ActiveCamera->GetFarPlane();
    }

    ActorRemovedDelegateHandle = World->GetOnActorRemovedEvent().AddRaw(this, &FWorldSnapshot::OnActorRemoved);
}

void FWorldSnapshot::Restore(FWorld* World)
{
    if (!World || World != CapturedWorld)
    {
        return;
    }

    for (const FActorEntry& Entry : Entries)
    {
        Entry.Actor->SetTransform(Entry.Transform);
    }

    TArray<FActor*> SpawnedActors;
    for (FActor* Actor : World->GetActors())
    {
        const int32 EntryIndex = Entries.FindWithPredicate([Actor](const FActorEntry& Entry)
        {
            return Entry.Actor == Actor;
        });

        if (EntryIndex == Entries.InvalidIndex)
        {
            SpawnedActors.Emplace(Actor);
        }
    }

    for (FActor* SpawnedActor : SpawnedActors)
    {
        World->RemoveActor(SpawnedActor);
    }

    if (ActiveCamera)
    {
        ActiveCamera->SetFieldOfView(FieldOfView);
        ActiveCamera->SetNearPlane(NearPlane);
        ActiveCamera->SetFarPlane(FarPlane);
    }

    World->SetActiveCamera(ActiveCamera);
}

void FWorldSnapshot::Reset()
{
    if (CapturedWorld && ActorRemovedDelegateHandle.IsValid())
    {
        CapturedWorld->GetOnActorRemovedEvent().Unbind(ActorRemovedDelegateHandle);
    }

    ActorRemovedDelegateHandle.Reset();
    Entries.Clear();

    CapturedWorld = nullptr;
    ActiveCamera  = nullptr;
    FieldOfView   = 0.0f;
    NearPlane     = 0.0f;
    FarPlane      = 0.0f;
}

void FWorldSnapshot::OnActorRemoved(FActor* RemovedActor)
{
    const int32 EntryIndex = Entries.FindWithPredicate([RemovedActor](const FActorEntry& Entry)
    {
        return Entry.Actor == RemovedActor;
    });

    if (EntryIndex != Entries.InvalidIndex)
    {
        Entries.RemoveAt(EntryIndex);
    }

    if (ActiveCamera && ActiveCamera->GetActorOwner() == RemovedActor)
    {
        ActiveCamera = nullptr;
    }
}
