#pragma once
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanPipelineState.h"

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
    void EvictStaleDescriptorStates();

    void SetViewInstanceInfo(const FRHIViewInstancingState& InViewInstancingInfo);
    void SetGraphicsPipelineState(FVulkanGraphicsPipelineState* InGraphicsPipelineState);
    void SetComputePipelineState(FVulkanComputePipelineState* InComputePipelineState);
    void SetViewports(VkViewport* Viewports, uint32 NumViewports);
    void SetScissorRects(VkRect2D* ScissorRects, uint32 NumScissorRects);
    void SetBlendFactor(const float BlendFactor[4]);
    void SetStencilRef(uint32 InStencilRef);
    void SetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias);
    void SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets);
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

    FORCEINLINE void GetScissorRects(VkRect2D* ScissorRects, uint32& OutNumScissorRects) const
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
    struct FCachedDescriptorState
    {
        FVulkanDescriptorState* State;
        uint64                  LastUsedFrame;
    };

    struct FGraphicsState
    {
        typedef TMap<FVulkanGraphicsPipelineState*, FCachedDescriptorState> FPipelineToDescriptorStateMap;
        
        FGraphicsState()
            : CurrentLayout(nullptr)
            , PipelineState(nullptr)
            , ViewInstancingState()
            , DescriptorStates()
            , CurrentDescriptorState(nullptr)
            , NumViewports(0)
            , NumScissorRects(0)
            , IndexBufferCache()
            , VertexBufferCache()
        {
            FMemory::Memzero(BlendFactor, sizeof(BlendFactor));
            FMemory::Memzero(DepthBias, sizeof(DepthBias));
            StencilRef = 0;
            FMemory::Memzero(Viewports, sizeof(Viewports));
            FMemory::Memzero(ScissorRects, sizeof(ScissorRects));
            FMemory::Memzero(StreamOutputBuffers, sizeof(StreamOutputBuffers));
            FMemory::Memzero(StreamOutputOffsets, sizeof(StreamOutputOffsets));
            FMemory::Memzero(StreamOutputSizes, sizeof(StreamOutputSizes));
            NumStreamOutputBuffers = 0;
        }

        FVulkanPipelineLayout*          CurrentLayout;
        FVulkanGraphicsPipelineStateRef PipelineState;
        FRHIViewInstancingState         ViewInstancingState;
        FPipelineToDescriptorStateMap   DescriptorStates;
        FVulkanDescriptorState*         CurrentDescriptorState;
        float                           BlendFactor[4];
        float                           DepthBias[3];
        uint32                          StencilRef;
        VkViewport                      Viewports[VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                          NumViewports;
        VkRect2D                        ScissorRects[VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                          NumScissorRects;
        FVulkanIndexBufferCache         IndexBufferCache;
        FVulkanVertexBufferCache        VertexBufferCache;
        VkBuffer                        StreamOutputBuffers[VULKAN_MAX_STREAM_OUTPUT_BUFFER_COUNT];
        VkDeviceSize                    StreamOutputOffsets[VULKAN_MAX_STREAM_OUTPUT_BUFFER_COUNT];
        VkDeviceSize                    StreamOutputSizes[VULKAN_MAX_STREAM_OUTPUT_BUFFER_COUNT];
        uint32                          NumStreamOutputBuffers;

        bool bBindBlendFactor          : 1;
        bool bBindStencilRef           : 1;
        bool bBindDepthBias            : 1;
        bool bBindPipelineState        : 1;
        bool bBindScissorRects         : 1;
        bool bBindViewports            : 1;
        bool bBindVertexBuffers        : 1;
        bool bBindIndexBuffer          : 1;
        bool bBindPushConstants        : 1;
        bool bBindStreamOutputTargets  : 1;
    } GraphicsState;

    struct FComputeState
    {
        typedef TMap<FVulkanComputePipelineState*, FCachedDescriptorState> FPipelineToDescriptorStateMap;
        
        FComputeState()
            : CurrentLayout(nullptr)
            , PipelineState(nullptr)
            , DescriptorStates()
            , CurrentDescriptorState(nullptr)
        {
        }

        FVulkanPipelineLayout*         CurrentLayout;
        FVulkanComputePipelineStateRef PipelineState;
        FPipelineToDescriptorStateMap  DescriptorStates;
        FVulkanDescriptorState*        CurrentDescriptorState;
        
        bool bBindPipelineState : 1;
        bool bBindPushConstants : 1;
    } ComputeState;

    struct FCommonState
    {
        FVulkanPushConstantsCache PushConstantsCache;
    } CommonState;
    
    FVulkanCommandContext& Context;
    uint64                 CurrentFrame;
};
