#pragma once
#include "VulkanCommandPool.h"
#include "VulkanFence.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVulkanPiplineBarrier

struct SVulkanPiplineBarrier
{

};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanCommandBuffer

class CVulkanCommandBuffer : public CVulkanDeviceObject
{
public:
    CVulkanCommandBuffer(CVulkanDevice* InDevice, EVulkanCommandQueueType InType);
    CVulkanCommandBuffer(CVulkanCommandBuffer&& Other);
    ~CVulkanCommandBuffer();

    bool Initialize(VkCommandBufferLevel InLevel);

    FORCEINLINE bool Begin(VkCommandBufferUsageFlags Flags = 0)
    {
        // Wait for GPU to finish with this CommandBuffer and then reset it
        if (!Fence.Wait(UINT64_MAX))
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
        CMemory::Memzero(&BeginInfo);

		BeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.pNext            = nullptr;
		BeginInfo.flags            = Flags;
		BeginInfo.pInheritanceInfo = nullptr;

		VkResult Result = vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
        VULKAN_CHECK_RESULT(Result, "vkBeginCommandBuffer Failed");

        return true;
    }

    FORCEINLINE bool End()
    {
        VULKAN_CHECK_RESULT(vkEndCommandBuffer(CommandBuffer), "vkEndCommandBuffer Failed");
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

    FORCEINLINE void PipelineBarrier(
        VkPipelineStageFlags SrcStageMask, 
        VkPipelineStageFlags DstStageMask, 
        VkDependencyFlags DependencyFlags,
        uint32 MemoryBarrierCount, 
        const VkMemoryBarrier* MemoryBarriers, 
        uint32 BufferMemoryBarrierCount, 
        const VkBufferMemoryBarrier* BufferMemoryBarriers,
        uint32 ImageMemoryBarrierCount, 
        const VkImageMemoryBarrier* ImageMemoryBarriers)
    {
        vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, MemoryBarrierCount, MemoryBarriers, BufferMemoryBarrierCount, BufferMemoryBarriers, ImageMemoryBarrierCount, ImageMemoryBarriers);
    }

    FORCEINLINE CVulkanCommandPool* GetCommandPool()
    {
        return &CommandPool;
    }

    FORCEINLINE CVulkanFence* GetFence()
    {
        return &Fence;
    }

	FORCEINLINE VkCommandBuffer GetVkCommandBuffer() const
	{
		return CommandBuffer;
	}
	
private:
    CVulkanCommandPool   CommandPool;
    CVulkanFence         Fence;

    VkCommandBufferLevel Level;
    VkCommandBuffer      CommandBuffer;
};
