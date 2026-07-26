#pragma once
#include "Core/Containers/Queue.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanQuery.h"

class FVulkanCommandPool;
class FVulkanQueryAllocator;

class FVulkanCommandBuffer : public FVulkanDeviceChild, FNonCopyable
{
    friend struct FVulkanCommands;

    class FCommandBuffer : FNonCopyable
    {
    public:
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

        FORCEINLINE VkResult AllocateCommandBuffer(VkDevice InDevice, const VkCommandBufferAllocateInfo* AllocateInfo)
        {
            return vkAllocateCommandBuffers(InDevice, AllocateInfo, &CommandBuffer);
        }

        FORCEINLINE void FreeCommandBuffer(VkDevice InDevice, VkCommandPool InCommandPool)
        {
            vkFreeCommandBuffers(InDevice, InCommandPool, 1, &CommandBuffer);
        }

        FORCEINLINE VkResult ResetCommandBuffer(VkCommandBufferResetFlags Flags)
        {
            return vkResetCommandBuffer(CommandBuffer, Flags);
        }

        FORCEINLINE VkResult BeginCommandBuffer(const VkCommandBufferBeginInfo* BeginInfo)
        {
            return vkBeginCommandBuffer(CommandBuffer, BeginInfo);
        }

        FORCEINLINE VkResult EndCommandBuffer()
        {
            return vkEndCommandBuffer(CommandBuffer);
        }
    
        FORCEINLINE void ClearColorImage(VkImage Image, VkImageLayout ImageLayout, VkClearColorValue* ClearColor, uint32 RangeCount,
            const VkImageSubresourceRange* Ranges)
        {
            vkCmdClearColorImage(CommandBuffer, Image, ImageLayout, ClearColor, RangeCount, Ranges);
        }

        FORCEINLINE void ClearDepthStencilImage(VkImage Image, VkImageLayout ImageLayout, const VkClearDepthStencilValue* DepthStencil,
            uint32 RangeCount, const VkImageSubresourceRange* Ranges)
        {
            vkCmdClearDepthStencilImage(CommandBuffer, Image, ImageLayout, DepthStencil, RangeCount, Ranges);
        }
    
        FORCEINLINE void ResolveImage(VkImage SrcImage, VkImageLayout SrcImageLayout, VkImage DstImage, VkImageLayout DstImageLayout,
            uint32 RegionCount, const VkImageResolve* Regions)
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

        FORCEINLINE void BeginRendering(const VkRenderingInfo* RenderingInfo)
        {
            vkCmdBeginRendering(CommandBuffer, RenderingInfo);
        }

