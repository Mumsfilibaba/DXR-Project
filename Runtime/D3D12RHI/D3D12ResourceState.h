#pragma once
#include "Core/Containers/Array.h"
#include "D3D12RHI/D3D12Core.h"

class FD3D12ResourceState
{
public:
    FD3D12ResourceState();
    ~FD3D12ResourceState() = default;

    void Initialize(uint32 InNumSubresources);

    void SetResourceState(D3D12_RESOURCE_STATES State);
    void SetSubresourceState(uint32 Subresource, D3D12_RESOURCE_STATES State);
    
    D3D12_RESOURCE_STATES GetResourceState() const;
    D3D12_RESOURCE_STATES GetSubresourceState(uint32 Subresource) const;

    bool AreAllSubresourcesSameState() const
    {
        return bAllSameState;
    }

    uint32 GetNumSubresources() const
    {
        return NumSubresources;
    }

private:
    D3D12_RESOURCE_STATES         ResourceState;
    TArray<D3D12_RESOURCE_STATES> SubresourceStates;
    uint32                        NumSubresources;
    bool                          bAllSameState;
};
