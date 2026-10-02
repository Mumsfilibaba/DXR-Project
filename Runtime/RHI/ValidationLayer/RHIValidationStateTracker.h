#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/Set.h"
#include "Core/Platform/CriticalSection.h"
#include "RHI/RHITypes.h"

class FRHIResource;

class FRHIValidationStateTracker
{
public:
    FRHIValidationStateTracker();
    ~FRHIValidationStateTracker();

    void RegisterResource(const FRHIResource* Resource, ERHIResourceState InitialState, ERHIResourceStateTrackingMode TrackingMode);
    void UnregisterResource(const FRHIResource* Resource);
    void SetResourceState(const FRHIResource* Resource, ERHIResourceState NewState);

    bool ApplyTransition(const FRHITransitionBarrierDesc& Desc);
    bool ValidateState(const FRHIResource* Resource, ERHIResourceState AcceptedStates, const CHAR* Caller) const;

    void MarkAccelerationStructureWritten(const FRHIResource* AccelerationStructure, bool bIsScene);
    void ClearAccelerationStructureWrite(const FRHIResource* AccelerationStructure);
    bool ConsumeAccelerationStructureWrite(const FRHIResource* AccelerationStructure);
    const FRHIResource* ConsumeAnySceneAccelerationStructureWrite();

private:
    struct FResourceState
    {
        ERHIResourceState             CurrentState;
        ERHIResourceStateTrackingMode TrackingMode;
        bool                          bSubresourcesDiverged;
    };

    mutable FCriticalSection                  ResourceStatesCS;
    TMap<const FRHIResource*, FResourceState> ResourceStates;
    FCriticalSection                          AccelerationStructureWritesCS;
    TSet<const FRHIResource*>                 PendingAccelerationStructureWrites;
    TSet<const FRHIResource*>                 PendingSceneAccelerationStructureWrites;
};
