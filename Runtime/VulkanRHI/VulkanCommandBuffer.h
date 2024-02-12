#pragma once
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

class FCommandBuffer
{
public:
    FCommandBuffer(const FCommandBuffer& Other) = delete;
    FCommandBuffer& operator=(const FCommandBuffer& Other) = delete;
    
    FORCEINLINE FCommandBuffer()
        : CommandBuffer(VK_NULL_HANDLE)
    {
    }

    FORCEINLINE FCommandBuffer(FCommandBuffer&& Other)
        : CommandBuffer(Other.CommandBuffer)
    {
        Other.CommandBuffer = VK_NULL_HANDLE;
    }

    FORCEINLINE FCommandBuffer& operator=(FCommandBuffer&& Other)
    {
        CommandBuffer = Move(Other.CommandBuffer);
        Other.CommandBuffer = VK_NULL_HANDLE;
        return *this;
    }

    FORCEINLINE ~FCommandBuffer()
    {
        CHECK(CommandBuffer == VK_NULL_HANDLE);
    }

    FORCEINLINE VkResult AllocateCommandBuffer(VkDevice Device, const VkCommandBufferAllocateInfo* AllocateInfo)
    {
        return vkAllocateCommandBuffers(Device, AllocateInfo, &CommandBuffer);
    }

    FORCEINLINE VkResult BeginCommandBuffer(const VkCommandBufferBeginInfo* BeginInfo)
    {
        return vkBeginCommandBuffer(CommandBuffer, BeginInfo);
    }

    FORCEINLINE VkResult EndCommandBuffer()
    {
        return vkEndCommandBuffer(CommandBuffer);
    }
    
    FORCEINLINE void ClearColorImage(VkImage Image, VkImageLayout ImageLayout, VkClearColorValue* ClearColor, uint32 RangeCount, VkImageSubresourceRange* Ranges)
    {
        vkCmdClearColorImage(CommandBuffer, Image, ImageLayout, ClearColor, RangeCount, Ranges);
    }

    FORCEINLINE void ClearDepthStencilImage(VkImage Image, VkImageLayout ImageLayout, const VkClearDepthStencilValue* DepthStencil, uint32 RangeCount, VkImageSubresourceRange* Ranges)
    {
        vkCmdClearDepthStencilImage(CommandBuffer, Image, ImageLayout, DepthStencil, RangeCount, Ranges);
    }
    
    FORCEINLINE void ResolveImage(VkImage SrcImage, VkImageLayout SrcImageLayout, VkImage DstImage, VkImageLayout DstImageLayout, uint32 RegionCount, const VkImageResolve* Regions)
    {
        vkCmdResolveImage(CommandBuffer, SrcImage, SrcImageLayout, DstImage, DstImageLayout, RegionCount, Regions);
    }

    FORCEINLINE void BeginRenderPass(const VkRenderPassBeginInfo* RenderPassBegin, VkSubpassContents SubpassContents)
    {
        vkCmdBeginRenderPass(CommandBuffer, RenderPassBegin, SubpassContents);
    }

    FORCEINLINE void EndRenderPass()
    {
        vkCmdEndRenderPass(CommandBuffer);
    }
    
    FORCEINLINE void SetViewport(uint32 FirstViewport, uint32 ViewportCount, const VkViewport* Viewports)
    {
        vkCmdSetViewport(CommandBuffer, FirstViewport, ViewportCount, Viewports);
    }
    
    FORCEINLINE void SetScissor(uint32 FirstScissor, uint32 ScissorCount, const VkRect2D* Scissors)
    {
        vkCmdSetScissor(CommandBuffer, FirstScissor, ScissorCount, Scissors);
    }
    
    FORCEINLINE void BindVertexBuffers(uint32 FirstBinding, uint32 BindingCount, const VkBuffer* Buffers, const VkDeviceSize* Offsets)
    {
        vkCmdBindVertexBuffers(CommandBuffer, FirstBinding, BindingCount, Buffers, Offsets);
    }
    
    FORCEINLINE void BindIndexBuffer(VkBuffer Buffer, VkDeviceSize Offset, VkIndexType IndexType)
    {
        vkCmdBindIndexBuffer(CommandBuffer, Buffer, Offset, IndexType);
    }
    
    FORCEINLINE void BindDescriptorSets(VkPipelineBindPoint PipelineBindPoint, VkPipelineLayout Layout, uint32 FirstSet, uint32 DescriptorSetCount, const VkDescriptorSet* DescriptorSets, uint32 DynamicOffsetCount, const uint32* DynamicOffsets)
    {
        vkCmdBindDescriptorSets(CommandBuffer, PipelineBindPoint, Layout, FirstSet, DescriptorSetCount, DescriptorSets, DynamicOffsetCount, DynamicOffsets);
    }
    
    FORCEINLINE void SetBlendConstants(const float BlendConstants[4])
    {
        vkCmdSetBlendConstants(CommandBuffer, BlendConstants);
    }
    
    FORCEINLINE void BindPipeline(VkPipelineBindPoint PipelineBindPoint, VkPipeline Pipeline)
    {
        vkCmdBindPipeline(CommandBuffer, PipelineBindPoint, Pipeline);
    }
    
    FORCEINLINE void PushConstants(VkPipelineLayout Layout, VkShaderStageFlags StageFlags, uint32 Offset, uint32 Size, const void* Values)
    {
        vkCmdPushConstants(CommandBuffer, Layout, StageFlags, Offset, Size, Values);
    }

