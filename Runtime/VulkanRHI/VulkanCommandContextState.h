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

class FVulkanCommandContextState : public FVulkanDeviceChild, public FNonCopyAndNonMovable
{
public:
    FVulkanCommandContextState(FVulkanDevice* InDevice, FVulkanCommandContext& InContext);
    ~FVulkanCommandContextState();

    bool Initialize();
    void BindGraphicsStates();
    void BindComputeState();
    void BindPushConstants(FVulkanPipelineLayout* PipelineLayout);
    void ResetState();
    void ResetStateForNewCommandBuffer();

    void SetViewInstanceInfo(const FViewInstancingInfo& InViewInstancingInfo);
    void SetGraphicsPipelineState(FVulkanGraphicsPipelineState* InGraphicsPipelineState);
    void SetComputePipelineState(FVulkanComputePipelineState* InComputePipelineState);
    void SetViewports(VkViewport* Viewports, uint32 NumViewports);
    void SetScissorRects(VkRect2D* ScissorRects, uint32 NumScissorRects);
    void SetBlendFactor(const float BlendFactor[4]);
    void SetVertexBuffer(FVulkanBuffer* VertexBuffer, uint32 VertexBufferSlot);
    void SetIndexBuffer(FVulkanBuffer* IndexBuffer, VkIndexType IndexFormat);
    void SetPushConstants(const uint32* ShaderConstants, uint32 NumShaderConstants);
    void SetSRV(FVulkanShaderResourceView* ShaderResourceView, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetUAV(FVulkanUnorderedAccessView* UnorderedAccessView, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetUniformBuffer(FVulkanBuffer* UniformBuffer, EShaderVisibility ShaderStage, uint32 ResourceIndex);
    void SetSampler(FVulkanSamplerState* SamplerState, EShaderVisibility ShaderStage, uint32 SamplerIndex);

    FORCEINLINE FVulkanCommandContext& GetContext()
    {
        return Context;
    }

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
            : CurrentLayout(nullptr)
            , PipelineState(nullptr)
            , ViewInstancingInfo()
            , DescriptorStates()
            , CurrentDescriptorState(nullptr)
            , NumViewports(0)
            , NumScissorRects(0)
            , IBCache()
            , VBCache()
        {
            FMemory::Memzero(BlendFactor, sizeof(BlendFactor));
            FMemory::Memzero(Viewports, sizeof(Viewports));
            FMemory::Memzero(ScissorRects, sizeof(ScissorRects));
        }

        FVulkanPipelineLayout* CurrentLayout;
        FVulkanGraphicsPipelineStateRef PipelineState;
        FViewInstancingInfo ViewInstancingInfo;

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
            : CurrentLayout(nullptr)
            , PipelineState(nullptr)
            , DescriptorStates()
            , CurrentDescriptorState(nullptr)
        {
        }

        FVulkanPipelineLayout* CurrentLayout;
        FVulkanComputePipelineStateRef PipelineState;

        TMap<FVulkanComputePipelineState*, FVulkanDescriptorState*> DescriptorStates;
        FVulkanDescriptorState* CurrentDescriptorState;
        
        bool bBindPipelineState : 1;
        bool bBindPushConstants : 1;
    } ComputeState;

    struct FCommonState
    {
        FVulkanPushConstantsCache PushConstantsCache;
    } CommonState;
    
    FVulkanCommandContext& Context;
};
