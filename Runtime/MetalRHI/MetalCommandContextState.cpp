#include "MetalRHI/MetalCommandContextState.h"
#include "MetalRHI/MetalBindlessDescriptors.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalEncoderManager.h"
#include "Core/Math/Math.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

static constexpr uint8 InvalidMSLSlot = FMetalPipelineBindingLayout::InvalidSlot;

void FMetalStageResourceTable::Reset()
{
    Memory::Memzero(ConstantBuffers, sizeof(ConstantBuffers));
    Memory::Memzero(ShaderResourceViews, sizeof(ShaderResourceViews));
    Memory::Memzero(UnorderedAccessViews, sizeof(UnorderedAccessViews));
    Memory::Memzero(SamplerStates, sizeof(SamplerStates));
    Memory::Memzero(ShaderResourceViewSources, sizeof(ShaderResourceViewSources));
    Memory::Memzero(UnorderedAccessViewSources, sizeof(UnorderedAccessViewSources));

    MarkAllDirty();
}

void FMetalStageResourceTable::MarkAllDirty()
{
    DirtyConstantBuffers      = UINT16_MAX;
    DirtyShaderResourceViews  = UINT16_MAX;
    DirtyUnorderedAccessViews = UINT16_MAX;
    DirtySamplerStates        = UINT16_MAX;
    bDirtyShaderConstants     = true;
}

FMetalCommandContextState::FMetalCommandContextState(FMetalDevice* InDevice, FMetalEncoderManager& InEncoders)
    : FMetalDeviceChild(InDevice)
    , Encoders(InEncoders)
    , DefaultResources(&InDevice->GetDefaultResources())
    , RenderPipelineOwner(nullptr)
    , RenderPipeline(nullptr)
    , AppliedRenderPipeline(nullptr)
    , ComputePipeline(nullptr)
    , bComputePipelineDirty(false)
    , RenderEncoderSerial(0)
    , ComputeEncoderSerial(0)
{
    ResetState();
}

FMetalCommandContextState::~FMetalCommandContextState() = default;

void FMetalCommandContextState::ResetState()
{
    for (FMetalStageResourceTable& Table : StageTables)
    {
        Table.Reset();
    }

    Memory::Memzero(&GraphicsConstants, sizeof(GraphicsConstants));
    Memory::Memzero(&ComputeConstants, sizeof(ComputeConstants));

    RenderPipelineOwner   = nullptr;
    RenderPipeline        = nullptr;
    AppliedRenderPipeline = nullptr;
    ComputePipeline       = nullptr;
    bComputePipelineDirty = false;

    RenderPassInfo  = FMetalRenderPassInfo();
    IndexBuffer     = FMetalIndexBufferCache();
    SamplePositions = FRHISamplePositionsDesc();

    VertexBuffers.Clear();

    Memory::Memzero(Viewports, sizeof(Viewports));
    Memory::Memzero(ScissorRects, sizeof(ScissorRects));
    Memory::Memzero(BlendFactor, sizeof(BlendFactor));
    Memory::Memzero(DepthBias, sizeof(DepthBias));

    StencilRef        = 0;
    NumViewports      = 0;
    NumScissorRects   = 0;
    DirtyDynamicState = EMetalDynamicState::All;
}

void FMetalCommandContextState::BeginCommandBuffer()
{
    InvalidateRenderEncoderState();
    InvalidateComputeEncoderState();
}

void FMetalCommandContextState::EndCommandBuffer()
{
    if (FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager())
    {
        BindlessManager->Flush();
    }
}

void FMetalCommandContextState::InvalidateRenderEncoderState()
{
    for (uint32 Stage = EShaderVisibility::Vertex; Stage < EShaderVisibility::Count; ++Stage)
    {
        StageTables[Stage].MarkAllDirty();
    }

    AppliedRenderPipeline = nullptr;
    DirtyDynamicState     = EMetalDynamicState::All;
    RenderEncoderSerial   = Encoders.GetEncoderSerial();
}

void FMetalCommandContextState::InvalidateComputeEncoderState()
{
    StageTables[EShaderVisibility::Compute].MarkAllDirty();
    bComputePipelineDirty = true;
    ComputeEncoderSerial  = Encoders.GetEncoderSerial();
}