        FORCEINLINE void EndRendering()
        {
            vkCmdEndRendering(CommandBuffer);
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
    
        FORCEINLINE void BindDescriptorSets(VkPipelineBindPoint PipelineBindPoint, VkPipelineLayout Layout, uint32 FirstSet, uint32 DescriptorSetCount,
            const VkDescriptorSet* DescriptorSets, uint32 DynamicOffsetCount, const uint32* DynamicOffsets)
        {
            vkCmdBindDescriptorSets(CommandBuffer, PipelineBindPoint, Layout, FirstSet, DescriptorSetCount, DescriptorSets, DynamicOffsetCount, DynamicOffsets);
        }
    
        FORCEINLINE void SetBlendConstants(const float BlendConstants[4])
        {
            vkCmdSetBlendConstants(CommandBuffer, BlendConstants);
        }

        FORCEINLINE void SetStencilReference(VkStencilFaceFlags FaceMask, uint32 Reference)
        {
            vkCmdSetStencilReference(CommandBuffer, FaceMask, Reference);
        }

        FORCEINLINE void SetDepthBias(float DepthBiasConstantFactor, float DepthBiasClamp, float DepthBiasSlopeFactor)
        {
            vkCmdSetDepthBias(CommandBuffer, DepthBiasConstantFactor, DepthBiasClamp, DepthBiasSlopeFactor);
        }

    #if VK_EXT_transform_feedback
        FORCEINLINE void BindTransformFeedbackBuffers(uint32 FirstBinding, uint32 BindingCount, const VkBuffer* Buffers, const VkDeviceSize* Offsets, const VkDeviceSize* Sizes)
        {
            vkCmdBindTransformFeedbackBuffersEXT(CommandBuffer, FirstBinding, BindingCount, Buffers, Offsets, Sizes);
        }

        FORCEINLINE void BeginTransformFeedback(uint32 FirstCounterBuffer, uint32 CounterBufferCount, const VkBuffer* CounterBuffers, const VkDeviceSize* CounterBufferOffsets)
        {
            vkCmdBeginTransformFeedbackEXT(CommandBuffer, FirstCounterBuffer, CounterBufferCount, CounterBuffers, CounterBufferOffsets);
        }

        FORCEINLINE void EndTransformFeedback(uint32 FirstCounterBuffer, uint32 CounterBufferCount, const VkBuffer* CounterBuffers, const VkDeviceSize* CounterBufferOffsets)
        {
            vkCmdEndTransformFeedbackEXT(CommandBuffer, FirstCounterBuffer, CounterBufferCount, CounterBuffers, CounterBufferOffsets);
        }
    #endif
    
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

        FORCEINLINE void CopyImageToBuffer(VkImage SrcImage, VkImageLayout SrcImageLayout, VkBuffer DstBuffer, uint32 RegionCount, const VkBufferImageCopy* Regions)
        {
            vkCmdCopyImageToBuffer(CommandBuffer, SrcImage, SrcImageLayout, DstBuffer, RegionCount, Regions);
        }
    
        FORCEINLINE void CopyImage(VkImage SrcImage, VkImageLayout SrcImageLayout, VkImage DstImage, VkImageLayout DstImageLayout,
            uint32 RegionCount, const VkImageCopy* Regions)
        {
            vkCmdCopyImage(CommandBuffer, SrcImage, SrcImageLayout, DstImage, DstImageLayout, RegionCount, Regions);
        }
    
        FORCEINLINE void BlitImage(VkImage SrcImage, VkImageLayout SrcImageLayout, VkImage DstImage, VkImageLayout DstImageLayout,
            uint32 RegionCount, const VkImageBlit* Regions, VkFilter Filter)
        {
            vkCmdBlitImage(CommandBuffer, SrcImage, SrcImageLayout, DstImage, DstImageLayout, RegionCount, Regions, Filter);
        }

        FORCEINLINE void BufferMemoryPipelineBarrier(VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags,
            uint32 BufferMemoryBarrierCount, const VkBufferMemoryBarrier* BufferMemoryBarriers)
        {
            vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, 0, nullptr, BufferMemoryBarrierCount, BufferMemoryBarriers, 0, nullptr);
        }

        FORCEINLINE void ImageMemoryPipelineBarrier(VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags,
            uint32 ImageMemoryBarrierCount, const VkImageMemoryBarrier* ImageMemoryBarriers)
        {
            vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, 0, nullptr, 0, nullptr, ImageMemoryBarrierCount, ImageMemoryBarriers);
        }

