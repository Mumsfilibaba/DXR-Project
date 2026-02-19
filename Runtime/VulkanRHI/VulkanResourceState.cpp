#include "VulkanRHI/VulkanResourceState.h"

FVulkanImageState::FVulkanImageState()
    : ImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    , NumSubresources(0)
    , bAllSameLayout(true)
{
}

void FVulkanImageState::Initialize(uint32 InNumSubresources)
{
    NumSubresources = InNumSubresources;
    SubresourceLayouts.Resize(NumSubresources);
}

VkImageLayout FVulkanImageState::GetSubresourceLayout(uint32 Subresource) const
{
    CHECK(Subresource < NumSubresources);

    if (bAllSameLayout || SubresourceLayouts.Size() == 0)
    {
        return ImageLayout;
    }

    return SubresourceLayouts[Subresource];
}

void FVulkanImageState::SetSubresourceLayout(uint32 Subresource, VkImageLayout Layout)
{
    CHECK(Subresource < NumSubresources);

    SubresourceLayouts[Subresource] = Layout;

    bool bAllSame = true;
    for (uint32 i = 0; i < NumSubresources; i++)
    {
        if (SubresourceLayouts[i] != SubresourceLayouts[0])
        {
            bAllSame = false;
            break;
        }
    }

    bAllSameLayout = bAllSame;

    if (bAllSameLayout)
    {
        ImageLayout = SubresourceLayouts[0];
    }
}

void FVulkanImageState::SetImageLayout(VkImageLayout Layout)
{
    ImageLayout = Layout;

    if (SubresourceLayouts.Size() > 0)
    {
        for (uint32 i = 0; i < NumSubresources; i++)
        {
            SubresourceLayouts[i] = Layout;
        }
    }

    bAllSameLayout = true;
}

VkImageLayout FVulkanImageState::GetImageLayout() const
{
    CHECK(bAllSameLayout);
    return ImageLayout;
}

FVulkanBufferState::FVulkanBufferState()
    : Access(0)
    , Stage(0)
{
}

void FVulkanBufferState::SetState(VkAccessFlags2 InAccess, VkPipelineStageFlags2 InStage)
{
    Access = InAccess;
    Stage  = InStage;
}
