#pragma once
#include "Core/Containers/Array.h"
#include "D3D12RHI/D3D12Core.h"

class FD3D12ResourceState
{
public:
    FD3D12ResourceState() = default;

    void Enable(D3D12_RESOURCE_STATES InitialState, uint32 MipCount, uint32 ArrayCount)
    {
        bEnabled = true;
        State = InitialState;
        SubresourceMipCount = MipCount;
        SubresourceArrayCount = ArrayCount;

        const uint32 SubresourceCount = MipCount * ArrayCount;
        if (SubresourceCount > 0)
        {
            SubresourceStates.Resize(SubresourceCount);
            for (uint32 Index = 0; Index < SubresourceCount; ++Index)
            {
                SubresourceStates[Index] = InitialState;
            }
        }
    }

    void Disable()
    {
        bEnabled = false;
        SubresourceStates.Clear();
        SubresourceMipCount = 0;
        SubresourceArrayCount = 0;
    }

    bool IsEnabled() const { return bEnabled; }
    D3D12_RESOURCE_STATES GetState() const { return State; }

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
        if (!bEnabled || !IsSubresourceTrackingEnabled())
        {
            return;
        }

        CHECK(MipLevel < SubresourceMipCount);
        CHECK(ArraySlice < SubresourceArrayCount);

        const uint32 Index = GetSubresourceIndex(MipLevel, ArraySlice);
        SubresourceStates[Index] = NewState;
    }

    D3D12_RESOURCE_STATES GetSubresourceState(uint32 MipLevel, uint32 ArraySlice) const
    {
        CHECK(MipLevel < SubresourceMipCount);
        CHECK(ArraySlice < SubresourceArrayCount);

        const uint32 Index = GetSubresourceIndex(MipLevel, ArraySlice);
        return SubresourceStates[Index];
    }

    bool IsSubresourceTrackingEnabled() const
    {
        return bEnabled && SubresourceStates.Size() > 0;
    }

    uint32 GetSubresourceMipCount() const { return SubresourceMipCount; }
    uint32 GetSubresourceArrayCount() const { return SubresourceArrayCount; }

private:
    uint32 GetSubresourceIndex(uint32 MipLevel, uint32 ArraySlice) const
    {
        return MipLevel + ArraySlice * SubresourceMipCount;
    }

    bool bEnabled = false;
    D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATE_COMMON;

    uint32 SubresourceMipCount = 0;
    uint32 SubresourceArrayCount = 0;
    TArray<D3D12_RESOURCE_STATES> SubresourceStates;
};
