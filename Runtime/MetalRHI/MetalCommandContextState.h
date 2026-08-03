#pragma once
#include "Core/Memory/Memory.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "MetalRHI/MetalCore.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalShader.h"
#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/MetalSamplerState.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalCommandContext;

enum class EMetalDescriptorState : uint8
{
    None           = 0,
    ResourcesDirty = (1 << 0),
};
ENUM_CLASS_OPERATORS(EMetalDescriptorState);

struct FMetalResourceCache
{
    FMetalResourceCache()
    {
        ClearAll();
    }

    bool IsResourcesDirty(EShaderVisibility::Type ShaderStage) const
    {
        return IsEnumFlagSet(DescriptorState[ShaderStage], EMetalDescriptorState::ResourcesDirty);
    }

    void DirtyResources(EShaderVisibility::Type ShaderStage)
    {
        DescriptorState[ShaderStage] |= EMetalDescriptorState::ResourcesDirty;
    }

    void DirtyResourcesAll()
    {
        for (uint32 Index = 0; Index < EShaderVisibility::Count; Index++)
        {
            DescriptorState[Index] |= EMetalDescriptorState::ResourcesDirty;
        }
    }

    void ClearResourcesDirty(EShaderVisibility::Type ShaderStage)
    {
        DescriptorState[ShaderStage] &= ~EMetalDescriptorState::ResourcesDirty;
    }

    void ClearAll()
    {
        for (uint32 Index = 0; Index < EShaderVisibility::Count; Index++)
        {
            DescriptorState[Index] = EMetalDescriptorState::None;
        }
    }

    EMetalDescriptorState DescriptorState[EShaderVisibility::Count];
};

struct FMetalVertexBufferCache
{
    FMetalVertexBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        for (uint32 Index = 0; Index < RHI_MAX_VERTEX_BUFFERS; Index++)
        {
            VertexBuffers[Index] = nil;
            Offsets[Index]       = 0;
        }

        DirtyRange       = NSMakeRange(0, 0);
        NumVertexBuffers = 0;
    }

    id<MTLBuffer> VertexBuffers[RHI_MAX_VERTEX_BUFFERS];
    NSUInteger    Offsets[RHI_MAX_VERTEX_BUFFERS];
    NSRange       DirtyRange;
    uint32        NumVertexBuffers;
};

struct FMetalIndexBufferCache
{
    FMetalIndexBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        IndexBuffer    = nil;
        BufferResource = nullptr;
        Offset         = 0;
        IndexType      = MTLIndexTypeUInt32;
    }

    id<MTLBuffer>    IndexBuffer;
    FMetalBufferRHI* BufferResource;
    NSUInteger       Offset;
    MTLIndexType     IndexType;
};

struct FMetalRenderTargetCache
{
    FMetalRenderTargetCache()
    {
        Clear();
    }

    void Clear()
    {
        for (uint32 Index = 0; Index < RHI_MAX_RENDER_TARGETS; Index++)
        {
            RenderTargetViews[Index] = nullptr;
        }

        DepthStencilView = nullptr;
        NumRenderTargets = 0;
    }

    FMetalRenderTargetViewRHI* RenderTargetViews[RHI_MAX_RENDER_TARGETS];
    FMetalDepthStencilViewRHI* DepthStencilView;
    uint32                     NumRenderTargets;
};

struct FMetalConstantBufferCache : public FMetalResourceCache
{
    FMetalConstantBufferCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();

        for (uint32 Stage = 0; Stage < EShaderVisibility::Count; Stage++)
        {
            for (uint32 Index = 0; Index < kMaxConstantBuffers; Index++)
            {
                ConstantBuffers[Stage][Index] = nullptr;
            }

            NumBuffers[Stage] = 0;
        }
    }

    FMetalBufferRHI* ConstantBuffers[EShaderVisibility::Count][kMaxConstantBuffers];
    uint8            NumBuffers[EShaderVisibility::Count];
};

struct FMetalShaderResourceViewCache : public FMetalResourceCache
{
    FMetalShaderResourceViewCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();

        for (uint32 Stage = 0; Stage < EShaderVisibility::Count; Stage++)
        {
            for (uint32 Index = 0; Index < kMaxSRVs; Index++)
            {
                ResourceViews[Stage][Index] = nullptr;
            }

            NumViews[Stage] = 0;
        }
    }

    FMetalShaderResourceViewRHI* ResourceViews[EShaderVisibility::Count][kMaxSRVs];
    uint8                        NumViews[EShaderVisibility::Count];
};

struct FMetalUnorderedAccessViewCache : public FMetalResourceCache
{
    FMetalUnorderedAccessViewCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();