template<EMetalRenderPipelineType Type>
void FMetalCommandContextState::BindRenderState(id<MTLRenderCommandEncoder> Encoder)
{
    CHECK(Encoder != nil);
    CHECK(RenderPipeline != nullptr && RenderPipeline->GetType() == Type);

    if (RenderEncoderSerial != Encoders.GetEncoderSerial())
    {
        InvalidateRenderEncoderState();
    }

    if (RenderPipeline != AppliedRenderPipeline)
    {
        RenderPipeline->Apply(Encoder, AppliedRenderPipeline);
        AppliedRenderPipeline = RenderPipeline;
    }

    if (DirtyDynamicState != EMetalDynamicState::None)
    {
        FlushDynamicState(Encoder);
    }

    const FMetalPipelineBindingLayout& Bindings = RenderPipeline->GetBindings();

    if constexpr (Type == EMetalRenderPipelineType::Graphics)
    {
        FMetalEncoderBindingCache& Cache = Encoders.GetBindingCache();
        const uint32 NumVertexStreams = RenderPipeline->GetNumVertexStreams();
        for (uint32 Stream = 0; Stream < NumVertexStreams; ++Stream)
        {
            FMetalBufferRHI* VertexBuffer = VertexBuffers.VertexBuffers[Stream];

            if (VertexBuffer)
            {
                Encoders.UpdateResidency(VertexBuffer->GetResidencyEntry());
            }

            Cache.SetBuffer<EShaderVisibility::Vertex>(Encoder, VertexBuffer ? VertexBuffer->GetMTLBuffer() : nil, VertexBuffer ? VertexBuffer->GetMetalBindOffset() : 0, GetMSLVertexStreamBufferIndex(Stream));
        }

        FlushStage<EShaderVisibility::Vertex>(Encoder, Bindings.Stages[EShaderVisibility::Vertex]);
    }
    else
    {
        FlushStage<EShaderVisibility::Amplification>(Encoder, Bindings.Stages[EShaderVisibility::Amplification]);
        FlushStage<EShaderVisibility::Mesh>(Encoder, Bindings.Stages[EShaderVisibility::Mesh]);
    }

    FlushStage<EShaderVisibility::Pixel>(Encoder, Bindings.Stages[EShaderVisibility::Pixel]);
}

template void FMetalCommandContextState::BindRenderState<EMetalRenderPipelineType::Graphics>(id<MTLRenderCommandEncoder>);
template void FMetalCommandContextState::BindRenderState<EMetalRenderPipelineType::Meshlet>(id<MTLRenderCommandEncoder>);

void FMetalCommandContextState::BindComputeState(id<MTLComputeCommandEncoder> Encoder)
{
    CHECK(Encoder != nil);
    CHECK(ComputePipeline != nullptr);

    if (ComputeEncoderSerial != Encoders.GetEncoderSerial())
    {
        InvalidateComputeEncoderState();
    }

    if (bComputePipelineDirty)
    {
        [Encoder setComputePipelineState:ComputePipeline->GetMTLPipelineState()];
        bComputePipelineDirty = false;
    }

    FlushStage<EShaderVisibility::Compute>(Encoder, ComputePipeline->GetBindings().Stages[EShaderVisibility::Compute]);
}

template<typename PipelineStateType>
void FMetalCommandContextState::SetRenderPipelineState(PipelineStateType* PipelineState)
{
    const FMetalRenderPipeline* NewPipeline = PipelineState ? &PipelineState->GetRenderPipeline() : nullptr;

    if (NewPipeline == RenderPipeline)
    {
        return;
    }

    if (NewPipeline)
    {
        const FMetalPipelineBindingLayout& NewBindings = NewPipeline->GetBindings();
        for (uint32 Stage = EShaderVisibility::Vertex; Stage < EShaderVisibility::Count; ++Stage)
        {
            if (!RenderPipeline || !(RenderPipeline->GetBindings().Stages[Stage] == NewBindings.Stages[Stage]))
            {
                StageTables[Stage].MarkAllDirty();
            }
        }

        ApplyStaticSamplers(NewPipeline->GetStaticSamplers());
    }

    RenderPipelineOwner = MakeSharedRef<FRHIPipelineState>(PipelineState);
    RenderPipeline      = NewPipeline;
}

template void FMetalCommandContextState::SetRenderPipelineState(FMetalGraphicsPipelineStateRHI*);
template void FMetalCommandContextState::SetRenderPipelineState(FMetalMeshletPipelineStateRHI*);

