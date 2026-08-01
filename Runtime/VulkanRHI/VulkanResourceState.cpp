#include "VulkanRHI/VulkanResourceState.h"

void FVulkanImageLayoutState::Initialize(uint32 InNumSubresources)
{
    NumSubresources = InNumSubresources;
    SubresourceLayouts.Resize(NumSubresources);
}

VkImageLayout FVulkanImageLayoutState::GetSubresourceLayout(uint32 Subresource) const
{
    CHECK(Subresource < NumSubresources);

    if (bAllSameLayout || SubresourceLayouts.Size() == 0)
    {
        return ImageLayout;
    }

    return SubresourceLayouts[Subresource];
}

void FVulkanImageLayoutState::SetSubresourceLayout(uint32 Subresource, VkImageLayout Layout)
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

void FVulkanImageLayoutState::SetImageLayout(VkImageLayout Layout)
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

VkImageLayout FVulkanImageLayoutState::GetImageLayout() const
{
    CHECK(bAllSameLayout);
    return ImageLayout;
}

void FVulkanBufferState::SetState(VkAccessFlags2KHR InAccess, VkPipelineStageFlags2KHR InStage)
{
    Access = InAccess;
    Stage  = InStage;
}
