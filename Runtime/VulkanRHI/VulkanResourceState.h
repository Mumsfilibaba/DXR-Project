#pragma once
#include "Core/Containers/Array.h"
#include "VulkanRHI/VulkanCore.h"

constexpr VkImageLayout          VK_IMAGE_LAYOUT_TO_BE_DETERMINED           = static_cast<VkImageLayout>(0x7FFFFFFF);
constexpr VkAccessFlags2         VK_ACCESS_FLAGS_2_TO_BE_DETERMINED         = ~static_cast<VkAccessFlags2>(0);
constexpr VkPipelineStageFlags2  VK_PIPELINE_STAGE_FLAGS_2_TO_BE_DETERMINED = ~static_cast<VkPipelineStageFlags2>(0);

class FVulkanTexture;
class FVulkanBuffer;

class FVulkanImageLayoutState
{
public:
    void Initialize(uint32 InNumSubresources);

    void SetImageLayout(VkImageLayout Layout);
    void SetSubresourceLayout(uint32 Subresource, VkImageLayout Layout);

    VkImageLayout GetImageLayout() const;
    VkImageLayout GetSubresourceLayout(uint32 Subresource) const;

    bool IsInitialized() const
    {
        return NumSubresources > 0;
    }

    bool AreAllSubresourcesSameLayout() const
    {
        return bAllSameLayout;
    }

    uint32 GetNumSubresources() const
    {
        return NumSubresources;
    }

private:
    VkImageLayout         ImageLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    TArray<VkImageLayout> SubresourceLayouts;
    uint32                NumSubresources = 0;
    bool                  bAllSameLayout  = true;
};

class FVulkanBufferState
{
public:
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
    VkAccessFlags2        Access = 0;
    VkPipelineStageFlags2 Stage  = 0;
};

struct FVulkanPendingImageBarrier
{
    FVulkanTexture* Texture;
    VkImageLayout   DesiredLayout;
    uint32          Subresource;
};

struct FVulkanPendingBufferBarrier
{
    FVulkanBuffer*        Buffer;
    VkAccessFlags2        DesiredAccess;
    VkPipelineStageFlags2 DesiredStage;
};
