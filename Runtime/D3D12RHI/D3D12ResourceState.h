#pragma once
#include "Core/Containers/Array.h"
#include "D3D12RHI/D3D12Core.h"

class FD3D12Resource;

constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_TO_BE_DETERMINED = static_cast<D3D12_RESOURCE_STATES>(0xFFFFFFFF);

class FD3D12ResourceState
{
public:
    void Initialize(uint32 InNumSubresources);

    void SetResourceState(D3D12_RESOURCE_STATES State);
    void SetSubresourceState(uint32 Subresource, D3D12_RESOURCE_STATES State);
    
    D3D12_RESOURCE_STATES GetResourceState() const;
    D3D12_RESOURCE_STATES GetSubresourceState(uint32 Subresource) const;

    bool IsInitialized() const
    {
        return NumSubresources > 0;
    }

    bool AreAllSubresourcesSameState() const
    {
        return bAllSameState;
    }

    uint32 GetNumSubresources() const
    {
        return NumSubresources;
    }

private:
    D3D12_RESOURCE_STATES         ResourceState    = D3D12_RESOURCE_STATE_COMMON;
    TArray<D3D12_RESOURCE_STATES> SubresourceStates;
    uint32                        NumSubresources  = 0;
    bool                          bAllSameState    = true;
};

struct FD3D12PendingBarrier
{
    FD3D12Resource*       Resource;
    D3D12_RESOURCE_STATES DesiredState;
    uint32                Subresource;
};
