#include "D3D12RHI/D3D12ResourceState.h"

void FD3D12ResourceState::Initialize(uint32 InNumSubresources)
{
    NumSubresources = InNumSubresources;
    SubresourceStates.Resize(NumSubresources);
}

D3D12_RESOURCE_STATES FD3D12ResourceState::GetSubresourceState(uint32 Subresource) const
{
    CHECK(Subresource < NumSubresources);

    if (bAllSameState || SubresourceStates.Size() == 0)
    {
        return ResourceState;
    }

    return SubresourceStates[Subresource];
}

void FD3D12ResourceState::SetSubresourceState(uint32 Subresource, D3D12_RESOURCE_STATES State)
{
    CHECK(Subresource < NumSubresources);

    SubresourceStates[Subresource] = State;

    bool bAllSame = true;
    for (uint32 i = 0; i < NumSubresources; i++)
    {
        if (SubresourceStates[i] != SubresourceStates[0])
        {
            bAllSame = false;
            break;
        }
    }

    bAllSameState = bAllSame;

    if (bAllSameState)
    {
        ResourceState = SubresourceStates[0];
    }
}

void FD3D12ResourceState::SetResourceState(D3D12_RESOURCE_STATES State)
{
    ResourceState = State;
    
    if (SubresourceStates.Size() > 0)
    {
        for (uint32 i = 0; i < NumSubresources; i++)
        {
            SubresourceStates[i] = State;
        }
    }

    bAllSameState = true;
}

D3D12_RESOURCE_STATES FD3D12ResourceState::GetResourceState() const
{
    CHECK(bAllSameState);
    return ResourceState;
}
