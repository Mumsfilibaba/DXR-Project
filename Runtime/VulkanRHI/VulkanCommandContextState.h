#pragma once
#include "RHI/IRHICommandContext.h"
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanPipelineState.h"
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    #include "VulkanRHI/VulkanRenderPass.h"
#endif

class FVulkanCommandContext;
class FVulkanDescriptorState;
class FVulkanResourceView;
struct FRHIBeginRenderPassDesc;

struct FVulkanVertexBufferCache
{
    FVulkanVertexBufferCache() 
    {
        Clear();
    }

    void Clear()
    {
        Memory::Memzero(VertexBuffers, sizeof(VertexBuffers));
        Memory::Memzero(VertexBufferOffsets, sizeof(VertexBufferOffsets));
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
        Memory::Memzero(Constants, sizeof(Constants));
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
        Memory::Memzero(Buffers, sizeof(Buffers));
        Memory::Memzero(Offsets, sizeof(Offsets));
        Memory::Memzero(Sizes, sizeof(Sizes));
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
        Memory::Memzero(RenderTargetViews, sizeof(RenderTargetViews));

        for (uint32 Index = 0; Index < RHI_MAX_RENDER_TARGETS; Index++)
        {
            ColorStoreActions[Index] = EAttachmentStoreAction::Store;
        }

        DepthStencilView        = nullptr;
        DepthStencilStoreAction = EAttachmentStoreAction::Store;
    #if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
        Framebuffer             = VK_NULL_HANDLE;
    #endif
        NumRenderTargets        = 0;
        RenderAreaWidth         = 0;
        RenderAreaHeight        = 0;
        RenderingLayerCount     = 0;
        RenderingViewMask       = 0;
    }

    FVulkanResourceView*   RenderTargetViews[RHI_MAX_RENDER_TARGETS];
    FVulkanResourceView*   DepthStencilView;
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    VkFramebuffer          Framebuffer;
#endif
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
    void PrepareMeshletState();

    void BindGraphicsState();
    void BindComputeState();
    void BindMeshletState();
    void BindPushConstants(FVulkanPipelineLayout* PipelineLayout);

    void ResetState();
    void ResetStateForNewCommandBuffer();

    void EvictStaleDescriptorStates();

    void BeginRenderPass(const FRHIBeginRenderPassDesc& RenderPassDesc);
    void EndRenderPass();
    void PauseRenderPass();
    void ResumeRenderPass();

