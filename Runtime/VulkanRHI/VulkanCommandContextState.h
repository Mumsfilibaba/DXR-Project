#pragma once
#include "VulkanQueue.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipelineState.h"

class FVulkanCommandContext;
class FVulkanDescriptorState;

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

struct FVulkanPushConstantsCache
{
    FVulkanPushConstantsCache()
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(Constants, sizeof(Constants));
        NumConstants = VULKAN_MAX_NUM_PUSH_CONSTANTS;
    }

    uint32 Constants[VULKAN_MAX_NUM_PUSH_CONSTANTS];
    uint32 NumConstants;
};

struct FVulkanSRVCache
{
    FVulkanSRVCache()
    {
        Clear();
    }
    
    void Clear()
    {
        FMemory::Memzero(Views, sizeof(Views));
    }
    
    FVulkanShaderResourceView* Views[VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT];
};

struct FVulkanUAVCache
{
    FVulkanUAVCache()
    {
        Clear();
    }
    
    void Clear()
    {
        FMemory::Memzero(Views, sizeof(Views));
    }
    
    FVulkanUnorderedAccessView* Views[VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT];
};

struct FVulkanSamplerCache
{
    FVulkanSamplerCache()
    {
        Clear();
    }
    
    void Clear()
    {
        FMemory::Memzero(Samplers, sizeof(Samplers));
    }
    
    FVulkanSamplerState* Samplers[VULKAN_DEFAULT_SAMPLER_STATE_COUNT];
};

struct FVulkanUniformCache
{
    FVulkanUniformCache()
    {
        Clear();
    }
    
    void Clear()
    {
        FMemory::Memzero(UniformBuffers, sizeof(UniformBuffers));
    }
    
    FVulkanBuffer* UniformBuffers[VULKAN_DEFAULT_SAMPLER_STATE_COUNT];
};

class FVulkanCommandContextState : public FVulkanDeviceChild, public FNonCopyAndNonMovable
{
public:
    FVulkanCommandContextState(FVulkanDevice* InDevice, FVulkanCommandContext& InContext);
    ~FVulkanCommandContextState() = default;

    bool Initialize();

    void BindGraphicsStates();
    void BindComputeState();
    void BindDescriptorSets(FVulkanPipelineLayout* PipelineLayout, EShaderVisibility StartStage, EShaderVisibility EndStage);
    void BindPushConstants(FVulkanPipelineLayout* PipelineLayout);
    
    void ResetState();
    void ResetStateForNewCommandBuffer();

    void SetGraphicsPipelineState(FVulkanGraphicsPipelineState* InGraphicsPipelineState);
    void SetComputePipelineState(FVulkanComputePipelineState* InComputePipelineState);

    void SetViewports(VkViewport* Viewports, uint32 NumViewports);
    void SetScissorRects(VkRect2D* ScissorRects, uint32 NumScissorRects);

    void SetBlendFactor(const float BlendFactor[4]);

    void SetVertexBuffer(FVulkanBuffer* VertexBuffer, uint32 VertexBufferSlot);
    void SetIndexBuffer(FVulkanBuffer* IndexBuffer, VkIndexType IndexFormat);

    void SetPushConstants(const uint32* ShaderConstants, uint32 NumShaderConstants);

