#pragma once
#include "VulkanLoader.h"
#include "VulkanSamplerState.h"
#include "VulkanResourceViews.h"

class FVulkanBuffer;
class FVulkanCommandContext;

struct FVulkanVertexBufferCache
{
    FVulkanVertexBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(VertexBuffers, sizeof(VertexBuffers));
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


struct FVulkanResourceCache
{
    bool IsDirty(EShaderVisibility ShaderStage) const
    {
        return bDirty[ShaderStage];
    }

    void DirtyState(uint32 StartStage, uint32 EndStage)
    {
        CHECK(EndStage < ShaderVisibility_Count);

        for (uint32 Index = StartStage; Index < ShaderVisibility_Count; Index++)
        {
            bDirty[Index] = true;
        }
    }

    void DirtyStateAll()
    {
        for (uint32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            bDirty[Index] = true;
        }
    }

    bool bDirty[ShaderVisibility_Count];
};


struct FVulkanConstantBufferCache : public FVulkanResourceCache
{
    FVulkanConstantBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyState(ShaderVisibility_All, ShaderVisibility_Pixel);

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageConstantBuffers = ConstantBuffers[Index];
            FMemory::Memzero(&StageConstantBuffers, sizeof(StageConstantBuffers));
            NumBuffers[Index] = 0;
        }
    }

    FVulkanBuffer* ConstantBuffers[ShaderVisibility_Count][VULKAN_DEFAULT_CONSTANT_BUFFER_COUNT];
    uint8 NumBuffers[ShaderVisibility_Count];
};


struct FVulkanShaderResourceViewCache : public FVulkanResourceCache
{
    FVulkanShaderResourceViewCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyState(ShaderVisibility_All, ShaderVisibility_Pixel);

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageViews = ResourceViews[Index];
            FMemory::Memzero(&StageViews, sizeof(StageViews));
            NumViews[Index] = 0;
        }
    }

    FVulkanShaderResourceView* ResourceViews[ShaderVisibility_Count][VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT];
    uint8 NumViews[ShaderVisibility_Count];
};


struct FVulkanUnorderedAccessViewCache : public FVulkanResourceCache
{
    FVulkanUnorderedAccessViewCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyState(ShaderVisibility_All, ShaderVisibility_Pixel);

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageViews = ResourceViews[Index];
            FMemory::Memzero(&StageViews, sizeof(StageViews));
            NumViews[Index] = 0;
        }
    }

    FVulkanUnorderedAccessView* ResourceViews[ShaderVisibility_Count][VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT];
    uint8 NumViews[ShaderVisibility_Count];
};

struct FVulkanPushConstantsCache
{
    FVulkanPushConstantsCache()
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(Constants, sizeof(Constants));
        NumConstants = 0;
    }

    uint32 Constants[VULKAN_MAX_NUM_PUSH_CONSTANTS];
    uint32 NumConstants;
};


struct FVulkanSamplerStateCache : public FVulkanResourceCache
{
    FVulkanSamplerStateCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyState(ShaderVisibility_All, ShaderVisibility_Pixel);

        for (int32 Index = 0; Index < ShaderVisibility_Count; Index++)
        {
            auto& StageSamplers = SamplerStates[Index];
            NumSamplers[Index] = VULKAN_DEFAULT_SAMPLER_STATE_COUNT;
            FMemory::Memzero(&StageSamplers, sizeof(StageSamplers));
        }
    }

    FVulkanSamplerState* SamplerStates[ShaderVisibility_Count][VULKAN_DEFAULT_SAMPLER_STATE_COUNT];
    uint8 NumSamplers[ShaderVisibility_Count];
};


struct FVulkanDefaultResources
{
    FVulkanDefaultResources()
        : NullBuffer(VK_NULL_HANDLE)
        , NullImage(VK_NULL_HANDLE)
        , NullImageView(VK_NULL_HANDLE)
        , NullSampler(VK_NULL_HANDLE)
    {
    }
    
    ~FVulkanDefaultResources()
    {
        CHECK(NullBuffer    == VK_NULL_HANDLE);
        CHECK(NullImage     == VK_NULL_HANDLE);
        CHECK(NullImageView == VK_NULL_HANDLE);
        CHECK(NullSampler   == VK_NULL_HANDLE);
    }
    
    bool Initialize(FVulkanDevice& Device);
    void Release(FVulkanDevice& Device);
    
    // Null-Buffer
    VkBuffer                NullBuffer;
    FVulkanMemoryAllocation NullBufferMemory;
    
    // Null-Image
    VkImage                 NullImage;
    VkImageView             NullImageView;
    FVulkanMemoryAllocation NullImageMemory;
    
    // NullSampler
    VkSampler               NullSampler;
};


class FVulkanDescriptorSetCache : public FVulkanDeviceObject
{
public:
    FVulkanDescriptorSetCache(FVulkanDevice* InDevice, FVulkanCommandContext& InContext);
    ~FVulkanDescriptorSetCache();

    bool Initialize();

    void DirtyState();

    void DirtyStateSamplers();

    void DirtyStateResources();

    void SetVertexBuffers(FVulkanVertexBufferCache& VertexBuffers);
    
    void SetIndexBuffer(FVulkanIndexBufferCache& IndexBuffer);
    
    bool AllocateDescriptorSets(VkDescriptorSetLayout Layout);

    void SetSRVs(FVulkanShaderResourceViewCache& Cache, EShaderVisibility ShaderStage, uint32 NumSRVs);
    
    void SetUAVs(FVulkanUnorderedAccessViewCache& Cache, EShaderVisibility ShaderStage, uint32 NumUAVs);

    void SetConstantBuffers(FVulkanConstantBufferCache& Cache, EShaderVisibility ShaderStage, uint32 NumBuffers);
    
    void SetSamplers(FVulkanSamplerStateCache& Cache, EShaderVisibility ShaderStage, uint32 NumSamplers);

    void SetDescriptorSet(VkPipelineLayout PipelineLayout, EShaderVisibility ShaderStage);
    
    void ResetPendingDescriptorPools();
    
    FORCEINLINE FVulkanCommandContext& GetContext()
    {
        return Context;
    }
    
    FORCEINLINE FVulkanDefaultResources& GetDefaultResources()
    {
        return DefaultResources;
    }

private:
    bool AllocateDescriptorPool();
    
    FVulkanCommandContext&  Context;
    FVulkanDefaultResources DefaultResources;
    
    VkDescriptorSet          DescriptorSet;
    VkDescriptorPool         DescriptorPool;
    TArray<VkDescriptorPool> PendingDescriptorPools;
    TArray<VkDescriptorPool> AvailableDescriptorPools;
};