        for (uint32 Stage = 0; Stage < EShaderVisibility::Count; Stage++)
        {
            for (uint32 Index = 0; Index < kMaxUAVs; Index++)
            {
                ResourceViews[Stage][Index] = nullptr;
            }

            NumViews[Stage] = 0;
        }
    }

    FMetalUnorderedAccessViewRHI* ResourceViews[EShaderVisibility::Count][kMaxUAVs];
    uint8                         NumViews[EShaderVisibility::Count];
};

struct FMetalSamplerStateCache : public FMetalResourceCache
{
    FMetalSamplerStateCache()
    {
        Clear();
    }

    void Clear()
    {
        DirtyResourcesAll();

        for (uint32 Stage = 0; Stage < EShaderVisibility::Count; Stage++)
        {
            for (uint32 Index = 0; Index < kMaxSamplerStates; Index++)
            {
                SamplerStates[Stage][Index] = nullptr;
            }

            NumSamplers[Stage] = 0;
        }
    }

    FMetalSamplerStateRHI* SamplerStates[EShaderVisibility::Count][kMaxSamplerStates];
    uint8                  NumSamplers[EShaderVisibility::Count];
};

struct FMetalShaderConstantsCache
{
    FMetalShaderConstantsCache()
    {
        Clear();
    }

    void Clear()
    {
        for (uint32 Stage = 0; Stage < EShaderVisibility::Count; Stage++)
        {
            Memory::Memzero(Constants[Stage], sizeof(Constants[Stage]));
            NumConstants[Stage] = 0;
        }
    }

    uint32 Constants[EShaderVisibility::Count][kMaxShaderConstants];
    uint32 NumConstants[EShaderVisibility::Count];
};

class FMetalCommandContextState : public FMetalDeviceChild
{
public:
    FMetalCommandContextState(FMetalDevice* InDevice, FMetalCommandContext& InContext);
    ~FMetalCommandContextState() = default;

    bool Initialize();

    void PrepareGraphicsState();
    void PrepareComputeState();

    void BindGraphicsState();
    void BindComputeState();

    void BindShaderConstants(EShaderVisibility::Type ShaderStage);

    void ResetState();
    void ResetStateResources();

    void BeginCommandBuffer();
    void EndCommandBuffer() {}

