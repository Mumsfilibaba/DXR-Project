#pragma once
#include "VulkanCommandPool.h"
#include "VulkanFence.h"

struct FVulkanImageTransitionBarrier
{
    VkImage                 Image;
    VkImageLayout           PreviousLayout;
    VkImageLayout           NewLayout;
    VkPipelineStageFlags    SrcStageMask;
    VkPipelineStageFlags    DstStageMask;
    VkAccessFlagBits        SrcAccessMask;
    VkAccessFlagBits        DstAccessMask;
    VkDependencyFlags       DependencyFlags;
    VkImageSubresourceRange SubresourceRange;
};

class FVulkanCommandBuffer : public FVulkanDeviceObject
{
public:
    FVulkanCommandBuffer(FVulkanDevice* InDevice, EVulkanCommandQueueType InType);
    FVulkanCommandBuffer(FVulkanCommandBuffer&& Other);
    ~FVulkanCommandBuffer();

    bool Initialize(VkCommandBufferLevel InLevel);

    FORCEINLINE bool Begin(VkCommandBufferUsageFlags Flags = 0)
    {
        // Wait for GPU to finish with this CommandBuffer and then reset it
        if (!WaitForFence())
        {
            return false;
        }

        if (!Fence.Reset())
        {
            return false;
        }

        // Avoid using the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT since we can reuse the memory
        if (!CommandPool.Reset())
        {
            return false;
        }

        VkCommandBufferBeginInfo BeginInfo;
        FMemory::Memzero(&BeginInfo);

        BeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        BeginInfo.pNext            = nullptr;
        BeginInfo.flags            = Flags;
        BeginInfo.pInheritanceInfo = nullptr;

        VkResult Result = vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
        VULKAN_CHECK_RESULT(Result, "vkBeginCommandBuffer Failed");

        bIsRecording = true;
        return true;
    }

    FORCEINLINE bool End()
    {
        VULKAN_CHECK_RESULT(vkEndCommandBuffer(CommandBuffer), "vkEndCommandBuffer Failed");
        bIsRecording = false;
        return true;
    }

    FORCEINLINE void ClearColorImage(VkImage Image, VkImageLayout ImageLayout, VkClearColorValue* ClearColor, uint32 RangeCount, VkImageSubresourceRange* Ranges)
    {
        vkCmdClearColorImage(CommandBuffer, Image, ImageLayout, ClearColor, RangeCount, Ranges);
    }

    FORCEINLINE void ClearDepthStencilImage(VkImage Image, VkImageLayout ImageLayout, const VkClearDepthStencilValue* DepthStencil, uint32 RangeCount, VkImageSubresourceRange* Ranges)
    {
        vkCmdClearDepthStencilImage(CommandBuffer, Image, ImageLayout, DepthStencil, RangeCount, Ranges);
    }

    FORCEINLINE void BeginRenderPass(const VkRenderPassBeginInfo* RenderPassBegin, VkSubpassContents SubpassContents)
    {
        vkCmdBeginRenderPass(CommandBuffer, RenderPassBegin, SubpassContents);
    }

    FORCEINLINE void EndRenderPass()
    {
        vkCmdEndRenderPass(CommandBuffer);
    }

    FORCEINLINE void ImageLayoutTransitionBarrier(const FVulkanImageTransitionBarrier& TransitionBarrier)
    {
        VkImageMemoryBarrier ImageMemoryBarrier;
        FMemory::Memzero(&ImageMemoryBarrier);

        ImageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ImageMemoryBarrier.pNext               = nullptr;
        ImageMemoryBarrier.srcAccessMask       = TransitionBarrier.SrcAccessMask;
        ImageMemoryBarrier.dstAccessMask       = TransitionBarrier.DstAccessMask;
        ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        ImageMemoryBarrier.subresourceRange    = TransitionBarrier.SubresourceRange;
        ImageMemoryBarrier.image               = TransitionBarrier.Image;
        ImageMemoryBarrier.newLayout           = TransitionBarrier.NewLayout;
        ImageMemoryBarrier.oldLayout           = TransitionBarrier.PreviousLayout;

        ImageMemoryPipelineBarrier(TransitionBarrier.SrcStageMask, TransitionBarrier.DstStageMask, TransitionBarrier.DependencyFlags, 1, &ImageMemoryBarrier);
    }

    FORCEINLINE void ImageMemoryPipelineBarrier(VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags, uint32 ImageMemoryBarrierCount, const VkImageMemoryBarrier* ImageMemoryBarriers)
    {
        vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, 0, nullptr, 0, nullptr, ImageMemoryBarrierCount, ImageMemoryBarriers);
    }

    FORCEINLINE bool WaitForFence(uint64 TimeOut = UINT64_MAX)
    {
        return Fence.Wait(TimeOut);
    }

    FORCEINLINE FVulkanCommandPool* GetCommandPool()
    {
        return &CommandPool;
    }

    FORCEINLINE FVulkanFence* GetFence()
    {
        return &Fence;
    }

    FORCEINLINE VkCommandBuffer GetVkCommandBuffer() const
    {
        return CommandBuffer;
    }
    
    FORCEINLINE bool IsRecording() const
    {
        return bIsRecording;
    }

private:
    FVulkanCommandPool   CommandPool;
    FVulkanFence         Fence;

    VkCommandBufferLevel Level;
    VkCommandBuffer      CommandBuffer;

    bool                 bIsRecording;
};
