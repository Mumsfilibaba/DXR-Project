#pragma once
#include "VulkanLoader.h"

class FVulkanBuffer;

struct FVulkanVertexBufferCache
{
    FVulkanVertexBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(VertexBuffers      , sizeof(VertexBuffers));
        FMemory::Memzero(VertexBufferOffsets, sizeof(VertexBufferOffsets));
        NumVertexBuffers = 0;
    }

    VkBuffer     VertexBuffers[VULKAN_MAX_VERTEX_BUFFER_SLOTS];
    VkDeviceSize VertexBufferOffsets[VULKAN_MAX_VERTEX_BUFFER_SLOTS];
    uint32       NumVertexBuffers;
};


struct FVulkanIndexBufferCache
{
    FVulkanIndexBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        IndexType   = VK_INDEX_TYPE_UINT32;
        Offset      = 0;
        IndexBuffer = VK_NULL_HANDLE;
    }

    VkBuffer     IndexBuffer;
    VkDeviceSize Offset;
    VkIndexType  IndexType;
};


class FVulkanPushConstantsCache
{
public:
    FVulkanPushConstantsCache()
    {
        Reset();
    }

    void SetPushConstants(const uint32* InConstants, uint32 InNumConstants)
    {
        VULKAN_ERROR_COND(
            InNumConstants <= VULKAN_MAX_NUM_PUSH_CONSTANTS,
            "Trying to set a number of push-constants (NumConstants=%u) higher than the maximum (MaxShaderConstants=%u)",
            InNumConstants,
            VULKAN_MAX_NUM_PUSH_CONSTANTS);

        FMemory::Memcpy(Constants, InConstants, sizeof(uint32) * InNumConstants);
        NumConstants = InNumConstants;
        bIsDirty     = true;
    }

    void Commit(FVulkanCommandBuffer& CommandBuffer, VkPipelineLayout PipelineLayout)
    {
        if (bIsDirty && NumConstants > 0)
        {
            CommandBuffer.PushConstants(PipelineLayout, VK_SHADER_STAGE_ALL, 0, NumConstants * sizeof(uint32), Constants);
            bIsDirty = false;
        }
    }

    void Reset()
    {
        // Reset by setting all constants to zero, this ensures that a shader always at least reads from zero constants
        NumConstants = VULKAN_MAX_NUM_PUSH_CONSTANTS;
        bIsDirty     = true;
        FMemory::Memzero(Constants, sizeof(Constants));
    }

private:
    uint32 Constants[VULKAN_MAX_NUM_PUSH_CONSTANTS];
    uint32 NumConstants;
    bool   bIsDirty;
};