    void SetGraphicsPipelineState(FMetalGraphicsPipelineStateRHI* InGraphicsPipelineState);
    void SetComputePipelineState(FMetalComputePipelineStateRHI* InComputePipelineState);
    void SetRenderTargets(FMetalRenderTargetViewRHI* const* RenderTargets, uint32 NumRenderTargets, FMetalDepthStencilViewRHI* DepthStencil);
    void SetViewports(const MTLViewport* Viewports, uint32 NumViewports);
    void SetScissorRects(const MTLScissorRect* ScissorRects, uint32 NumScissorRects);
    void SetBlendFactor(const float BlendFactor[4]);
    void SetStencilRef(uint32 InStencilRef);
    void SetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias);
    void SetVertexBuffer(FMetalBufferRHI* VertexBuffer, uint32 VertexBufferSlot);
    void SetIndexBuffer(FMetalBufferRHI* IndexBuffer, MTLIndexType IndexType);
    void SetPrimitiveType(MTLPrimitiveType PrimitiveType);
    void SetSRV(FMetalShaderResourceViewRHI* ShaderResourceView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetUAV(FMetalUnorderedAccessViewRHI* UnorderedAccessView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetCBV(FMetalBufferRHI* Buffer, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex);
    void SetSampler(FMetalSamplerStateRHI* SamplerState, EShaderVisibility::Type ShaderStage, uint32 SamplerIndex);
    void SetShaderConstants(EShaderVisibility::Type ShaderStage, const uint32* ShaderConstants, uint32 NumShaderConstants);

    FORCEINLINE FMetalCommandContext& GetContext()
    {
        return Context;
    }

    FORCEINLINE FMetalGraphicsPipelineStateRHI* GetGraphicsPipelineState() const
    {
        return GraphicsState.PipelineState.Get();
    }

    FORCEINLINE FMetalComputePipelineStateRHI* GetComputePipelineState() const
    {
        return ComputeState.PipelineState.Get();
    }

    FORCEINLINE MTLPrimitiveType GetPrimitiveType() const
    {
        return GraphicsState.PrimitiveType;
    }

    FORCEINLINE const FMetalIndexBufferCache& GetIndexBufferCache() const
    {
        return GraphicsState.IndexBufferCache;
    }

    FORCEINLINE void GetRenderTargets(FMetalRenderTargetViewRHI** RenderTargetViews, uint32& OutNumRenderTargets, FMetalDepthStencilViewRHI** DepthStencilView) const
    {
        const uint32 CurrentNumRenderTargets = GraphicsState.RenderTargetCache.NumRenderTargets;
        if (RenderTargetViews)
        {
            Memory::Memcpy(RenderTargetViews, GraphicsState.RenderTargetCache.RenderTargetViews, sizeof(FMetalRenderTargetViewRHI*) * CurrentNumRenderTargets);
        }

        OutNumRenderTargets = CurrentNumRenderTargets;

        if (DepthStencilView)
        {
            *DepthStencilView = GraphicsState.RenderTargetCache.DepthStencilView;
        }
    }

    FORCEINLINE void GetViewports(MTLViewport* Viewports, uint32& OutNumViewports) const
    {
        if (Viewports)
        {
            Memory::Memcpy(Viewports, GraphicsState.Viewports, sizeof(MTLViewport) * GraphicsState.NumViewports);
        }

        OutNumViewports = GraphicsState.NumViewports;
    }

    FORCEINLINE void GetScissorRects(MTLScissorRect* ScissorRects, uint32& OutNumScissorRects) const
    {
        if (ScissorRects)
        {
            Memory::Memcpy(ScissorRects, GraphicsState.ScissorRects, sizeof(MTLScissorRect) * GraphicsState.NumScissorRects);
        }

        OutNumScissorRects = GraphicsState.NumScissorRects;
    }

    FORCEINLINE void GetBlendFactor(float* BlendFactor) const
    {
        if (BlendFactor)
        {
            Memory::Memcpy(BlendFactor, GraphicsState.BlendFactor, sizeof(GraphicsState.BlendFactor));
        }
    }

private:
    void BindGraphicsResources(EShaderVisibility::Type ShaderStage);
    void BindGraphicsSamplers(EShaderVisibility::Type ShaderStage);
    void BindGraphicsShaderConstants(EShaderVisibility::Type ShaderStage);

    void BindComputeResources();
    void BindComputeSamplers();
    void BindComputeShaderConstants();

    FMetalCommandContext& Context;

    struct FGraphicsState
    {
        FGraphicsState()
            : PipelineState(nullptr)
            , PrimitiveType(MTLPrimitiveType(-1))
            , StencilRef(0)
            , NumViewports(0)
            , NumScissorRects(0)
            , RenderTargetCache()
            , IndexBufferCache()
            , VertexBufferCache()
        {
            Memory::Memzero(BlendFactor, sizeof(BlendFactor));
            Memory::Memzero(DepthBias, sizeof(DepthBias));
            Memory::Memzero(Viewports, sizeof(Viewports));
            Memory::Memzero(ScissorRects, sizeof(ScissorRects));

            bBindPipelineState       = false;
            bBindPrimitiveTopology   = false;
            bBindRenderTargets       = false;
            bBindViewports           = false;
            bBindScissorRects        = false;
            bBindBlendFactor         = false;
            bBindStencilRef          = false;
            bBindDepthBias           = false;
            bBindVertexBuffers       = false;
            bBindIndexBuffer         = false;
            bBindShaderConstants     = false;
        }

        FMetalGraphicsPipelineStateRef PipelineState;
        MTLPrimitiveType               PrimitiveType;
        float                          BlendFactor[4];
        uint32                         StencilRef;
        float                          DepthBias[3];
        MTLViewport                    Viewports[kMaxViewports];
        uint32                         NumViewports;
        MTLScissorRect                 ScissorRects[kMaxViewports];
        uint32                         NumScissorRects;
        FMetalRenderTargetCache        RenderTargetCache;
        FMetalIndexBufferCache         IndexBufferCache;
        FMetalVertexBufferCache        VertexBufferCache;

        bool bBindPipelineState     : 1;
        bool bBindPrimitiveTopology : 1;
        bool bBindRenderTargets     : 1;
        bool bBindViewports         : 1;
        bool bBindScissorRects      : 1;
        bool bBindBlendFactor       : 1;
        bool bBindStencilRef        : 1;
        bool bBindDepthBias         : 1;
        bool bBindVertexBuffers     : 1;
        bool bBindIndexBuffer       : 1;
        bool bBindShaderConstants   : 1;
    } GraphicsState;

    struct FComputeState
    {
        FComputeState()
            : PipelineState(nullptr)
        {
            bBindPipelineState   = false;
            bBindShaderConstants = false;
        }

        FMetalComputePipelineStateRef PipelineState;

        bool bBindPipelineState   : 1;
        bool bBindShaderConstants : 1;
    } ComputeState;

    struct FCommonState
    {
        FMetalConstantBufferCache      ConstantBufferCache;
        FMetalShaderResourceViewCache  ShaderResourceViewCache;
        FMetalUnorderedAccessViewCache UnorderedAccessViewCache;
        FMetalSamplerStateCache        SamplerStateCache;
        FMetalShaderConstantsCache     ShaderConstantsCache;
    } CommonState;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