        FORCEINLINE void PipelineBarrier(VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags, 
            uint32 MemoryBarrierCount, const VkMemoryBarrier* MemoryBarriers, uint32 BufferMemoryBarrierCount, const VkBufferMemoryBarrier* BufferMemoryBarriers, 
            uint32 ImageMemoryBarrierCount, const VkImageMemoryBarrier*  ImageMemoryBarriers)
        {
            vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, MemoryBarrierCount, MemoryBarriers, 
                BufferMemoryBarrierCount, BufferMemoryBarriers, ImageMemoryBarrierCount, ImageMemoryBarriers);
        }
    
        FORCEINLINE void PipelineBarrier2(const VkDependencyInfo* DependencyInfo)
        {
            vkCmdPipelineBarrier2(CommandBuffer, DependencyInfo);
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

    #if VK_EXT_mesh_shader
        FORCEINLINE void DrawMeshTasks(uint32 GroupCountX, uint32 GroupCountY, uint32 GroupCountZ)
        {
            vkCmdDrawMeshTasksEXT(CommandBuffer, GroupCountX, GroupCountY, GroupCountZ);
        }
    #endif
    
    #if VK_EXT_debug_utils
        FORCEINLINE void InsertDebugUtilsLabel(const VkDebugUtilsLabelEXT* LabelInfo)
        {
            vkCmdInsertDebugUtilsLabelEXT(CommandBuffer, LabelInfo);
        }

        FORCEINLINE void BeginDebugUtilsLabel(const VkDebugUtilsLabelEXT* LabelInfo)
        {
            vkCmdBeginDebugUtilsLabelEXT(CommandBuffer, LabelInfo);
        }

        FORCEINLINE void EndDebugUtilsLabel()
        {
            vkCmdEndDebugUtilsLabelEXT(CommandBuffer);
        }
    #endif

        FORCEINLINE void WriteTimestamp(VkPipelineStageFlagBits PipelineStage, VkQueryPool QueryPool, uint32 Query)
        {
            vkCmdWriteTimestamp(CommandBuffer, PipelineStage, QueryPool, Query);
        }

        FORCEINLINE void BeginQuery(VkQueryPool QueryPool, uint32 Query, VkQueryControlFlags Flags)
        {
            vkCmdBeginQuery(CommandBuffer, QueryPool, Query, Flags);
        }

        FORCEINLINE void EndQuery(VkQueryPool QueryPool, uint32 Query)
        {
            vkCmdEndQuery(CommandBuffer, QueryPool, Query);
        }

        FORCEINLINE void CopyQueryPoolResults(VkQueryPool QueryPool, uint32 FirstQuery, uint32 QueryCount, VkBuffer DstBuffer, VkDeviceSize DstOffset, VkDeviceSize Stride, VkQueryResultFlags Flags)
        {
            vkCmdCopyQueryPoolResults(CommandBuffer, QueryPool, FirstQuery, QueryCount, DstBuffer, DstOffset, Stride, Flags);
        }

        FORCEINLINE void ResetQueryPool(VkQueryPool QueryPool, uint32 FirstQuery, uint32 QueryCount)
        {
            vkCmdResetQueryPool(CommandBuffer, QueryPool, FirstQuery, QueryCount);
        }

    #if VK_KHR_acceleration_structure
        FORCEINLINE void BuildAccelerationStructures(uint32 InfoCount, const VkAccelerationStructureBuildGeometryInfoKHR* Infos, 
            const VkAccelerationStructureBuildRangeInfoKHR* const* BuildRangeInfos)
        {
            vkCmdBuildAccelerationStructuresKHR(CommandBuffer, InfoCount, Infos, BuildRangeInfos);
        }

        FORCEINLINE void CopyAccelerationStructure(const VkCopyAccelerationStructureInfoKHR* Info)
        {
            vkCmdCopyAccelerationStructureKHR(CommandBuffer, Info);
        }

        FORCEINLINE void CopyAccelerationStructureToMemory(const VkCopyAccelerationStructureToMemoryInfoKHR* Info)
        {
            vkCmdCopyAccelerationStructureToMemoryKHR(CommandBuffer, Info);
        }

        FORCEINLINE void CopyMemoryToAccelerationStructure(const VkCopyMemoryToAccelerationStructureInfoKHR* Info)
        {
            vkCmdCopyMemoryToAccelerationStructureKHR(CommandBuffer, Info);
        }

        FORCEINLINE void WriteAccelerationStructuresProperties(uint32 Count, const VkAccelerationStructureKHR* Structures, VkQueryType QueryType, VkQueryPool QueryPool, uint32 FirstQuery)
        {
            vkCmdWriteAccelerationStructuresPropertiesKHR(CommandBuffer, Count, Structures, QueryType, QueryPool, FirstQuery);
        }
    #endif

    #if VK_EXT_opacity_micromap
        FORCEINLINE void BuildMicromaps(uint32 InfoCount, const VkMicromapBuildInfoEXT* Infos)
        {
            vkCmdBuildMicromapsEXT(CommandBuffer, InfoCount, Infos);
        }
    #endif

    #if VK_KHR_ray_tracing_pipeline
        FORCEINLINE void TraceRays(const VkStridedDeviceAddressRegionKHR* RayGenRegion, const VkStridedDeviceAddressRegionKHR* MissRegion,
            const VkStridedDeviceAddressRegionKHR* HitGroupRegion, const VkStridedDeviceAddressRegionKHR* CallableRegion, uint32 Width, uint32 Height, uint32 Depth)
        {
            vkCmdTraceRaysKHR(CommandBuffer, RayGenRegion, MissRegion, HitGroupRegion, CallableRegion, Width, Height, Depth);
        }

        FORCEINLINE void TraceRaysIndirect(const VkStridedDeviceAddressRegionKHR* RayGenRegion, const VkStridedDeviceAddressRegionKHR* MissRegion,
            const VkStridedDeviceAddressRegionKHR* HitGroupRegion, const VkStridedDeviceAddressRegionKHR* CallableRegion, VkDeviceAddress IndirectDeviceAddress)
        {
            vkCmdTraceRaysIndirectKHR(CommandBuffer, RayGenRegion, MissRegion, HitGroupRegion, CallableRegion, IndirectDeviceAddress);
        }
    #endif

    #if VK_NV_cluster_acceleration_structure
        FORCEINLINE void BuildClusterAccelerationStructureIndirect(const VkClusterAccelerationStructureCommandsInfoNV* CommandInfos)
        {
            vkCmdBuildClusterAccelerationStructureIndirectNV(CommandBuffer, CommandInfos);
        }
    #endif

    #if VK_NV_partitioned_acceleration_structure
        FORCEINLINE void BuildPartitionedAccelerationStructures(const VkBuildPartitionedAccelerationStructureInfoNV* BuildInfo)
        {
            vkCmdBuildPartitionedAccelerationStructuresNV(CommandBuffer, BuildInfo);
        }
    #endif

        FORCEINLINE VkCommandBuffer GetVkCommandBuffer() const
        {
            return CommandBuffer;
        }

    private:
        VkCommandBuffer CommandBuffer;
    };