    void SetGraphicsPipelineState(FVulkanGraphicsPipelineStateRHI* InGraphicsPipelineState);
    void SetComputePipelineState(FVulkanComputePipelineStateRHI* InComputePipelineState);
    void SetMeshletPipelineState(FVulkanMeshletPipelineStateRHI* InMeshletPipelineState);
    void SetViewports(VkViewport* Viewports, uint32 NumViewports);
    void SetScissorRects(VkRect2D* ScissorRects, uint32 NumScissorRects);
    void SetBlendFactor(const float BlendFactor[4]);
    void SetStencilRef(uint32 InStencilRef);
    void SetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias);
    void SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets);
    void SetVertexBuffer(FVulkanBufferRHI* VertexBuffer, uint32 VertexBufferSlot);
    void SetIndexBuffer(FVulkanBufferRHI* IndexBuffer, VkIndexType IndexFormat);
    void SetPushConstants(const uint32* ShaderConstants, uint32 NumShaderConstants);
    void SetSRV(FVulkanShaderResourceViewRHI* ShaderResourceView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetUAV(FVulkanUnorderedAccessViewRHI* UnorderedAccessView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetUniformBuffer(FVulkanBufferRHI* UniformBuffer, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetSampler(FVulkanSamplerStateRHI* SamplerState, EShaderVisibility::Type ShaderStage, uint32 SamplerIndex);

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

    FORCEINLINE FVulkanGraphicsPipelineStateRHI* GetGraphicsPipelineState() const
    {
        return GraphicsState.PipelineState.Get();
    }

    FORCEINLINE FVulkanComputePipelineStateRHI* GetComputePipelineState() const
    {
        return ComputeState.PipelineState.Get();
    }

    FORCEINLINE FVulkanMeshletPipelineStateRHI* GetMeshletPipelineState() const
    {
        return MeshletState.PipelineState.Get();
    }

    FORCEINLINE void GetViewports(VkViewport* Viewports, uint32& OutNumViewports) const
    {
        if (Viewports)
        {
            Memory::Memcpy(Viewports, CommonGraphicsState.Viewports, sizeof(VkViewport) * CommonGraphicsState.NumViewports);
        }

        OutNumViewports = CommonGraphicsState.NumViewports;
    }

    FORCEINLINE void GetScissorRects(VkRect2D* ScissorRects, uint32& OutNumScissorRects) const
    {
        if (ScissorRects)
        {
            Memory::Memcpy(ScissorRects, CommonGraphicsState.ScissorRects, sizeof(VkRect2D) * CommonGraphicsState.NumScissorRects);
        }

        OutNumScissorRects = CommonGraphicsState.NumScissorRects;
    }

    FORCEINLINE void GetBlendFactor(float* BlendFactor) const
    {
        if (BlendFactor)
        {
            Memory::Memcpy(BlendFactor, CommonGraphicsState.BlendFactor, sizeof(CommonGraphicsState.BlendFactor));
        }
    }

private:
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    FVulkanRenderPassKey BuildRenderPassKey(const FVulkanRenderTargetState& RenderTargetState) const;
#endif

    struct FCachedDescriptorState
    {
        FVulkanDescriptorState* State;
        uint64                  LastUsedFrame;
    };

    struct FCommonGraphicsState
    {
        FCommonGraphicsState()
            : ViewInstancingState()
            , StencilRef(0)
            , NumViewports(0)
            , NumScissorRects(0)
            , RenderTargetState()
        {
            Memory::Memzero(BlendFactor, sizeof(BlendFactor));
            Memory::Memzero(DepthBias, sizeof(DepthBias));
            Memory::Memzero(Viewports, sizeof(Viewports));
            Memory::Memzero(ScissorRects, sizeof(ScissorRects));
        }

        FRHIViewInstancingState  ViewInstancingState;
        float                    BlendFactor[4];
        float                    DepthBias[3];
        uint32                   StencilRef;
        VkViewport               Viewports[VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                   NumViewports;
        VkRect2D                 ScissorRects[VULKAN_MAX_VIEWPORT_AND_SCISSORRECT_COUNT];
        uint32                   NumScissorRects;
        FVulkanRenderTargetState RenderTargetState;

        bool bBindBlendFactor  : 1;
        bool bBindStencilRef   : 1;
        bool bBindDepthBias    : 1;
        bool bBindScissorRects : 1;
        bool bBindViewports    : 1;
    } CommonGraphicsState;

    struct FGraphicsState
    {
        typedef TMap<FVulkanGraphicsPipelineStateRHI*, FCachedDescriptorState> FPipelineToDescriptorStateMap;
        
        FGraphicsState()
            : CurrentLayout(nullptr)
            , PipelineState(nullptr)
            , DescriptorStates()
            , CurrentDescriptorState(nullptr)
            , IndexBufferCache()
            , VertexBufferCache()
        {
        }

        FVulkanPipelineLayout*             CurrentLayout;
        FVulkanGraphicsPipelineStateRHIRef PipelineState;
        FPipelineToDescriptorStateMap      DescriptorStates;
        FVulkanDescriptorState*            CurrentDescriptorState;
        FVulkanIndexBufferCache            IndexBufferCache;
        FVulkanVertexBufferCache           VertexBufferCache;
        FVulkanStreamOutputCache           StreamOutputCache;

        bool bBindPipelineState        : 1;
        bool bBindVertexBuffers        : 1;
        bool bBindIndexBuffer          : 1;
        bool bBindPushConstants        : 1;
        bool bBindStreamOutputTargets  : 1;
    } GraphicsState;

    struct FComputeState
    {
        typedef TMap<FVulkanComputePipelineStateRHI*, FCachedDescriptorState> FPipelineToDescriptorStateMap;
        
        FComputeState()
            : CurrentLayout(nullptr)
            , PipelineState(nullptr)
            , DescriptorStates()
            , CurrentDescriptorState(nullptr)
        {
        }

        FVulkanPipelineLayout*            CurrentLayout;
        FVulkanComputePipelineStateRHIRef PipelineState;
        FPipelineToDescriptorStateMap     DescriptorStates;
        FVulkanDescriptorState*           CurrentDescriptorState;
        
        bool bBindPipelineState : 1;
        bool bBindPushConstants : 1;
    } ComputeState;

    struct FMeshletState
    {
        typedef TMap<FVulkanMeshletPipelineStateRHI*, FCachedDescriptorState> FPipelineToDescriptorStateMap;

        FMeshletState()
            : CurrentLayout(nullptr)
            , PipelineState(nullptr)
            , DescriptorStates()
            , CurrentDescriptorState(nullptr)
        {
        }

        FVulkanPipelineLayout*             CurrentLayout;
        FVulkanMeshletPipelineStateRHIRef  PipelineState;
        FPipelineToDescriptorStateMap      DescriptorStates;
        FVulkanDescriptorState*            CurrentDescriptorState;

        bool bBindPipelineState : 1;
        bool bBindPushConstants : 1;
    } MeshletState;

    struct FCommonState
    {
        FVulkanPushConstantsCache PushConstantsCache;
    } CommonState;
    
    FVulkanCommandContext& Context;
    uint64                 CurrentFrame;
    ECommandContextPhase   ContextPhase;
    bool                   bMeshletPipelineActive;
};
