#pragma once
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"
#include "RHI/RHITypes.h"

class FRHIResource;

class FRHIValidationStateTracker
{
public:
    FRHIValidationStateTracker()  = default;
    ~FRHIValidationStateTracker() = default;

    void RegisterResource(const FRHIResource* Resource, ERHIResourceState InitialState, ERHIResourceStateTrackingMode TrackingMode);
    void UnregisterResource(const FRHIResource* Resource);
    bool ApplyTransition(const FRHITransitionBarrierDesc& Desc);
    bool ValidateState(const FRHIResource* Resource, ERHIResourceState AcceptedStates, const CHAR* Caller) const;

private:
    struct FResourceState
    {
        ERHIResourceState             CurrentState;
        ERHIResourceStateTrackingMode TrackingMode;
        bool                          bSubresourcesDiverged;
    };

    mutable FCriticalSection                  ResourceStatesCS;
    TMap<const FRHIResource*, FResourceState> ResourceStates;
};