void FMetalCommandContextState::SetComputePipelineState(FMetalComputePipelineStateRHI* Pipeline)
{
    if (ComputePipeline.Get() == Pipeline)
    {
        return;
    }

    if (Pipeline)
    {
        const FMetalStageBindPlan& NewPlan = Pipeline->GetBindings().Stages[EShaderVisibility::Compute];

        if (!ComputePipeline || !(ComputePipeline->GetBindings().Stages[EShaderVisibility::Compute] == NewPlan))
        {
            StageTables[EShaderVisibility::Compute].MarkAllDirty();
        }

        ApplyStaticSamplers(Pipeline->GetStaticSamplers());
    }

    ComputePipeline       = MakeSharedRef<FMetalComputePipelineStateRHI>(Pipeline);
    bComputePipelineDirty = true;
}

void FMetalCommandContextState::SetRenderPassInfo(const FMetalRenderPassInfo& InRenderPassInfo)
{
    RenderPassInfo = InRenderPassInfo;
    DirtyDynamicState |= EMetalDynamicState::ScissorRects;
}

void FMetalCommandContextState::SetViewports(const MTLViewport* InViewports, uint32 InNumViewports)
{
    CHECK(InNumViewports <= MAX_VIEWPORTS);

    const uint64 NumBytes = sizeof(MTLViewport) * InNumViewports;

    if (NumViewports == InNumViewports && Memory::Memcmp(Viewports, InViewports, NumBytes) == 0)
    {
        return;
    }

    Memory::Memcpy(Viewports, InViewports, NumBytes);
    NumViewports       = static_cast<uint8>(InNumViewports);
    DirtyDynamicState |= EMetalDynamicState::Viewports;
}

void FMetalCommandContextState::SetScissorRects(const MTLScissorRect* InScissorRects, uint32 InNumScissorRects)
{
    CHECK(InNumScissorRects <= MAX_VIEWPORTS);

    const uint64 NumBytes = sizeof(MTLScissorRect) * InNumScissorRects;

    if (NumScissorRects == InNumScissorRects && Memory::Memcmp(ScissorRects, InScissorRects, NumBytes) == 0)
    {
        return;
    }

    Memory::Memcpy(ScissorRects, InScissorRects, NumBytes);
    NumScissorRects    = static_cast<uint8>(InNumScissorRects);
    DirtyDynamicState |= EMetalDynamicState::ScissorRects;
}

void FMetalCommandContextState::SetBlendFactor(const float InBlendFactor[4])
{
    Memory::Memcpy(BlendFactor, InBlendFactor, sizeof(BlendFactor));
    DirtyDynamicState |= EMetalDynamicState::BlendFactor;
}

void FMetalCommandContextState::SetStencilRef(uint32 InStencilRef)
{
    StencilRef         = InStencilRef;
    DirtyDynamicState |= EMetalDynamicState::StencilRef;
}

void FMetalCommandContextState::SetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias)
{
    DepthBias[0]       = InDepthBias;
    DepthBias[1]       = InDepthBiasClamp;
    DepthBias[2]       = InSlopeScaledDepthBias;
    DirtyDynamicState |= EMetalDynamicState::DepthBias;
}

void FMetalCommandContextState::SetVertexBuffer(FMetalBufferRHI* VertexBuffer, uint32 Slot)
{
    if (Slot >= MSL_MAX_VERTEX_STREAMS)
    {
        METAL_ERROR("Vertex buffer slot %u is outside the %u streams Metal reserves", Slot, MSL_MAX_VERTEX_STREAMS);
        return;
    }

    VertexBuffers.VertexBuffers[Slot] = VertexBuffer;
}

void FMetalCommandContextState::SetIndexBuffer(FMetalBufferRHI* InIndexBuffer, MTLIndexType IndexType)
{
    IndexBuffer.IndexBuffer = InIndexBuffer;
    IndexBuffer.IndexType   = IndexType;
}

void FMetalCommandContextState::SetSamplePositions(const FRHISamplePositionsDesc& InSamplePositions)
{
    SamplePositions = InSamplePositions;
}

void FMetalCommandContextState::SetCBV(FMetalBufferRHI* Buffer, EShaderVisibility::Type Stage, uint32 Register)
{
    CHECK(Register < MAX_CONSTANT_BUFFERS);

    FMetalStageResourceTable& Table = StageTables[Stage];
    Table.ConstantBuffers[Register] = Buffer;
    Table.DirtyConstantBuffers |= static_cast<uint16>(1u << Register);
}

