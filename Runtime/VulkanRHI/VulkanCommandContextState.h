#pragma once
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanPipelineState.h"
#include "VulkanRHI/VulkanRenderPass.h"
#include "RHI/IRHICommandContext.h"

class FVulkanCommandContext;
class FVulkanDescriptorState;
class FVulkanResourceView;
struct FRHIBeginRenderPassInfo;

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

struct FVulkanStreamOutputCache
{
    FVulkanStreamOutputCache()
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(Buffers, sizeof(Buffers));
        FMemory::Memzero(Offsets, sizeof(Offsets));
        FMemory::Memzero(Sizes, sizeof(Sizes));
        NumBuffers = 0;
    }

    VkBuffer     Buffers[VULKAN_MAX_STREAM_OUTPUT_BUFFER_COUNT];
    VkDeviceSize Offsets[VULKAN_MAX_STREAM_OUTPUT_BUFFER_COUNT];
    VkDeviceSize Sizes[VULKAN_MAX_STREAM_OUTPUT_BUFFER_COUNT];
    uint32       NumBuffers;
};

struct FVulkanRenderTargetState
{
    FVulkanRenderTargetState()
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(RenderTargetViews, sizeof(RenderTargetViews));

        for (uint32 Index = 0; Index < RHI_MAX_RENDER_TARGETS; Index++)
        {
            ColorStoreActions[Index] = EAttachmentStoreAction::Store;
        }

        DepthStencilView        = nullptr;
        DepthStencilStoreAction = EAttachmentStoreAction::Store;
        Framebuffer             = VK_NULL_HANDLE;
        NumRenderTargets        = 0;
        RenderAreaWidth         = 0;
        RenderAreaHeight        = 0;
        RenderingLayerCount     = 0;
        RenderingViewMask       = 0;
    }

    FVulkanResourceView*   RenderTargetViews[RHI_MAX_RENDER_TARGETS];
    FVulkanResourceView*   DepthStencilView;
    VkFramebuffer          Framebuffer;
    EAttachmentStoreAction ColorStoreActions[RHI_MAX_RENDER_TARGETS];
    EAttachmentStoreAction DepthStencilStoreAction;
    uint32                 NumRenderTargets;
    uint32                 RenderAreaWidth;
    uint32                 RenderAreaHeight;
    uint32                 RenderingLayerCount;
    uint32                 RenderingViewMask;
};

class FVulkanCommandContextState : public FVulkanDeviceChild, public FNonCopyAndNonMovable
{
public:
    FVulkanCommandContextState(FVulkanDevice* InDevice, FVulkanCommandContext& InContext);
    ~FVulkanCommandContextState();

    bool Initialize();
    
    void PrepareGraphicsState();
    void PrepareComputeState();

    void BindGraphicsState();
    void BindComputeState();
    void BindPushConstants(FVulkanPipelineLayout* PipelineLayout);

    void ResetState();
    void ResetStateForNewCommandBuffer();

    void EvictStaleDescriptorStates();

    void BeginRenderPass(const FRHIBeginRenderPassInfo& RenderPassInfo);
    void EndRenderPass();
    void PauseRenderPass();
    void ResumeRenderPass();

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

    FORCEINLINE void OnStartRecording()
    {
        CHECK(ContextPhase == ECommandContextPhase::Finished);
        ContextPhase = ECommandContextPhase::Recording;
    }

    FORCEINLINE void OnFinishRecording()
    {
        CHECK(ContextPhase == ECommandContextPhase::Recording);
        ContextPhase = ECommandContextPhase::Finished;
    }

    FORCEINLINE bool IsRecording()        const { return ContextPhase >= ECommandContextPhase::Recording; }
    FORCEINLINE bool IsFinished()         const { return ContextPhase == ECommandContextPhase::Finished; }
    FORCEINLINE bool IsInsideRenderPass() const { return ContextPhase == ECommandContextPhase::InsideRenderPass; }
    FORCEINLINE bool IsRenderPassPaused() const { return ContextPhase == ECommandContextPhase::RenderPassPaused; }

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
    FVulkanRenderPassKey BuildRenderPassKey(const FVulkanRenderTargetState& RenderTargetState) const;

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
            , StencilRef(0)
        {
            FMemory::Memzero(BlendFactor, sizeof(BlendFactor));
            FMemory::Memzero(DepthBias, sizeof(DepthBias));
            FMemory::Memzero(Viewports, sizeof(Viewports));
            FMemory::Memzero(ScissorRects, sizeof(ScissorRects));
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
        FVulkanStreamOutputCache        StreamOutputCache;
        FVulkanRenderTargetState        RenderTargetState;

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
    ECommandContextPhase   ContextPhase;
};