public:
    FVulkanCommandBuffer(FVulkanDevice* InDevice, FVulkanCommandPool* InOwnerPool);
    ~FVulkanCommandBuffer();

    bool Initialize(VkCommandBufferLevel InLevel);
    bool Reset();
    bool Begin(VkCommandBufferUsageFlags Flags = 0);
    bool End();

    void BeginQuery(const FVulkanQuery& Query);
    void EndQuery(const FVulkanQuery& Query, VkPipelineStageFlagBits TimestampStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    void InsertBeginTimestamp(FVulkanQueryAllocator& Allocator);
    void InsertEndTimestamp(FVulkanQueryAllocator& Allocator);

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
    FVulkanQuery         BeginTimestamp;
    FVulkanQuery         EndTimestamp;
    TArray<FVulkanQuery> TimestampQueries;
    TArray<FVulkanQuery> OcclusionQueries;
    TArray<FVulkanQuery> PipelineStatsQueries;
};

class FVulkanCommandPool : public FVulkanDeviceChild, FNonCopyable
{
public:
    FVulkanCommandPool(FVulkanDevice* InDevice, EVulkanCommandQueueType InType);
    ~FVulkanCommandPool();

    bool Initialize(VkCommandPoolCreateFlags InFlags);
    bool Reset(VkCommandPoolResetFlags Flags);
    void DestroyBuffers();

    FVulkanCommandBuffer* GetOrCreateBuffer();
    void RecycleBuffer(FVulkanCommandBuffer* InCommandBuffer);
    
    VkCommandPool GetVkCommandPool() const
    {
        return CommandPool;
    }
    
private:
    bool AllowCommandBufferReset() const
    {
        return (Flags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) == VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    }

    VkCommandPool                 CommandPool;
    EVulkanCommandQueueType       Type;
    VkCommandPoolCreateFlags      Flags;
    TQueue<FVulkanCommandBuffer*> AvailableCommandBuffers;
    TArray<FVulkanCommandBuffer*> RecycledCommandBuffers;
    TArray<FVulkanCommandBuffer*> CommandBuffers;
};