    FORCEINLINE void FillBuffer(VkBuffer DstBuffer, VkDeviceSize DstOffset, VkDeviceSize Size, uint32 Data)
    {
        vkCmdFillBuffer(CommandBuffer, DstBuffer, DstOffset, Size, Data);
    }
    
    FORCEINLINE void CopyBuffer(VkBuffer SrcBuffer, VkBuffer DstBuffer, uint32 RegionCount, const VkBufferCopy* Regions)
    {
        vkCmdCopyBuffer(CommandBuffer, SrcBuffer, DstBuffer, RegionCount, Regions);
    }

    FORCEINLINE void CopyBufferToImage(VkBuffer SrcBuffer, VkImage DstImage, VkImageLayout DstImageLayout, uint32 RegionCount, const VkBufferImageCopy* Regions)
    {
        vkCmdCopyBufferToImage(CommandBuffer, SrcBuffer, DstImage, DstImageLayout, RegionCount, Regions);
    }
    
    FORCEINLINE void CopyImage(VkImage SrcImage, VkImageLayout SrcImageLayout, VkImage DstImage, VkImageLayout DstImageLayout, uint32 RegionCount, const VkImageCopy* Regions)
    {
        vkCmdCopyImage(CommandBuffer, SrcImage, SrcImageLayout, DstImage, DstImageLayout, RegionCount, Regions);
    }
    
    FORCEINLINE void BlitImage(VkImage SrcImage, VkImageLayout SrcImageLayout, VkImage DstImage, VkImageLayout DstImageLayout, uint32 RegionCount, const VkImageBlit* Regions, VkFilter Filter)
    {
        vkCmdBlitImage(CommandBuffer, SrcImage, SrcImageLayout, DstImage, DstImageLayout, RegionCount, Regions, Filter);
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

    FORCEINLINE void ImageMemoryPipelineBarrier(VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags, uint32 ImageMemoryBarrierCount, const VkImageMemoryBarrier* ImageMemoryBarriers)
    {
        vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, 0, nullptr, 0, nullptr, ImageMemoryBarrierCount, ImageMemoryBarriers);
    }
    
    FORCEINLINE void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance)
    {
        vkCmdDraw(CommandBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
    }
    
    FORCEINLINE void DrawIndexed(uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, int32 VertexOffset, uint32 FirstInstance)
    {
        vkCmdDrawIndexed(CommandBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
    }
    
    FORCEINLINE void Dispatch(uint32 GroupCountX, uint32 GroupCountY, uint32 GroupCountZ)
    {
        vkCmdDispatch(CommandBuffer, GroupCountX, GroupCountY, GroupCountZ);
    }
    
#if VK_EXT_debug_utils
    FORCEINLINE void InsertDebugUtilsLabel(const VkDebugUtilsLabelEXT* LabelInfo)
    {
        vkCmdInsertDebugUtilsLabelEXT(CommandBuffer, LabelInfo);
    }
#endif

    FORCEINLINE VkCommandBuffer GetVkCommandBuffer() const
    {
        return CommandBuffer;
    }

private:
    VkCommandBuffer CommandBuffer;
};


class FVulkanCommandPool;

class FVulkanCommandBuffer : public FVulkanDeviceChild
{
public:
    FVulkanCommandBuffer(const FVulkanCommandBuffer& Other) = delete;
    FVulkanCommandBuffer& operator=(const FVulkanCommandBuffer& Other) = delete;

    FVulkanCommandBuffer(FVulkanDevice* InDevice, FVulkanCommandPool* InOwnerPool);
    ~FVulkanCommandBuffer();

    bool Initialize(VkCommandBufferLevel InLevel);

    bool Begin(VkCommandBufferUsageFlags Flags = 0);
    bool End();

    FVulkanCommandPool* GetOwnerPool()
    {
        return OwnerPool;
    }

    VkCommandBuffer GetVkCommandBuffer() const
    {
        return CommandBuffer.GetVkCommandBuffer();
    }
    
    bool IsRecording() const
    {
        return bIsRecording;
    }

    uint32 GetNumCommands() const 
    {
        return NumCommands;
    }

    FCommandBuffer* operator->()
    {
        NumCommands++;
        return &CommandBuffer;
    }

private:
    FVulkanCommandPool*  OwnerPool;
    FCommandBuffer       CommandBuffer;
    VkCommandBufferLevel Level;
    uint32               NumCommands;
    bool                 bIsRecording;
};

class FVulkanCommandPool : public FVulkanDeviceChild
{
public:
    FVulkanCommandPool(const FVulkanCommandPool& Other) = delete;
    FVulkanCommandPool& operator=(const FVulkanCommandPool& Other) = delete;

    FVulkanCommandPool(FVulkanDevice* InDevice, EVulkanCommandQueueType InType);
    ~FVulkanCommandPool();

    bool Initialize();

    FVulkanCommandBuffer* CreateBuffer();
    void RecycleBuffer(FVulkanCommandBuffer* InCommandBuffer);
    
    bool Reset(VkCommandPoolResetFlags Flags = 0)
    {
        VkResult Result = vkResetCommandPool(GetDevice()->GetVkDevice(), CommandPool, Flags);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("vkResetCommandPool Failed");
            return false;
        }

        return true;
    }

    VkCommandPool GetVkCommandPool() const
    {
        return CommandPool;
    }
    
private:
    VkCommandPool                 CommandPool;
    EVulkanCommandQueueType       Type;
    TArray<FVulkanCommandBuffer*> CommandBuffers;
};
