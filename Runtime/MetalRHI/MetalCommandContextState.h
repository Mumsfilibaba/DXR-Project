#pragma once
#include "Core/Memory/Memory.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "MetalRHI/MetalCore.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalViews.h"
#include "MetalRHI/MetalSamplerState.h"
#include "MetalRHI/MetalStageEncoder.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalEncoderManager;
struct FMetalDefaultResources;

static_assert(MAX_CONSTANT_BUFFERS <= 16 && MAX_SRVS <= 16 && MAX_UAVS <= 16 && MAX_SAMPLER_STATES <= 16, "Dirty masks are 16 bits wide");

struct FMetalStageResourceTable
{
    void Reset();
    void MarkAllDirty();

    FMetalBufferRHI*              ConstantBuffers[MAX_CONSTANT_BUFFERS];
    FMetalShaderResourceViewRHI*  ShaderResourceViews[MAX_SRVS];
    FMetalUnorderedAccessViewRHI* UnorderedAccessViews[MAX_UAVS];
    FMetalSamplerStateRHI*        SamplerStates[MAX_SAMPLER_STATES];
    FMetalBufferRHI*              ShaderResourceViewSources[MAX_SRVS];
    FMetalBufferRHI*              UnorderedAccessViewSources[MAX_UAVS];
    uint16                        DirtyConstantBuffers;
    uint16                        DirtyShaderResourceViews;
    uint16                        DirtyUnorderedAccessViews;
    uint16                        DirtySamplerStates;
    bool                          bDirtyShaderConstants;
};

struct FMetalShaderConstantsBlock
{
    uint32 Constants[MAX_SHADER_CONSTANTS];
    uint32 NumConstants;
};

struct FMetalVertexBufferCache
{
    void Clear()
    {
        Memory::Memzero(VertexBuffers, sizeof(VertexBuffers));
    }

    FMetalBufferRHI* VertexBuffers[MSL_MAX_VERTEX_STREAMS];
};

struct FMetalIndexBufferCache
{
    FMetalBufferRHI* IndexBuffer = nullptr;
    MTLIndexType     IndexType   = MTLIndexTypeUInt32;
};

struct FMetalRenderPassInfo
{
    MTLSize    Extent           = MTLSizeMake(0, 0, 0);
    NSUInteger ArrayLength      = 1;
    uint8      NumRenderTargets = 0;
    bool       bHasDepthStencil = false;
};

enum class EMetalDynamicState : uint8
{
    None         = 0,
    Viewports    = FLAG(0),
    ScissorRects = FLAG(1),
    BlendFactor  = FLAG(2),
    StencilRef   = FLAG(3),
    DepthBias    = FLAG(4),
    All          = Viewports | ScissorRects | BlendFactor | StencilRef | DepthBias,
};
ENUM_CLASS_OPERATORS(EMetalDynamicState);

class FMetalCommandContextState : public FMetalDeviceChild
{
public:
    FMetalCommandContextState(FMetalDevice* InDevice, FMetalEncoderManager& InEncoders);
    ~FMetalCommandContextState();

    template<EMetalRenderPipelineType Type>
    void BindRenderState(id<MTLRenderCommandEncoder> Encoder);
    void BindComputeState(id<MTLComputeCommandEncoder> Encoder);

    void ResetState();
    void BeginCommandBuffer();
    void EndCommandBuffer();
    void InvalidateRenderEncoderState();
    void InvalidateComputeEncoderState();

    template<typename PipelineStateType>
    void SetRenderPipelineState(PipelineStateType* PipelineState);
    void SetComputePipelineState(FMetalComputePipelineStateRHI* Pipeline);
    void SetRenderPassInfo(const FMetalRenderPassInfo& InRenderPassInfo);
    void SetViewports(const MTLViewport* InViewports, uint32 InNumViewports);
    void SetScissorRects(const MTLScissorRect* InScissorRects, uint32 InNumScissorRects);
    void SetBlendFactor(const float InBlendFactor[4]);
    void SetStencilRef(uint32 InStencilRef);
    void SetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias);
    void SetVertexBuffer(FMetalBufferRHI* VertexBuffer, uint32 Slot);
    void SetIndexBuffer(FMetalBufferRHI* InIndexBuffer, MTLIndexType IndexType);
    void SetSamplePositions(const FRHISamplePositionsDesc& InSamplePositions);

    void SetCBV(FMetalBufferRHI* Buffer, EShaderVisibility::Type Stage, uint32 Register);
    void OnBufferRelocated(FMetalBufferRHI* Buffer);
    void SetSRV(FMetalShaderResourceViewRHI* View, EShaderVisibility::Type Stage, uint32 Register);
    void SetUAV(FMetalUnorderedAccessViewRHI* View, EShaderVisibility::Type Stage, uint32 Register);
    void SetSampler(FMetalSamplerStateRHI* Sampler, EShaderVisibility::Type Stage, uint32 Register);
    void SetShaderConstants(EShaderVisibility::Type Stage, const uint32* Constants, uint32 NumConstants);

    const FMetalRenderPipeline*    GetRenderPipeline() const      { return RenderPipeline; }
    FRHIPipelineState*             GetRenderPipelineState() const { return RenderPipelineOwner.Get(); }
    FMetalComputePipelineStateRHI* GetComputePipeline() const { return ComputePipeline.Get(); }
    const FMetalIndexBufferCache&  GetIndexBuffer() const     { return IndexBuffer; }
    const FRHISamplePositionsDesc& GetSamplePositions() const { return SamplePositions; }

private:
    template<EShaderVisibility::Type Stage>
    void FlushStage(typename TMetalStageEncoder<Stage>::EncoderType Encoder, const FMetalStageBindPlan& Plan);
    void FlushDynamicState(id<MTLRenderCommandEncoder> Encoder);
    void ApplyStaticSamplers(const TArray<FMetalStaticSamplerBinding>& StaticSamplers);

    FMetalEncoderManager&         Encoders;
    const FMetalDefaultResources* DefaultResources;
    FMetalStageResourceTable      StageTables[EShaderVisibility::Count];
    FMetalShaderConstantsBlock    GraphicsConstants;
    FMetalShaderConstantsBlock    ComputeConstants;
    TSharedRef<FRHIPipelineState> RenderPipelineOwner;
    const FMetalRenderPipeline*   RenderPipeline;
    const FMetalRenderPipeline*   AppliedRenderPipeline;
    FMetalComputePipelineStateRef ComputePipeline;
    bool                          bComputePipelineDirty;
    uint64                        RenderEncoderSerial;
    uint64                        ComputeEncoderSerial;
    FMetalRenderPassInfo          RenderPassInfo;
    FMetalVertexBufferCache       VertexBuffers;
    FMetalIndexBufferCache        IndexBuffer;
    FRHISamplePositionsDesc       SamplePositions;
    MTLViewport                   Viewports[MAX_VIEWPORTS];
    MTLScissorRect                ScissorRects[MAX_VIEWPORTS];
    float                         BlendFactor[4];
    float                         DepthBias[3];
    uint32                        StencilRef;
    uint8                         NumViewports;
    uint8                         NumScissorRects;
    EMetalDynamicState            DirtyDynamicState;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
