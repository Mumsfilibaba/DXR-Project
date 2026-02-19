#pragma once
#include "Core/Containers/Array.h"
#include "VulkanRHI/VulkanCore.h"

class FVulkanImageState
{
public:
    FVulkanImageState();
    ~FVulkanImageState() = default;

    void Initialize(uint32 InNumSubresources);

    void SetImageLayout(VkImageLayout Layout);
    void SetSubresourceLayout(uint32 Subresource, VkImageLayout Layout);

    VkImageLayout GetImageLayout() const;
    VkImageLayout GetSubresourceLayout(uint32 Subresource) const;

    bool AreAllSubresourcesSameLayout() const
    {
        return bAllSameLayout;
    }

    uint32 GetNumSubresources() const
    {
        return NumSubresources;
    }

private:
    VkImageLayout         ImageLayout;
    TArray<VkImageLayout> SubresourceLayouts;
    uint32                NumSubresources;
    bool                  bAllSameLayout;
};

class FVulkanBufferState
{
public:
    FVulkanBufferState();
    ~FVulkanBufferState() = default;

    void SetState(VkAccessFlags2 InAccess, VkPipelineStageFlags2 InStage);

    VkAccessFlags2 GetAccess() const
    {
        return Access;
    }

    VkPipelineStageFlags2 GetStage() const
    {
        return Stage;
    }

private:
    VkAccessFlags2        Access;
    VkPipelineStageFlags2 Stage;
};