void FMetalCommandContextState::OnBufferRelocated(FMetalBufferRHI* Buffer)
{
    for (FMetalStageResourceTable& Table : StageTables)
    {
        for (uint32 Register = 0; Register < MAX_CONSTANT_BUFFERS; ++Register)
        {
            if (Table.ConstantBuffers[Register] == Buffer)
            {
                Table.DirtyConstantBuffers |= static_cast<uint16>(1u << Register);
            }
        }

        for (uint32 Register = 0; Register < MAX_SRVS; ++Register)
        {
            if (Table.ShaderResourceViewSources[Register] == Buffer)
            {
                Table.DirtyShaderResourceViews |= static_cast<uint16>(1u << Register);
            }
        }

        for (uint32 Register = 0; Register < MAX_UAVS; ++Register)
        {
            if (Table.UnorderedAccessViewSources[Register] == Buffer)
            {
                Table.DirtyUnorderedAccessViews |= static_cast<uint16>(1u << Register);
            }
        }
    }
}

void FMetalCommandContextState::SetSRV(FMetalShaderResourceViewRHI* View, EShaderVisibility::Type Stage, uint32 Register)
{
    CHECK(Register < MAX_SRVS);

    FMetalStageResourceTable& Table = StageTables[Stage];

    if (Table.ShaderResourceViews[Register] != View)
    {
        Table.ShaderResourceViews[Register]       = View;
        Table.ShaderResourceViewSources[Register] = View ? View->GetSourceBuffer() : nullptr;
        Table.DirtyShaderResourceViews |= static_cast<uint16>(1u << Register);
    }
}

void FMetalCommandContextState::SetUAV(FMetalUnorderedAccessViewRHI* View, EShaderVisibility::Type Stage, uint32 Register)
{
    CHECK(Register < MAX_UAVS);

    FMetalStageResourceTable& Table = StageTables[Stage];

    if (Table.UnorderedAccessViews[Register] != View)
    {
        Table.UnorderedAccessViews[Register]       = View;
        Table.UnorderedAccessViewSources[Register] = View ? View->GetSourceBuffer() : nullptr;
        Table.DirtyUnorderedAccessViews |= static_cast<uint16>(1u << Register);
    }
}

void FMetalCommandContextState::SetSampler(FMetalSamplerStateRHI* Sampler, EShaderVisibility::Type Stage, uint32 Register)
{
    CHECK(Register < MAX_SAMPLER_STATES);

    const FMetalPipelineBindingLayout* Bindings = nullptr;

    if (Stage == EShaderVisibility::Compute)
    {
        Bindings = ComputePipeline ? &ComputePipeline->GetBindings() : nullptr;
    }
    else
    {
        Bindings = RenderPipeline ? &RenderPipeline->GetBindings() : nullptr;
    }

    if (Bindings && (Bindings->Stages[Stage].StaticSamplerMask & (1u << Register)))
    {
        return;
    }

    FMetalStageResourceTable& Table = StageTables[Stage];

    if (Table.SamplerStates[Register] != Sampler)
    {
        Table.SamplerStates[Register] = Sampler;
        Table.DirtySamplerStates |= static_cast<uint16>(1u << Register);
    }
}

void FMetalCommandContextState::SetShaderConstants(EShaderVisibility::Type Stage, const uint32* Constants, uint32 NumConstants)
{
    CHECK(NumConstants <= MAX_SHADER_CONSTANTS);

    const bool bIsCompute = (Stage == EShaderVisibility::Compute);

    FMetalShaderConstantsBlock& Block = bIsCompute ? ComputeConstants : GraphicsConstants;

    if (Constants && NumConstants > 0)
    {
        Memory::Memcpy(Block.Constants, Constants, sizeof(uint32) * NumConstants);
    }

    Memory::Memzero(Block.Constants + NumConstants, sizeof(uint32) * (MAX_SHADER_CONSTANTS - NumConstants));
    Block.NumConstants = NumConstants;

    if (bIsCompute)
    {
        StageTables[EShaderVisibility::Compute].bDirtyShaderConstants = true;
    }
    else
    {
        for (uint32 GraphicsStage = EShaderVisibility::Vertex; GraphicsStage < EShaderVisibility::Count; ++GraphicsStage)
        {
            StageTables[GraphicsStage].bDirtyShaderConstants = true;
        }
    }
}

