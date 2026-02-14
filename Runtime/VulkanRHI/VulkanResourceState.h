#pragma once
#include "Core/Containers/Array.h"
#include "VulkanRHI/VulkanCore.h"

class FVulkanImageLayoutState
{
public:
    struct FImageState
    {
        VkImageLayout         Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkAccessFlags2        Access = VK_ACCESS_2_NONE;
        VkPipelineStageFlags2 Stage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    };

    FVulkanImageLayoutState()
        : State()
        , NumMipLevels(0)
        , NumArraySlices(0)
    {
    }

    FVulkanImageLayoutState(const FImageState& InitialState, uint32 InNumMipLevels, uint32 InNumArraySlices)
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

    const FImageState& GetState() const
    {
        return State;
    }

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
        if (!IsSubresourceTrackingEnabled())
        {
            return;
        }

        CHECK(MipLevel < NumMipLevels);
        CHECK(ArraySlice < NumArraySlices);

        const uint32 Index = GetSubresourceIndex(MipLevel, ArraySlice);
        SubresourceStates[Index] = NewState;
    }

    const FImageState& GetSubresourceState(uint32 MipLevel, uint32 ArraySlice) const
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

    FImageState         State;
    uint32              NumMipLevels;
    uint32              NumArraySlices;
    TArray<FImageState> SubresourceStates;
};

class FVulkanBufferState
{
public:
    FVulkanBufferState(VkAccessFlags2 InAccess, VkPipelineStageFlags2 InStage)
        : Access(InAccess)
        , Stage(InStage)
    {
    }

    VkAccessFlags2 GetAccess() const
    {
        return Access;
    }

    VkPipelineStageFlags2 GetStage() const
    {
        return Stage;
    }

    void SetState(VkAccessFlags2 InAccess, VkPipelineStageFlags2 InStage)
    {
        Access = InAccess;
        Stage  = InStage;
    }

private:
    VkAccessFlags2        Access = VK_ACCESS_2_NONE;
    VkPipelineStageFlags2 Stage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
};
