#pragma once
#include "Core/Containers/Array.h"
#include "VulkanRHI/VulkanCore.h"

class FVulkanImageLayoutState
{
public:
    FVulkanImageLayoutState() = default;

    struct FImageState
    {
        VkImageLayout         Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkAccessFlags2        Access = VK_ACCESS_2_NONE;
        VkPipelineStageFlags2 Stage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    };

    void Enable(const FImageState& InitialState, uint32 MipCount, uint32 ArrayCount)
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
    const FImageState& GetState() const { return State; }

    void SetState(const FImageState& NewState)
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

    void UpdateSubresourceState(const FImageState& NewState, uint32 MipLevel, uint32 ArraySlice)
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

    const FImageState& GetSubresourceState(uint32 MipLevel, uint32 ArraySlice) const
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
    FImageState State = {};

    uint32 SubresourceMipCount = 0;
    uint32 SubresourceArrayCount = 0;
    TArray<FImageState> SubresourceStates;
};

class FVulkanBufferState
{
public:
    FVulkanBufferState() = default;

    struct FBufferState
    {
        VkAccessFlags2        Access = VK_ACCESS_2_NONE;
        VkPipelineStageFlags2 Stage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    };

    void Enable(const FBufferState& InitialState)
    {
        bEnabled = true;
        State = InitialState;
    }

    void Disable()
    {
        bEnabled = false;
    }

    bool IsEnabled() const { return bEnabled; }
    const FBufferState& GetState() const { return State; }

    void SetState(const FBufferState& NewState)
    {
        State = NewState;
    }

private:
    bool bEnabled = false;
    FBufferState State = {};
};
