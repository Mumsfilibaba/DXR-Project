#pragma once
#include "VulkanCommandPool.h"
#include "VulkanFence.h"

struct FVulkanBufferBarrier
{
    VkPipelineStageFlags SrcStageMask;
    VkPipelineStageFlags DstStageMask;
    VkDependencyFlags    DependencyFlags;
    VkBuffer             Buffer;
    VkAccessFlags        SrcAccessMask;
    VkAccessFlags        DstAccessMask;
    VkDeviceSize         Offset;
    VkDeviceSize         Size;
};

struct FVulkanImageTransitionBarrier
{
    VkPipelineStageFlags    SrcStageMask;
    VkPipelineStageFlags    DstStageMask;
    VkDependencyFlags       DependencyFlags;
    VkImage                 Image;
    VkAccessFlags           SrcAccessMask;
    VkAccessFlags           DstAccessMask;
    VkImageLayout           PreviousLayout;
    VkImageLayout           NewLayout;
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

        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        BeginInfo.flags = Flags;

        VkResult Result = vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
        VULKAN_CHECK_RESULT(Result, "vkBeginCommandBuffer Failed");

        bIsRecording = true;
        return true;
    }

    FORCEINLINE bool End()
    {
        VkResult Result = vkEndCommandBuffer(CommandBuffer);
        VULKAN_CHECK_RESULT(Result, "vkEndCommandBuffer Failed");

        NumCommands  = 0;
        bIsRecording = false;
        return true;
    }

    FORCEINLINE void ClearColorImage(VkImage Image, VkImageLayout ImageLayout, VkClearColorValue* ClearColor, uint32 RangeCount, VkImageSubresourceRange* Ranges)
    {
        vkCmdClearColorImage(CommandBuffer, Image, ImageLayout, ClearColor, RangeCount, Ranges);
        NumCommands++;
    }

    FORCEINLINE void ClearDepthStencilImage(VkImage Image, VkImageLayout ImageLayout, const VkClearDepthStencilValue* DepthStencil, uint32 RangeCount, VkImageSubresourceRange* Ranges)
    {
        vkCmdClearDepthStencilImage(CommandBuffer, Image, ImageLayout, DepthStencil, RangeCount, Ranges);
        NumCommands++;
    }
    
    FORCEINLINE void ResolveImage(VkImage SrcImage, VkImageLayout SrcImageLayout, VkImage DstImage, VkImageLayout DstImageLayout, uint32 RegionCount, const VkImageResolve* Regions)
    {
        vkCmdResolveImage(CommandBuffer, SrcImage, SrcImageLayout, DstImage, DstImageLayout, RegionCount, Regions);
        NumCommands++;
    }

    FORCEINLINE void BeginRenderPass(const VkRenderPassBeginInfo* RenderPassBegin, VkSubpassContents SubpassContents)
    {
        vkCmdBeginRenderPass(CommandBuffer, RenderPassBegin, SubpassContents);
        NumCommands++;
    }

    FORCEINLINE void EndRenderPass()
    {
        vkCmdEndRenderPass(CommandBuffer);
        NumCommands++;
    }
    
    FORCEINLINE void SetViewport(uint32 FirstViewport, uint32 ViewportCount, const VkViewport* Viewports)
    {
        vkCmdSetViewport(CommandBuffer, FirstViewport, ViewportCount, Viewports);
        NumCommands++;
    }
    
    FORCEINLINE void SetScissor(uint32 FirstScissor, uint32 ScissorCount, const VkRect2D* Scissors)
    {
        vkCmdSetScissor(CommandBuffer, FirstScissor, ScissorCount, Scissors);
        NumCommands++;
    }
    
    FORCEINLINE void BindVertexBuffers(uint32 FirstBinding, uint32 BindingCount, const VkBuffer* Buffers, const VkDeviceSize* Offsets)
    {
        vkCmdBindVertexBuffers(CommandBuffer, FirstBinding, BindingCount, Buffers, Offsets);
        NumCommands++;
    }
    
    FORCEINLINE void BindIndexBuffer(VkBuffer Buffer, VkDeviceSize Offset, VkIndexType IndexType)
    {
        vkCmdBindIndexBuffer(CommandBuffer, Buffer, Offset, IndexType);
        NumCommands++;
    }
    
    FORCEINLINE void SetBlendConstants(const float BlendConstants[4])
    {
        vkCmdSetBlendConstants(CommandBuffer, BlendConstants);
        NumCommands++;
    }
    
    FORCEINLINE void BindPipeline(VkPipelineBindPoint PipelineBindPoint, VkPipeline Pipeline)
    {
        vkCmdBindPipeline(CommandBuffer, PipelineBindPoint, Pipeline);
        NumCommands++;
    }
    
    FORCEINLINE void PushConstants(VkPipelineLayout Layout, VkShaderStageFlags StageFlags, uint32 Offset, uint32 Size, const void* Values)
    {
        vkCmdPushConstants(CommandBuffer, Layout, StageFlags, Offset, Size, Values);
        NumCommands++;
    }
    
    FORCEINLINE void CopyBuffer(VkBuffer SrcBuffer, VkBuffer DstBuffer, uint32 RegionCount, const VkBufferCopy* Regions)
    {
        vkCmdCopyBuffer(CommandBuffer, SrcBuffer, DstBuffer, RegionCount, Regions);
        NumCommands++;
    }

    FORCEINLINE void CopyBufferToImage(VkBuffer SrcBuffer, VkImage DstImage, VkImageLayout DstImageLayout, uint32 RegionCount, const VkBufferImageCopy* Regions)
    {
        vkCmdCopyBufferToImage(CommandBuffer, SrcBuffer, DstImage, DstImageLayout, RegionCount, Regions);
        NumCommands++;
    }
    
    FORCEINLINE void CopyImage(VkImage SrcImage, VkImageLayout SrcImageLayout, VkImage DstImage, VkImageLayout DstImageLayout, uint32 RegionCount, const VkImageCopy* Regions)
    {
        vkCmdCopyImage(CommandBuffer, SrcImage, SrcImageLayout, DstImage, DstImageLayout, RegionCount, Regions);
        NumCommands++;
    }

    FORCEINLINE void BufferMemoryPipelineBarrier(const FVulkanBufferBarrier& BufferBarrier)
    {
        VkBufferMemoryBarrier BufferMemoryBarrier;
        FMemory::Memzero(&BufferMemoryBarrier);
        
        BufferMemoryBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        BufferMemoryBarrier.srcAccessMask       = BufferBarrier.SrcAccessMask;
        BufferMemoryBarrier.dstAccessMask       = BufferBarrier.DstAccessMask;
        BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        BufferMemoryBarrier.buffer              = BufferBarrier.Buffer;
        BufferMemoryBarrier.offset              = BufferBarrier.Offset;
        BufferMemoryBarrier.size                = BufferBarrier.Size;
        
        BufferMemoryPipelineBarrier(BufferBarrier.SrcStageMask, BufferBarrier.DstStageMask, BufferBarrier.DependencyFlags, 1, &BufferMemoryBarrier);
    }
    
    FORCEINLINE void BufferMemoryPipelineBarrier(VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags, uint32 BufferMemoryBarrierCount, const VkBufferMemoryBarrier* BufferMemoryBarriers)
    {
        vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, 0, nullptr, BufferMemoryBarrierCount, BufferMemoryBarriers, 0, nullptr);
        NumCommands++;
    }

    FORCEINLINE void ImageLayoutTransitionBarrier(const FVulkanImageTransitionBarrier& TransitionBarrier)
    {
        VkImageMemoryBarrier ImageMemoryBarrier;
        FMemory::Memzero(&ImageMemoryBarrier);

        ImageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
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

    void ImageMemoryPipelineBarrier(VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags, uint32 ImageMemoryBarrierCount, const VkImageMemoryBarrier* ImageMemoryBarriers)
    {
        vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, 0, nullptr, 0, nullptr, ImageMemoryBarrierCount, ImageMemoryBarriers);
        NumCommands++;
    }
    
    void Dispatch(uint32 GroupCountX, uint32 GroupCountY, uint32 GroupCountZ)
    {
        vkCmdDispatch(CommandBuffer, GroupCountX, GroupCountY, GroupCountZ);
        NumCommands++;
    }

    bool WaitForFence(uint64 TimeOut = UINT64_MAX)
    {
        return Fence.Wait(TimeOut);
    }

    FVulkanCommandPool* GetCommandPool()
    {
        return &CommandPool;
    }

    FVulkanFence* GetFence()
    {
        return &Fence;
    }

    VkCommandBuffer GetVkCommandBuffer() const
    {
        return CommandBuffer;
    }
    
    bool IsRecording() const
    {
        return bIsRecording;
    }

    uint32 GetNumCommands() const 
    {
        return NumCommands;
    }

private:
    FVulkanCommandPool   CommandPool;
    FVulkanFence         Fence;

    VkCommandBufferLevel Level;
    VkCommandBuffer      CommandBuffer;

    uint32               NumCommands;
    bool                 bIsRecording;
};
