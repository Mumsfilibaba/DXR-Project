#include "Core/Threading/ScopedLock.h"
#include "RHI/ValidationLayer/RHIValidationStateTracker.h"
#include "RHI/ValidationLayer/RHIValidationInternal.h"

void FRHIValidationStateTracker::RegisterResource(const FRHIResource* Resource, ERHIResourceState InitialState, ERHIResourceStateTrackingMode TrackingMode)
{
    if (!Resource)
    {
        return;
    }

    TScopedLock Lock(ResourceStatesCS);

    FResourceState& State       = ResourceStates.Add(Resource);
    State.CurrentState          = InitialState;
    State.TrackingMode          = TrackingMode;
    State.bSubresourcesDiverged = false;
}

void FRHIValidationStateTracker::UnregisterResource(const FRHIResource* Resource)
{
    if (!Resource)
    {
        return;
    }

    TScopedLock Lock(ResourceStatesCS);
    (void)ResourceStates.RemoveKey(Resource);
}

bool FRHIValidationStateTracker::ApplyTransition(const FRHITransitionBarrierDesc& Desc)
{
    if (!RHIValidationInternal::ShouldValidateResourceStates())
    {
        return true;
    }

    const bool bIsWholeResource = Desc.IsTexture() ? Desc.Texture.Subresources.IsAllSubresources() : Desc.Buffer.Range.IsWholeResource();

    const FRHIResource* Resource = Desc.IsTexture() ? static_cast<const FRHIResource*>(Desc.Texture.Resource) : static_cast<const FRHIResource*>(Desc.Buffer.Resource);

    TScopedLock Lock(ResourceStatesCS);

    FResourceState* State = ResourceStates.Find(Resource);
    if (!State)
    {
        return true;
    }

    if (State->TrackingMode == ERHIResourceStateTrackingMode::Static)
    {
        if (Desc.IsTrackingModeChange())
        {
            State->TrackingMode = Desc.NewTrackingMode;
            State->CurrentState = Desc.AfterState;
        }

        return true;
    }

    if (!bIsWholeResource)
    {
        State->bSubresourcesDiverged = true;
        return true;
    }

    const bool bDeclaresBeforeState = State->TrackingMode == ERHIResourceStateTrackingMode::Manual;
    if (bDeclaresBeforeState && !State->bSubresourcesDiverged && !RHIIsBeforeStateValid(State->CurrentState, Desc.BeforeState))
    {
        RHI_VALIDATION_ERROR("%s: TransitionBarrier declares a before-state of %s (0x%X) but the resource is in %s (0x%X).",
            *RHIValidationInternal::GetResourceIdentity(Resource), ToString(Desc.BeforeState), uint32(Desc.BeforeState),
            ToString(State->CurrentState), uint32(State->CurrentState));
        return false;
    }

    State->bSubresourcesDiverged = false;
    State->CurrentState          = Desc.AfterState;

    if (Desc.IsTrackingModeChange())
    {
        State->TrackingMode = Desc.NewTrackingMode;
    }

    return true;
}

bool FRHIValidationStateTracker::ValidateState(const FRHIResource* Resource, ERHIResourceState AcceptedStates, const CHAR* Caller) const
{
    if (!RHIValidationInternal::ShouldValidateResourceStates() || !Resource)
    {
        return true;
    }

    TScopedLock Lock(ResourceStatesCS);

    const FResourceState* State = ResourceStates.Find(Resource);
    if (!State)
    {
        return true;
    }

    if (State->TrackingMode == ERHIResourceStateTrackingMode::Static || State->bSubresourcesDiverged)
    {
        return true;
    }

    if (State->CurrentState == ERHIResourceState::Common)
    {
        return true;
    }

    if ((State->CurrentState & AcceptedStates) == ERHIResourceState::Common)
    {
        RHI_VALIDATION_ERROR("%s: %s requires the resource to be in %s (0x%X) but it is in %s. A barrier is missing.",
            *RHIValidationInternal::GetResourceIdentity(Resource), Caller, ToString(AcceptedStates), uint32(AcceptedStates),
            ToString(State->CurrentState));
        return false;
    }

    return true;
}
