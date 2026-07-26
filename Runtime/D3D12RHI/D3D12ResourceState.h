#pragma once
#include "Core/Containers/Array.h"
#include "D3D12RHI/D3D12Core.h"

class FD3D12Resource;

constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_TO_BE_DETERMINED = static_cast<D3D12_RESOURCE_STATES>(0xFFFFFFFF);

inline constexpr D3D12_RESOURCE_STATES GD3D12CombinableReadStates = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

FORCEINLINE bool D3D12IsReadOnlyState(D3D12_RESOURCE_STATES State)
{
    return State != D3D12_RESOURCE_STATES(0) && (State & ~GD3D12CombinableReadStates) == D3D12_RESOURCE_STATES(0);
}

FORCEINLINE bool D3D12IsReadStateSatisfied(D3D12_RESOURCE_STATES CurrentState, D3D12_RESOURCE_STATES AfterState)
{
    return D3D12IsReadOnlyState(AfterState) && (CurrentState & AfterState) == AfterState;
}

FORCEINLINE bool D3D12IsBeforeStateValid(D3D12_RESOURCE_STATES CurrentState, D3D12_RESOURCE_STATES BeforeState)
{
    return D3D12IsReadOnlyState(BeforeState) ? ((CurrentState & BeforeState) == BeforeState) : (CurrentState == BeforeState);
}

struct FD3D12PendingBarrier
{
	FD3D12Resource*       Resource;
	D3D12_RESOURCE_STATES DesiredState;
	uint32                Subresource;
};

class FD3D12ResourceState
{
public:
    void Initialize(uint32 InNumSubresources);

    void SetState(D3D12_RESOURCE_STATES State);
    void SetSubresourceState(uint32 Subresource, D3D12_RESOURCE_STATES State);

    void ApplyResolvedStates(const FD3D12ResourceState& Other);
    
    D3D12_RESOURCE_STATES GetState() const;
    D3D12_RESOURCE_STATES GetSubresourceState(uint32 Subresource) const;

    bool IsInitialized() const
    {
        return NumSubresources > 0;
    }

    bool IsSingleState() const
    {
        return bAllSameState;
    }

    uint32 GetNumSubresources() const
    {
        return NumSubresources;
    }

private:
    TArray<D3D12_RESOURCE_STATES> SubresourceStates = {};
    D3D12_RESOURCE_STATES         ResourceState     = D3D12_RESOURCE_STATE_COMMON;
    uint32                        NumSubresources   = 0;
    bool                          bAllSameState     = true;
};