template<EShaderVisibility::Type Stage>
void FMetalCommandContextState::FlushStage(typename TMetalStageEncoder<Stage>::EncoderType Encoder, const FMetalStageBindPlan& Plan)
{
    FMetalStageResourceTable&     Table     = StageTables[Stage];
    FMetalEncoderBindingCache&    Cache    = Encoders.GetBindingCache();
    const FMetalDefaultResources& Defaults = *DefaultResources;

    if (Plan.ResourceHeapSlot != InvalidMSLSlot || Plan.SamplerHeapSlot != InvalidMSLSlot)
    {
        Encoders.RefreshBindlessResidency(Encoder);

        FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();

        if (Plan.ResourceHeapSlot != InvalidMSLSlot)
        {
            Cache.SetBuffer<Stage>(Encoder, BindlessManager->GetResourceHeapBuffer(), 0, Plan.ResourceHeapSlot);
        }

        if (Plan.SamplerHeapSlot != InvalidMSLSlot)
        {
            Cache.SetBuffer<Stage>(Encoder, BindlessManager->GetSamplerHeapBuffer(), 0, Plan.SamplerHeapSlot);
        }
    }

    for (uint32 Mask = Table.DirtyConstantBuffers & Plan.ConstantBufferMask; Mask != 0; Mask &= Mask - 1)
    {
        const uint32     Register = MetalRHI::FirstSetBit(Mask);
        FMetalBufferRHI* Buffer   = Table.ConstantBuffers[Register];

        if (Buffer)
        {
            Encoders.UpdateResidency(Buffer->GetResidencyEntry());
        }

        Cache.SetBuffer<Stage>(Encoder, Buffer ? Buffer->GetMTLBuffer() : Defaults.NullBuffer, Buffer ? Buffer->GetMetalBindOffset() : 0, Plan.ConstantBufferSlots[Register]);
    }

    Table.DirtyConstantBuffers &= ~Plan.ConstantBufferMask;

    for (uint32 Mask = Table.DirtyShaderResourceViews & Plan.ShaderResourceMask; Mask != 0; Mask &= Mask - 1)
    {
        const uint32                 Register = MetalRHI::FirstSetBit(Mask);
        FMetalShaderResourceViewRHI* View     = Table.ShaderResourceViews[Register];
        const uint8                  Slot     = Plan.ShaderResourceSlots[Register];

        if (View)
        {
            Encoders.UpdateResidency(View->GetResidencyEntry());
        }

        if (Plan.ShaderResourceAccelerationStructureMask & (1u << Register))
        {
            if constexpr (TMetalStageAccelerationStructure<Stage>::bSupported)
            {
                Encoders.RefreshBindlessResidency(Encoder);
                Cache.SetAccelerationStructure<Stage>(Encoder, View ? View->GetMTLAccelerationStructure() : nil, Slot);
            }
        }
        else if (Plan.ShaderResourceBufferMask & (1u << Register))
        {
            Cache.SetBuffer<Stage>(Encoder, View ? View->GetMTLBuffer() : Defaults.NullBuffer, View ? View->GetBufferOffset() : 0, Slot);
        }
        else
        {
            Cache.SetTexture<Stage>(View ? View->GetMTLTexture() : Defaults.GetNullTexture(Plan.ShaderResourceNullTypes[Register]), Slot);
        }
    }

    Table.DirtyShaderResourceViews &= ~Plan.ShaderResourceMask;

    for (uint32 Mask = Table.DirtyUnorderedAccessViews & Plan.UnorderedAccessMask; Mask != 0; Mask &= Mask - 1)
    {
        const uint32                  Register = MetalRHI::FirstSetBit(Mask);
        FMetalUnorderedAccessViewRHI* View     = Table.UnorderedAccessViews[Register];
        const uint8                   Slot     = Plan.UnorderedAccessSlots[Register];

        if (View)
        {
            Encoders.UpdateResidency(View->GetResidencyEntry());
        }

        if (Plan.UnorderedAccessBufferMask & (1u << Register))
        {
            Cache.SetBuffer<Stage>(Encoder, View ? View->GetMTLBuffer() : Defaults.NullBuffer, View ? View->GetBufferOffset() : 0, Slot);
        }
        else
        {
            Cache.SetTexture<Stage>(View ? View->GetMTLTexture() : Defaults.GetNullRWTexture(Plan.UnorderedAccessNullTypes[Register]), Slot);
        }
    }

    Table.DirtyUnorderedAccessViews &= ~Plan.UnorderedAccessMask;

    for (uint32 Mask = Table.DirtySamplerStates & Plan.SamplerMask; Mask != 0; Mask &= Mask - 1)
    {
        const uint32           Register = MetalRHI::FirstSetBit(Mask);
        FMetalSamplerStateRHI* Sampler  = Table.SamplerStates[Register];
        Cache.SetSamplerState<Stage>(Sampler ? Sampler->GetMTLSamplerState() : Defaults.DefaultSampler, Plan.SamplerSlots[Register]);
    }

    Table.DirtySamplerStates &= ~Plan.SamplerMask;
    Cache.Commit<Stage>(Encoder);

    if (Table.bDirtyShaderConstants && Plan.ShaderConstantsSlot != InvalidMSLSlot && Plan.NumShaderConstants > 0)
    {
        const FMetalShaderConstantsBlock& Block = (Stage == EShaderVisibility::Compute) ? ComputeConstants : GraphicsConstants;
        Cache.SetBytes<Stage>(Encoder, Block.Constants, sizeof(uint32) * Plan.NumShaderConstants, Plan.ShaderConstantsSlot);
        Table.bDirtyShaderConstants = false;
    }
}

