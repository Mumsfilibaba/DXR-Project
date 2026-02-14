#pragma once
#include "Core/Containers/Array.h"
#include "D3D12RHI/D3D12Core.h"

class FD3D12ResourceState
{
public:
    FD3D12ResourceState()
        : State(D3D12_RESOURCE_STATE_COMMON)
        , NumMipLevels(0)
        , NumArraySlices(0)
    {
    }

    FD3D12ResourceState(D3D12_RESOURCE_STATES InitialState, uint32 InNumMipLevels, uint32 InNumArraySlices)
        : State(InitialState)
        , NumMipLevels(InNumMipLevels)
        , NumArraySlices(InNumArraySlices)
    {
        const uint32 SubresourceCount = InNumMipLevels * InNumArraySlices;
        if (SubresourceCount > 0)
        {
            SubresourceStates.Resize(SubresourceCount);
            for (uint32 Index = 0; Index < SubresourceCount; ++Index)
            {
                SubresourceStates[Index] = InitialState;
            }
        }
    }

    D3D12_RESOURCE_STATES GetState() const
    {
        return State;
    }

    void SetState(D3D12_RESOURCE_STATES NewState)
    {
        State = NewState;

        if (IsSubresourceTrackingEnabled())
        {
            const uint32 SubresourceCount = SubresourceStates.Size();
            for (uint32 Index = 0; Index < SubresourceCount; ++Index)
            {
                SubresourceStates[Index] = NewState;
            }
        }
    }

    void UpdateSubresourceState(D3D12_RESOURCE_STATES NewState, uint32 MipLevel, uint32 ArraySlice)
    {
        if (!IsSubresourceTrackingEnabled())
        {
            return;
        }

        CHECK(MipLevel < NumMipLevels);
        CHECK(ArraySlice < NumArraySlices);

        const uint32 Index = GetSubresourceIndex(MipLevel, ArraySlice);
        SubresourceStates[Index] = NewState;
    }

    D3D12_RESOURCE_STATES GetSubresourceState(uint32 MipLevel, uint32 ArraySlice) const
    {
        CHECK(MipLevel < NumMipLevels);
        CHECK(ArraySlice < NumArraySlices);

        const uint32 Index = GetSubresourceIndex(MipLevel, ArraySlice);
        return SubresourceStates[Index];
    }

    bool IsSubresourceTrackingEnabled() const
    {
        return SubresourceStates.Size() > 0;
    }

    uint32 GetSubresourceMipCount() const
    {
        return NumMipLevels;
    }

    uint32 GetSubresourceArrayCount() const
    {
        return NumArraySlices;
    }

private:
    uint32 GetSubresourceIndex(uint32 MipLevel, uint32 ArraySlice) const
    {
        return MipLevel + ArraySlice * NumMipLevels;
    }

    D3D12_RESOURCE_STATES         State;
    uint32                        NumMipLevels;
    uint32                        NumArraySlices;
    TArray<D3D12_RESOURCE_STATES> SubresourceStates;
};
