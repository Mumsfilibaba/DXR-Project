#pragma once
#include "Core/Containers/Array.h"
#include "VulkanRHI/VulkanCore.h"

constexpr VkImageLayout             VK_IMAGE_LAYOUT_TO_BE_DETERMINED           = static_cast<VkImageLayout>(0x7FFFFFFF);
constexpr VkAccessFlags2KHR         VK_ACCESS_FLAGS_2_TO_BE_DETERMINED         = ~static_cast<VkAccessFlags2KHR>(0);
constexpr VkPipelineStageFlags2KHR  VK_PIPELINE_STAGE_FLAGS_2_TO_BE_DETERMINED = ~static_cast<VkPipelineStageFlags2KHR>(0);

class FVulkanTextureRHI;
class FVulkanBufferRHI;

class FVulkanImageLayoutState
{
public:
    void Initialize(uint32 InNumSubresources);

    void SetImageLayout(VkImageLayout Layout);
    void SetSubresourceLayout(uint32 Subresource, VkImageLayout Layout);

    VkImageLayout GetImageLayout() const;
    VkImageLayout GetSubresourceLayout(uint32 Subresource) const;

    void AdoptTrackedState(const FVulkanImageLayoutState& Other);

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

    void SetDefaultLayout(VkImageLayout InDefaultLayout)
    {
        DefaultLayout    = InDefaultLayout;
        bHasDefaultLayout = true;
    }

    void ClearDefaultLayout()
    {
        DefaultLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        bHasDefaultLayout = false;
    }

    bool HasDefaultLayout() const
    {
        return bHasDefaultLayout;
    }

    VkImageLayout GetDefaultLayout() const
    {
        return DefaultLayout;
    }

private:
    VkImageLayout         ImageLayout       = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout         DefaultLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32                NumSubresources   = 0;
    bool                  bAllSameLayout    = true;
    bool                  bHasDefaultLayout = false;
    TArray<VkImageLayout> SubresourceLayouts;
};

class FVulkanBufferState
{
public:
    void SetState(VkAccessFlags2KHR InAccess, VkPipelineStageFlags2KHR InStage);

    VkAccessFlags2KHR GetAccess() const
    {
        return Access;
    }

    VkPipelineStageFlags2KHR GetStage() const
    {
        return Stage;
    }

    void SetDefaultState(VkAccessFlags2KHR InDefaultAccess, VkPipelineStageFlags2KHR InDefaultStage)
    {
        DefaultAccess    = InDefaultAccess;
        DefaultStage     = InDefaultStage;
        bHasDefaultState = true;
    }

    bool HasDefaultState() const
    {
        return bHasDefaultState;
    }

    VkAccessFlags2KHR GetDefaultAccess() const
    {
        return DefaultAccess;
    }

    void AdoptTrackedState(const FVulkanBufferState& Other)
    {
        const VkAccessFlags2KHR        PreservedAccess      = DefaultAccess;
        const VkPipelineStageFlags2KHR PreservedStage       = DefaultStage;
        const bool                     bPreservedHasDefault = bHasDefaultState;

        *this = Other;

        DefaultAccess    = PreservedAccess;
        DefaultStage     = PreservedStage;
        bHasDefaultState = bPreservedHasDefault;
    }

private:
    VkAccessFlags2KHR        Access           = 0;
    VkPipelineStageFlags2KHR Stage            = 0;
    VkAccessFlags2KHR        DefaultAccess    = 0;
    VkPipelineStageFlags2KHR DefaultStage     = 0;
    bool                     bHasDefaultState = false;
};

struct FVulkanPendingImageBarrier
{
    FVulkanTextureRHI* Texture;
    VkImageLayout      DesiredLayout;
    uint32             Subresource;
};

struct FVulkanPendingBufferBarrier
{
    FVulkanBufferRHI*        Buffer;
    VkAccessFlags2KHR        DesiredAccess;
    VkPipelineStageFlags2KHR DesiredStage;
};
