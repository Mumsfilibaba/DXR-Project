#pragma once
#include "Core/Containers/Array.h"
#include "Core/Delegates/DelegateInstance.h"
#include "Engine/World/Actors/Actor.h"

class FCameraComponent;
class FWorld;

class ENGINE_API FWorldSnapshot
{
public:
    FWorldSnapshot();
    ~FWorldSnapshot();

    /**
     * @brief Record the current state of the world
     *
     * @param World World to record, recording replaces anything held from an earlier call
     */
    void Capture(FWorld* World);

    /**
     * @brief Put the world back the way it was when it was recorded
     *
     * Actors that appeared after the recording are destroyed, since only a run could have created them.
     *
     * @param World World to restore, which has to be the one that was recorded
     */
    void Restore(FWorld* World);

    /**
     * @brief Drop everything held, releasing the world it was watching
     */
    void Reset();

    /**
     * @return Returns true if a world has been recorded and not yet dropped
     */
    bool IsValid() const
    {
        return CapturedWorld != nullptr;
    }

private:
    struct FActorEntry
    {
        FActor*         Actor;
        FActorTransform Transform;
    };

    void OnActorRemoved(FActor* RemovedActor);

    FWorld*             CapturedWorld;
    TArray<FActorEntry> Entries;
    FDelegateHandle     ActorRemovedDelegateHandle;
    FCameraComponent*   ActiveCamera;
    float               FieldOfView;
    float               NearPlane;
    float               FarPlane;
};