    FORCEINLINE void SetSRV(FVulkanShaderResourceView* ShaderResourceView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
    {
        CHECK(ResourceIndex < VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT);
        CommonState.SRVCache[ShaderStage].Views[ResourceIndex] = ShaderResourceView;
    }

    FORCEINLINE void SetUAV(FVulkanUnorderedAccessView* UnorderedAccessView, EShaderVisibility ShaderStage, uint32 ResourceIndex)
    {
        CHECK(ResourceIndex < VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT);
        CommonState.UAVCache[ShaderStage].Views[ResourceIndex] = UnorderedAccessView;
    }

    FORCEINLINE void SetUniformBuffer(FVulkanBuffer* ConstantBuffer, EShaderVisibility ShaderStage, uint32 ResourceIndex)
    {
        CHECK(ResourceIndex < VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT);
        CommonState.UniformCache[ShaderStage].UniformBuffers[ResourceIndex] = ConstantBuffer;
    }

    FORCEINLINE void SetSampler(FVulkanSamplerState* SamplerState, EShaderVisibility ShaderStage, uint32 SamplerIndex)
    {
        CHECK(SamplerIndex < VULKAN_DEFAULT_SAMPLER_STATE_COUNT);
        CommonState.SamplerCache[ShaderStage].Samplers[SamplerIndex] = SamplerState;
    }

public:
    FORCEINLINE FVulkanCommandContext& GetContext()
    {
        return Context;
    }

public:
    FORCEINLINE FVulkanGraphicsPipelineState* GetGraphicsPipelineState() const
    {
        return GraphicsState.PipelineState.Get();
    }

    FORCEINLINE FVulkanComputePipelineState* GetComputePipelineState() const
    {
        return ComputeState.PipelineState.Get();
    }

    FORCEINLINE void GetViewports(VkViewport* Viewports, uint32& OutNumViewports) const
    {
        if (Viewports)
        {
            FMemory::Memcpy(Viewports, GraphicsState.Viewports, sizeof(VkViewport) * GraphicsState.NumViewports);
        }

        OutNumViewports = GraphicsState.NumViewports;
    }

    FORCEINLINE void GetViewports(VkRect2D* ScissorRects, uint32& OutNumScissorRects) const
    {
        if (ScissorRects)
        {
            FMemory::Memcpy(ScissorRects, GraphicsState.ScissorRects, sizeof(VkRect2D) * GraphicsState.NumScissorRects);
        }

        OutNumScissorRects = GraphicsState.NumScissorRects;
    }

    FORCEINLINE void GetBlendFactor(float* BlendFactor) const
    {
        if (BlendFactor)
        {
            FMemory::Memcpy(BlendFactor, GraphicsState.BlendFactor, sizeof(GraphicsState.BlendFactor));
        }
    }

private:
    struct FGraphicsState
    {
        FGraphicsState()
            : PipelineState(nullptr)
            , NumViewports(0)
            , NumScissorRects(0)
            , IBCache()
            , VBCache()
        {
            FMemory::Memzero(BlendFactor, sizeof(BlendFactor));
            FMemory::Memzero(Viewports, sizeof(Viewports));
            FMemory::Memzero(ScissorRects, sizeof(ScissorRects));
        }

        FVulkanGraphicsPipelineStateRef PipelineState;

        TMap<FVulkanGraphicsPipelineState*, FVulkanDescriptorState*> DescriptorStates;
        FVulkanDescriptorState* CurrentDescriptorState;
        
        float BlendFactor[4];

        VkViewport Viewports[VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32     NumViewports;

        VkRect2D   ScissorRects[VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32     NumScissorRects;

        FVulkanIndexBufferCache  IBCache;
        FVulkanVertexBufferCache VBCache;

        bool bBindBlendFactor   : 1;
        bool bBindPipelineState : 1;
        bool bBindScissorRects  : 1;
        bool bBindViewports     : 1;
        bool bBindVertexBuffers : 1;
        bool bBindIndexBuffer   : 1;
        bool bBindPushConstants : 1;
    } GraphicsState;

    struct FComputeState
    {
        FComputeState()
            : PipelineState(nullptr)
        {
        }

        FVulkanComputePipelineStateRef PipelineState;

        TMap<FVulkanComputePipelineState*, FVulkanDescriptorState*> DescriptorStates;
        FVulkanDescriptorState* CurrentDescriptorState;
        
        bool bBindPipelineState : 1;
        bool bBindPushConstants : 1;
    } ComputeState;

    struct FCommonState
    {
        FVulkanPushConstantsCache PushConstantsCache;
        FVulkanSRVCache           SRVCache[ShaderVisibility_Count];
        FVulkanUAVCache           UAVCache[ShaderVisibility_Count];
        FVulkanUniformCache       UniformCache[ShaderVisibility_Count];
        FVulkanSamplerCache       SamplerCache[ShaderVisibility_Count];
    } CommonState;
    
    FVulkanCommandContext& Context;
};