void FMetalCommandContextState::FlushDynamicState(id<MTLRenderCommandEncoder> Encoder)
{
    if (IsEnumFlagSet(DirtyDynamicState, EMetalDynamicState::Viewports) && NumViewports > 0)
    {
        if (NumViewports == 1)
        {
            [Encoder setViewport:Viewports[0]];
        }
        else
        {
            [Encoder setViewports:Viewports count:NumViewports];
        }
    }

    // Metal rejects a scissor outside the attachments, so it is clamped here where the pass extent is known
    if (IsEnumFlagSet(DirtyDynamicState, EMetalDynamicState::ScissorRects) && NumScissorRects > 0)
    {
        const NSUInteger Width  = Math::Max<NSUInteger>(RenderPassInfo.Extent.width, 1);
        const NSUInteger Height = Math::Max<NSUInteger>(RenderPassInfo.Extent.height, 1);

        MTLScissorRect Clamped[MAX_VIEWPORTS];
        for (uint32 Index = 0; Index < NumScissorRects; ++Index)
        {
            const MTLScissorRect& Rect = ScissorRects[Index];
            Clamped[Index].x      = Math::Min(Rect.x, Width);
            Clamped[Index].y      = Math::Min(Rect.y, Height);
            Clamped[Index].width  = Math::Min(Rect.width, Width - Clamped[Index].x);
            Clamped[Index].height = Math::Min(Rect.height, Height - Clamped[Index].y);
        }

        if (NumScissorRects == 1)
        {
            [Encoder setScissorRect:Clamped[0]];
        }
        else
        {
            [Encoder setScissorRects:Clamped count:NumScissorRects];
        }
    }

    if (IsEnumFlagSet(DirtyDynamicState, EMetalDynamicState::BlendFactor))
    {
        [Encoder setBlendColorRed:BlendFactor[0] green:BlendFactor[1] blue:BlendFactor[2] alpha:BlendFactor[3]];
    }

    if (IsEnumFlagSet(DirtyDynamicState, EMetalDynamicState::StencilRef))
    {
        [Encoder setStencilReferenceValue:StencilRef];
    }

    if (IsEnumFlagSet(DirtyDynamicState, EMetalDynamicState::DepthBias))
    {
        [Encoder setDepthBias:DepthBias[0] slopeScale:DepthBias[2] clamp:DepthBias[1]];
    }

    DirtyDynamicState = EMetalDynamicState::None;
}

void FMetalCommandContextState::ApplyStaticSamplers(const TArray<FMetalStaticSamplerBinding>& StaticSamplers)
{
    for (const FMetalStaticSamplerBinding& Binding : StaticSamplers)
    {
        FMetalStageResourceTable& Table   = StageTables[Binding.Stage];
        FMetalSamplerStateRHI*    Sampler = Binding.Sampler.Get();

        if (Table.SamplerStates[Binding.RegisterIndex] != Sampler)
        {
            Table.SamplerStates[Binding.RegisterIndex] = Sampler;
            Table.DirtySamplerStates |= static_cast<uint16>(1u << Binding.RegisterIndex);
        }
    }
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
