#include "MetalRHI/MetalCommandContextState.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalTexture.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

static constexpr uint8 InvalidMSLSlot = FMetalPipelineBindingLayout::InvalidSlot;

FMetalCommandContextState::FMetalCommandContextState(FMetalDevice* InDevice, FMetalCommandContext& InContext)
    : FMetalDeviceChild(InDevice)
    , Context(InContext)
    , GraphicsState()
    , ComputeState()
    , CommonState()
{
}

bool FMetalCommandContextState::Initialize()
{
    ResetState();
    return true;
}

void FMetalCommandContextState::ResetState()
{
    GraphicsState = FGraphicsState();
    ComputeState  = FComputeState();

    CommonState.ConstantBufferCache.Clear();
    CommonState.ShaderResourceViewCache.Clear();
    CommonState.UnorderedAccessViewCache.Clear();
    CommonState.SamplerStateCache.Clear();
    CommonState.ShaderConstantsCache.Clear();
}

void FMetalCommandContextState::ResetStateResources()
{
    CommonState.ConstantBufferCache.Clear();
    CommonState.ShaderResourceViewCache.Clear();
    CommonState.UnorderedAccessViewCache.Clear();
    CommonState.SamplerStateCache.Clear();
    CommonState.ShaderConstantsCache.Clear();
}

void FMetalCommandContextState::BeginCommandBuffer()
{
    GraphicsState.bBindPipelineState     = GraphicsState.PipelineState != nullptr || GraphicsState.MeshletPipelineState != nullptr;
    GraphicsState.bBindPrimitiveTopology = GraphicsState.PrimitiveType != MTLPrimitiveType(-1);
    GraphicsState.bBindRenderTargets     = GraphicsState.RenderTargetCache.NumRenderTargets > 0 || GraphicsState.RenderTargetCache.DepthStencilView != nullptr;
    GraphicsState.bBindViewports         = GraphicsState.NumViewports > 0;
    GraphicsState.bBindScissorRects      = GraphicsState.NumScissorRects > 0;
    GraphicsState.bBindBlendFactor       = true;
    GraphicsState.bBindStencilRef        = true;
    GraphicsState.bBindDepthBias         = true;
    GraphicsState.bBindVertexBuffers     = GraphicsState.VertexBufferCache.NumVertexBuffers > 0;
    GraphicsState.bBindIndexBuffer       = GraphicsState.IndexBufferCache.IndexBuffer != nil;
    GraphicsState.bBindShaderConstants   = true;

    ComputeState.bBindPipelineState   = ComputeState.PipelineState != nullptr;
    ComputeState.bBindShaderConstants = true;

    CommonState.ConstantBufferCache.DirtyResourcesAll();
    CommonState.ShaderResourceViewCache.DirtyResourcesAll();
    CommonState.UnorderedAccessViewCache.DirtyResourcesAll();
    CommonState.SamplerStateCache.DirtyResourcesAll();
}

void FMetalCommandContextState::SetGraphicsPipelineState(FMetalGraphicsPipelineStateRHI* InGraphicsPipelineState)
{
    if (GraphicsState.PipelineState.Get() == InGraphicsPipelineState)
    {
        return;
    }

    GraphicsState.PipelineState          = MakeSharedRef<FMetalGraphicsPipelineStateRHI>(InGraphicsPipelineState);
    GraphicsState.MeshletPipelineState   = nullptr;
    GraphicsState.PrimitiveType          = InGraphicsPipelineState ? InGraphicsPipelineState->GetMTLPrimitiveType() : MTLPrimitiveType(-1);
    GraphicsState.bBindPipelineState     = true;
    GraphicsState.bBindPrimitiveTopology = true;

    CommonState.ConstantBufferCache.DirtyResourcesAll();
    CommonState.ShaderResourceViewCache.DirtyResourcesAll();
    CommonState.UnorderedAccessViewCache.DirtyResourcesAll();
    CommonState.SamplerStateCache.DirtyResourcesAll();

    if (InGraphicsPipelineState)
    {
        InGraphicsPipelineState->ApplyStaticSamplers(CommonState.SamplerStateCache);
    }
}

void FMetalCommandContextState::SetComputePipelineState(FMetalComputePipelineStateRHI* InComputePipelineState)
{
    if (ComputeState.PipelineState.Get() == InComputePipelineState)
    {
        return;
    }

    ComputeState.PipelineState      = MakeSharedRef<FMetalComputePipelineStateRHI>(InComputePipelineState);
    ComputeState.bBindPipelineState = true;

    CommonState.ConstantBufferCache.DirtyResources(EShaderVisibility::Compute);
    CommonState.ShaderResourceViewCache.DirtyResources(EShaderVisibility::Compute);
    CommonState.UnorderedAccessViewCache.DirtyResources(EShaderVisibility::Compute);
    CommonState.SamplerStateCache.DirtyResources(EShaderVisibility::Compute);

    if (InComputePipelineState)
    {
        InComputePipelineState->ApplyStaticSamplers(CommonState.SamplerStateCache);
    }
}

void FMetalCommandContextState::SetMeshletPipelineState(FMetalMeshletPipelineStateRHI* InMeshletPipelineState)
{
    if (GraphicsState.MeshletPipelineState.Get() == InMeshletPipelineState)
    {
        return;
    }

    GraphicsState.MeshletPipelineState   = MakeSharedRef<FMetalMeshletPipelineStateRHI>(InMeshletPipelineState);
    GraphicsState.PipelineState          = nullptr;
    GraphicsState.bBindPipelineState     = true;
    GraphicsState.bBindPrimitiveTopology = false;

    CommonState.ConstantBufferCache.DirtyResourcesAll();
    CommonState.ShaderResourceViewCache.DirtyResourcesAll();
    CommonState.UnorderedAccessViewCache.DirtyResourcesAll();
    CommonState.SamplerStateCache.DirtyResourcesAll();

    if (InMeshletPipelineState)
    {
        InMeshletPipelineState->ApplyStaticSamplers(CommonState.SamplerStateCache);
    }
}

void FMetalCommandContextState::SetRenderTargets(FMetalRenderTargetViewRHI* const* RenderTargets, uint32 NumRenderTargets, FMetalDepthStencilViewRHI* DepthStencil)
{
    CHECK(NumRenderTargets <= RHI_MAX_RENDER_TARGETS);

    FMetalRenderTargetCache& Cache = GraphicsState.RenderTargetCache;
    for (uint32 Index = 0; Index < NumRenderTargets; Index++)
    {
        Cache.RenderTargetViews[Index] = RenderTargets ? RenderTargets[Index] : nullptr;
    }

    for (uint32 Index = NumRenderTargets; Index < RHI_MAX_RENDER_TARGETS; Index++)
    {
        Cache.RenderTargetViews[Index] = nullptr;
    }

    Cache.DepthStencilView = DepthStencil;
    Cache.NumRenderTargets = NumRenderTargets;

    GraphicsState.bBindRenderTargets = true;
}

void FMetalCommandContextState::SetViewports(const MTLViewport* Viewports, uint32 NumViewports)
{
    CHECK(NumViewports <= MAX_VIEWPORTS);

    if (Viewports && NumViewports > 0)
    {
        Memory::Memcpy(GraphicsState.Viewports, Viewports, sizeof(MTLViewport) * NumViewports);
    }

    GraphicsState.NumViewports   = NumViewports;
    GraphicsState.bBindViewports = true;
}

void FMetalCommandContextState::SetScissorRects(const MTLScissorRect* ScissorRects, uint32 NumScissorRects)
{
    CHECK(NumScissorRects <= MAX_VIEWPORTS);

    if (ScissorRects && NumScissorRects > 0)
    {
        Memory::Memcpy(GraphicsState.ScissorRects, ScissorRects, sizeof(MTLScissorRect) * NumScissorRects);
    }

    GraphicsState.NumScissorRects   = NumScissorRects;
    GraphicsState.bBindScissorRects = true;
}

void FMetalCommandContextState::SetBlendFactor(const float BlendFactor[4])
{
    Memory::Memcpy(GraphicsState.BlendFactor, BlendFactor, sizeof(GraphicsState.BlendFactor));
    GraphicsState.bBindBlendFactor = true;
}

void FMetalCommandContextState::SetStencilRef(uint32 InStencilRef)
{
    GraphicsState.StencilRef      = InStencilRef;
    GraphicsState.bBindStencilRef = true;
}

void FMetalCommandContextState::SetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias)
{
    GraphicsState.DepthBias[0]   = InDepthBias;
    GraphicsState.DepthBias[1]   = InDepthBiasClamp;
    GraphicsState.DepthBias[2]   = InSlopeScaledDepthBias;
    GraphicsState.bBindDepthBias = true;
}

void FMetalCommandContextState::SetVertexBuffer(FMetalBufferRHI* VertexBuffer, uint32 VertexBufferSlot)
{
    CHECK(VertexBufferSlot < RHI_MAX_VERTEX_BUFFERS);

    FMetalVertexBufferCache& Cache = GraphicsState.VertexBufferCache;
    Cache.VertexBuffers[VertexBufferSlot] = VertexBuffer ? VertexBuffer->GetMTLBuffer() : nil;
    Cache.Offsets[VertexBufferSlot]       = 0;

    const NSUInteger OldEnd   = Cache.DirtyRange.location + Cache.DirtyRange.length;
    const NSUInteger NewStart = (Cache.DirtyRange.length == 0) ? VertexBufferSlot : Math::Min<NSUInteger>(Cache.DirtyRange.location, VertexBufferSlot);
    const NSUInteger NewEnd   = Math::Max<NSUInteger>(OldEnd, VertexBufferSlot + 1);
    Cache.DirtyRange          = NSMakeRange(NewStart, NewEnd - NewStart);
    Cache.NumVertexBuffers    = Math::Max<uint32>(Cache.NumVertexBuffers, VertexBufferSlot + 1);

    GraphicsState.bBindVertexBuffers = true;
}

void FMetalCommandContextState::SetIndexBuffer(FMetalBufferRHI* IndexBuffer, MTLIndexType IndexType)
{
    FMetalIndexBufferCache& Cache = GraphicsState.IndexBufferCache;
    Cache.IndexBuffer    = IndexBuffer ? IndexBuffer->GetMTLBuffer() : nil;
    Cache.BufferResource = IndexBuffer;
    Cache.Offset         = 0;
    Cache.IndexType      = IndexType;

    GraphicsState.bBindIndexBuffer = true;
}

void FMetalCommandContextState::SetPrimitiveType(MTLPrimitiveType PrimitiveType)
{
    GraphicsState.PrimitiveType          = PrimitiveType;
    GraphicsState.bBindPrimitiveTopology = true;
}

void FMetalCommandContextState::SetSRV(FMetalShaderResourceViewRHI* ShaderResourceView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    CHECK(ShaderStage < EShaderVisibility::Count);
    CHECK(ResourceIndex < MAX_SRVS);

    FMetalShaderResourceViewCache& Cache = CommonState.ShaderResourceViewCache;
    Cache.ResourceViews[ShaderStage][ResourceIndex] = ShaderResourceView;
    Cache.NumViews[ShaderStage] = Math::Max<uint8>(Cache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex + 1));
    Cache.DirtyResources(ShaderStage);
}

void FMetalCommandContextState::SetUAV(FMetalUnorderedAccessViewRHI* UnorderedAccessView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    CHECK(ShaderStage < EShaderVisibility::Count);
    CHECK(ResourceIndex < MAX_UAVS);

    FMetalUnorderedAccessViewCache& Cache = CommonState.UnorderedAccessViewCache;
    Cache.ResourceViews[ShaderStage][ResourceIndex] = UnorderedAccessView;
    Cache.NumViews[ShaderStage] = Math::Max<uint8>(Cache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex + 1));
    Cache.DirtyResources(ShaderStage);
}

void FMetalCommandContextState::SetCBV(FMetalBufferRHI* Buffer, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    CHECK(ShaderStage < EShaderVisibility::Count);
    CHECK(ResourceIndex < MAX_CONSTANT_BUFFERS);

    FMetalConstantBufferCache& Cache = CommonState.ConstantBufferCache;
    Cache.ConstantBuffers[ShaderStage][ResourceIndex] = Buffer;
    Cache.NumBuffers[ShaderStage] = Math::Max<uint8>(Cache.NumBuffers[ShaderStage], static_cast<uint8>(ResourceIndex + 1));
    Cache.DirtyResources(ShaderStage);
}

void FMetalCommandContextState::SetSampler(FMetalSamplerStateRHI* SamplerState, EShaderVisibility::Type ShaderStage, uint32 SamplerIndex)
{
    CHECK(ShaderStage < EShaderVisibility::Count);
    CHECK(SamplerIndex < MAX_SAMPLER_STATES);

    if (ShaderStage == EShaderVisibility::Compute)
    {
        if (ComputeState.PipelineState && ComputeState.PipelineState->HasStaticSampler(ShaderStage, SamplerIndex))
        {
            return;
        }
    }
    else
    {
        if (GraphicsState.PipelineState && GraphicsState.PipelineState->HasStaticSampler(ShaderStage, SamplerIndex))
        {
            return;
        }

        if (GraphicsState.MeshletPipelineState && GraphicsState.MeshletPipelineState->HasStaticSampler(ShaderStage, SamplerIndex))
        {
            return;
        }
    }

    FMetalSamplerStateCache& Cache = CommonState.SamplerStateCache;
    Cache.SamplerStates[ShaderStage][SamplerIndex] = SamplerState;
    Cache.NumSamplers[ShaderStage] = Math::Max<uint8>(Cache.NumSamplers[ShaderStage], static_cast<uint8>(SamplerIndex + 1));
    Cache.DirtyResources(ShaderStage);
}

void FMetalCommandContextState::SetShaderConstants(EShaderVisibility::Type ShaderStage, const uint32* ShaderConstants, uint32 NumShaderConstants)
{
    CHECK(ShaderStage < EShaderVisibility::Count);
    CHECK(NumShaderConstants <= MAX_SHADER_CONSTANTS);

    FMetalShaderConstantsCache& Cache = CommonState.ShaderConstantsCache;
    if (ShaderConstants && NumShaderConstants > 0)
    {
        Memory::Memcpy(Cache.Constants[ShaderStage], ShaderConstants, sizeof(uint32) * NumShaderConstants);
    }

    Cache.NumConstants[ShaderStage] = NumShaderConstants;

    if (ShaderStage == EShaderVisibility::Compute)
    {
        ComputeState.bBindShaderConstants = true;
    }
    else
    {
        GraphicsState.bBindShaderConstants = true;
    }
}

void FMetalCommandContextState::PrepareGraphicsState()
{
}

void FMetalCommandContextState::PrepareComputeState()
{
}

void FMetalCommandContextState::BindGraphicsState()
{
    id<MTLRenderCommandEncoder> Encoder = Context.GetGraphicsEncoder();
    if (Encoder == nil)
    {
        return;
    }

    FMetalGraphicsPipelineStateRHI* GraphicsPipeline = GraphicsState.PipelineState.Get();
    FMetalMeshletPipelineStateRHI*  MeshletPipeline  = GraphicsState.MeshletPipelineState.Get();

    if (GraphicsPipeline == nullptr && MeshletPipeline == nullptr)
    {
        return;
    }

    if (GraphicsState.bBindPipelineState)
    {
        id<MTLRenderPipelineState> PipelineState = GraphicsPipeline ? GraphicsPipeline->GetMTLPipelineState() : MeshletPipeline->GetMTLPipelineState();
        if (PipelineState)
        {
            [Encoder setRenderPipelineState:PipelineState];
        }

        FMetalDepthStencilStateRHI* DepthStencilState = GraphicsPipeline ? GraphicsPipeline->GetMetalDepthStencilState() : MeshletPipeline->GetMetalDepthStencilState();
        if (DepthStencilState)
        {
            [Encoder setDepthStencilState:DepthStencilState->GetMTLDepthStencilState()];
        }

        FMetalRasterizerStateRHI* RasterizerState = GraphicsPipeline ? GraphicsPipeline->GetMetalRasterizerState() : MeshletPipeline->GetMetalRasterizerState();
        if (RasterizerState)
        {
            [Encoder setFrontFacingWinding:RasterizerState->GetMTLFrontFaceWinding()];
            [Encoder setTriangleFillMode:RasterizerState->GetMTLFillMode()];
            [Encoder setCullMode:RasterizerState->GetMTLCullMode()];
        }

        GraphicsState.bBindPipelineState = false;
    }

    if (GraphicsState.bBindViewports && GraphicsState.NumViewports > 0)
    {
        [Encoder setViewport:GraphicsState.Viewports[0]];
        GraphicsState.bBindViewports = false;
    }

    if (GraphicsState.bBindScissorRects && GraphicsState.NumScissorRects > 0)
    {
        [Encoder setScissorRect:GraphicsState.ScissorRects[0]];
        GraphicsState.bBindScissorRects = false;
    }

    if (GraphicsState.bBindBlendFactor)
    {
        [Encoder setBlendColorRed:GraphicsState.BlendFactor[0]
                            green:GraphicsState.BlendFactor[1]
                             blue:GraphicsState.BlendFactor[2]
                            alpha:GraphicsState.BlendFactor[3]];

        GraphicsState.bBindBlendFactor = false;
    }

    if (GraphicsState.bBindStencilRef)
    {
        [Encoder setStencilReferenceValue:GraphicsState.StencilRef];
        GraphicsState.bBindStencilRef = false;
    }

    if (GraphicsState.bBindDepthBias)
    {
        [Encoder setDepthBias:GraphicsState.DepthBias[0]
                   slopeScale:GraphicsState.DepthBias[2]
                        clamp:GraphicsState.DepthBias[1]];

        GraphicsState.bBindDepthBias = false;
    }

    if (GraphicsState.bBindVertexBuffers && GraphicsState.VertexBufferCache.DirtyRange.length > 0)
    {
        FMetalVertexBufferCache& Cache = GraphicsState.VertexBufferCache;
        [Encoder setVertexBuffers:Cache.VertexBuffers
                          offsets:Cache.Offsets
                        withRange:Cache.DirtyRange];

        Cache.DirtyRange                 = NSMakeRange(0, 0);
        GraphicsState.bBindVertexBuffers = false;
    }

    for (uint32 Stage = EShaderVisibility::Vertex; Stage < EShaderVisibility::Count; Stage++)
    {
        const EShaderVisibility::Type ShaderStage = static_cast<EShaderVisibility::Type>(Stage);
        BindGraphicsResources(ShaderStage);
        BindGraphicsSamplers(ShaderStage);
    }

    if (GraphicsState.bBindShaderConstants)
    {
        for (uint32 Stage = EShaderVisibility::Vertex; Stage < EShaderVisibility::Count; Stage++)
        {
            BindGraphicsShaderConstants(static_cast<EShaderVisibility::Type>(Stage));
        }

        GraphicsState.bBindShaderConstants = false;
    }
}

void FMetalCommandContextState::BindComputeState()
{
    id<MTLComputeCommandEncoder> Encoder = Context.GetComputeEncoder();
    if (Encoder == nil)
    {
        return;
    }

    FMetalComputePipelineStateRHI* Pipeline = ComputeState.PipelineState.Get();
    if (Pipeline == nullptr)
    {
        return;
    }

    if (ComputeState.bBindPipelineState)
    {
        if (id<MTLComputePipelineState> PipelineState = Pipeline->GetMTLPipelineState())
        {
            [Encoder setComputePipelineState:PipelineState];
        }

        ComputeState.bBindPipelineState = false;
    }

    BindComputeResources();
    BindComputeSamplers();

    if (ComputeState.bBindShaderConstants)
    {
        BindComputeShaderConstants();
        ComputeState.bBindShaderConstants = false;
    }
}

void FMetalCommandContextState::BindShaderConstants(EShaderVisibility::Type ShaderStage)
{
    if (ShaderStage == EShaderVisibility::Compute)
    {
        BindComputeShaderConstants();
    }
    else
    {
        BindGraphicsShaderConstants(ShaderStage);
    }
}

void FMetalCommandContextState::BindGraphicsResources(EShaderVisibility::Type ShaderStage)
{
    CHECK(ShaderStage != EShaderVisibility::Compute);

    if (Context.GetGraphicsEncoder() == nil)
    {
        return;
    }

    const FMetalPipelineBindingLayout* Layout = GetBoundLayout(ShaderStage);
    if (!Layout)
    {
        return;
    }

    FMetalConstantBufferCache&      CBVCache = CommonState.ConstantBufferCache;
    FMetalShaderResourceViewCache&  SRVCache = CommonState.ShaderResourceViewCache;
    FMetalUnorderedAccessViewCache& UAVCache = CommonState.UnorderedAccessViewCache;

    const bool bCBVsDirty = CBVCache.IsResourcesDirty(ShaderStage);
    const bool bSRVsDirty = SRVCache.IsResourcesDirty(ShaderStage);
    const bool bUAVsDirty = UAVCache.IsResourcesDirty(ShaderStage);

    if (!bCBVsDirty && !bSRVsDirty && !bUAVsDirty)
    {
        return;
    }

    if (bCBVsDirty)
    {
        for (uint8 Index = 0; Index < CBVCache.NumBuffers[ShaderStage]; Index++)
        {
            const uint8 Slot = Layout->GetSlot(ShaderStage, EMSLBindingType::ConstantBuffer, Index);
            if (Slot == InvalidMSLSlot)
            {
                continue;
            }

            FMetalBufferRHI* Buffer = CBVCache.ConstantBuffers[ShaderStage][Index];

            id<MTLBuffer> MTLBufferHandle = Buffer ? Buffer->GetMTLBuffer() : nil;
            Context.SetGraphicsBuffer(ShaderStage, MTLBufferHandle, 0, Slot);
        }

        CBVCache.ClearResourcesDirty(ShaderStage);
    }

    if (bSRVsDirty)
    {
        for (uint8 Index = 0; Index < SRVCache.NumViews[ShaderStage]; Index++)
        {
            FMetalShaderResourceViewRHI* View = SRVCache.ResourceViews[ShaderStage][Index];

            const uint8 BufferSlot = Layout->GetSlot(ShaderStage, EMSLBindingType::ShaderResourceBuffer, Index);
            if (BufferSlot != InvalidMSLSlot)
            {
                id<MTLBuffer> MTLBufferHandle = View ? View->GetMTLBuffer() : nil;
                const NSUInteger Offset = View ? View->GetBufferOffset() : 0;
                Context.SetGraphicsBuffer(ShaderStage, MTLBufferHandle, Offset, BufferSlot);
                continue;
            }

            const uint8 TextureSlot = Layout->GetSlot(ShaderStage, EMSLBindingType::ShaderResourceTexture, Index);
            if (TextureSlot != InvalidMSLSlot)
            {
                id<MTLTexture> MTLTextureHandle = View ? View->GetMTLTexture() : nil;
                Context.SetGraphicsTexture(ShaderStage, MTLTextureHandle, TextureSlot);
            }
        }

        SRVCache.ClearResourcesDirty(ShaderStage);
    }

    if (bUAVsDirty)
    {
        for (uint8 Index = 0; Index < UAVCache.NumViews[ShaderStage]; Index++)
        {
            FMetalUnorderedAccessViewRHI* View = UAVCache.ResourceViews[ShaderStage][Index];

            const uint8 BufferSlot = Layout->GetSlot(ShaderStage, EMSLBindingType::UnorderedAccessBuffer, Index);
            if (BufferSlot != InvalidMSLSlot)
            {
                id<MTLBuffer> MTLBufferHandle = View ? View->GetMTLBuffer() : nil;
                const NSUInteger Offset = View ? View->GetBufferOffset() : 0;
                Context.SetGraphicsBuffer(ShaderStage, MTLBufferHandle, Offset, BufferSlot);
                continue;
            }

            const uint8 TextureSlot = Layout->GetSlot(ShaderStage, EMSLBindingType::UnorderedAccessTexture, Index);
            if (TextureSlot != InvalidMSLSlot)
            {
                id<MTLTexture> MTLTextureHandle = View ? View->GetMTLTexture() : nil;
                Context.SetGraphicsTexture(ShaderStage, MTLTextureHandle, TextureSlot);
            }
        }

        UAVCache.ClearResourcesDirty(ShaderStage);
    }
}

void FMetalCommandContextState::BindGraphicsSamplers(EShaderVisibility::Type ShaderStage)
{
    CHECK(ShaderStage != EShaderVisibility::Compute);

    if (Context.GetGraphicsEncoder() == nil)
    {
        return;
    }

    const FMetalPipelineBindingLayout* Layout = GetBoundLayout(ShaderStage);
    if (!Layout)
    {
        return;
    }

    FMetalSamplerStateCache& Cache = CommonState.SamplerStateCache;
    if (!Cache.IsResourcesDirty(ShaderStage))
    {
        return;
    }

    for (uint8 Index = 0; Index < Cache.NumSamplers[ShaderStage]; Index++)
    {
        const uint8 Slot = Layout->GetSlot(ShaderStage, EMSLBindingType::Sampler, Index);
        if (Slot == InvalidMSLSlot)
        {
            continue;
        }

        FMetalSamplerStateRHI* SamplerState = Cache.SamplerStates[ShaderStage][Index];

        id<MTLSamplerState> MTLSampler = SamplerState ? SamplerState->GetMTLSamplerState() : nil;
        Context.SetGraphicsSampler(ShaderStage, MTLSampler, Slot);
    }

    Cache.ClearResourcesDirty(ShaderStage);
}

void FMetalCommandContextState::BindGraphicsShaderConstants(EShaderVisibility::Type ShaderStage)
{
    CHECK(ShaderStage != EShaderVisibility::Compute);

    if (Context.GetGraphicsEncoder() == nil)
    {
        return;
    }

    const FMetalPipelineBindingLayout* Layout = GetBoundLayout(ShaderStage);
    if (!Layout)
    {
        return;
    }

    FMetalShaderConstantsCache& Cache = CommonState.ShaderConstantsCache;
    const uint32 NumConstants = Cache.NumConstants[ShaderStage];
    if (NumConstants == 0)
    {
        return;
    }

    const uint8 Slot = Layout->GetSlot(ShaderStage, EMSLBindingType::ShaderConstants, 0);
    if (Slot == InvalidMSLSlot)
    {
        return;
    }

    const NSUInteger ByteLength = NumConstants * sizeof(uint32);
    Context.SetGraphicsBytes(ShaderStage, Cache.Constants[ShaderStage], ByteLength, Slot);
}

const FMetalPipelineBindingLayout* FMetalCommandContextState::GetBoundLayout(EShaderVisibility::Type ShaderStage) const
{
    if (ShaderStage == EShaderVisibility::Compute)
    {
        FMetalComputePipelineStateRHI* Pipeline = ComputeState.PipelineState.Get();
        return Pipeline ? &Pipeline->GetBindings() : nullptr;
    }

    if (FMetalMeshletPipelineStateRHI* MeshletPipeline = GraphicsState.MeshletPipelineState.Get())
    {
        return &MeshletPipeline->GetBindings();
    }

    if (FMetalGraphicsPipelineStateRHI* Pipeline = GraphicsState.PipelineState.Get())
    {
        return &Pipeline->GetBindings();
    }

    return nullptr;
}

void FMetalCommandContextState::BindComputeResources()
{
    id<MTLComputeCommandEncoder> Encoder = Context.GetComputeEncoder();
    if (Encoder == nil)
    {
        return;
    }

    const FMetalPipelineBindingLayout* Layout = GetBoundLayout(EShaderVisibility::Compute);
    if (!Layout)
    {
        return;
    }

    FMetalConstantBufferCache&      CBVCache = CommonState.ConstantBufferCache;
    FMetalShaderResourceViewCache&  SRVCache = CommonState.ShaderResourceViewCache;
    FMetalUnorderedAccessViewCache& UAVCache = CommonState.UnorderedAccessViewCache;

    if (CBVCache.IsResourcesDirty(EShaderVisibility::Compute))
    {
        for (uint8 Index = 0; Index < CBVCache.NumBuffers[EShaderVisibility::Compute]; Index++)
        {
            const uint8 Slot = Layout->GetSlot(EShaderVisibility::Compute, EMSLBindingType::ConstantBuffer, Index);
            if (Slot == InvalidMSLSlot)
            {
                continue;
            }

            FMetalBufferRHI* Buffer = CBVCache.ConstantBuffers[EShaderVisibility::Compute][Index];

            id<MTLBuffer> MTLBufferHandle = Buffer ? Buffer->GetMTLBuffer() : nil;
            [Encoder setBuffer:MTLBufferHandle offset:0 atIndex:Slot];
        }

        CBVCache.ClearResourcesDirty(EShaderVisibility::Compute);
    }

    if (SRVCache.IsResourcesDirty(EShaderVisibility::Compute))
    {
        for (uint8 Index = 0; Index < SRVCache.NumViews[EShaderVisibility::Compute]; Index++)
        {
            FMetalShaderResourceViewRHI* View = SRVCache.ResourceViews[EShaderVisibility::Compute][Index];

            const uint8 BufferSlot = Layout->GetSlot(EShaderVisibility::Compute, EMSLBindingType::ShaderResourceBuffer, Index);
            if (BufferSlot != InvalidMSLSlot)
            {
                id<MTLBuffer> MTLBufferHandle = View ? View->GetMTLBuffer() : nil;
                const NSUInteger Offset = View ? View->GetBufferOffset() : 0;
                [Encoder setBuffer:MTLBufferHandle offset:Offset atIndex:BufferSlot];
                continue;
            }

            const uint8 TextureSlot = Layout->GetSlot(EShaderVisibility::Compute, EMSLBindingType::ShaderResourceTexture, Index);
            if (TextureSlot != InvalidMSLSlot)
            {
                id<MTLTexture> MTLTextureHandle = View ? View->GetMTLTexture() : nil;
                [Encoder setTexture:MTLTextureHandle atIndex:TextureSlot];
            }
        }

        SRVCache.ClearResourcesDirty(EShaderVisibility::Compute);
    }

    if (UAVCache.IsResourcesDirty(EShaderVisibility::Compute))
    {
        for (uint8 Index = 0; Index < UAVCache.NumViews[EShaderVisibility::Compute]; Index++)
        {
            FMetalUnorderedAccessViewRHI* View = UAVCache.ResourceViews[EShaderVisibility::Compute][Index];

            const uint8 BufferSlot = Layout->GetSlot(EShaderVisibility::Compute, EMSLBindingType::UnorderedAccessBuffer, Index);
            if (BufferSlot != InvalidMSLSlot)
            {
                id<MTLBuffer> MTLBufferHandle = View ? View->GetMTLBuffer() : nil;
                const NSUInteger Offset = View ? View->GetBufferOffset() : 0;
                [Encoder setBuffer:MTLBufferHandle offset:Offset atIndex:BufferSlot];
                continue;
            }

            const uint8 TextureSlot = Layout->GetSlot(EShaderVisibility::Compute, EMSLBindingType::UnorderedAccessTexture, Index);
            if (TextureSlot != InvalidMSLSlot)
            {
                id<MTLTexture> MTLTextureHandle = View ? View->GetMTLTexture() : nil;
                [Encoder setTexture:MTLTextureHandle atIndex:TextureSlot];
            }
        }

        UAVCache.ClearResourcesDirty(EShaderVisibility::Compute);
    }
}

void FMetalCommandContextState::BindComputeSamplers()
{
    id<MTLComputeCommandEncoder> Encoder = Context.GetComputeEncoder();
    if (Encoder == nil)
    {
        return;
    }

    const FMetalPipelineBindingLayout* Layout = GetBoundLayout(EShaderVisibility::Compute);
    if (!Layout)
    {
        return;
    }

    FMetalSamplerStateCache& Cache = CommonState.SamplerStateCache;
    if (!Cache.IsResourcesDirty(EShaderVisibility::Compute))
    {
        return;
    }

    for (uint8 Index = 0; Index < Cache.NumSamplers[EShaderVisibility::Compute]; Index++)
    {
        const uint8 Slot = Layout->GetSlot(EShaderVisibility::Compute, EMSLBindingType::Sampler, Index);
        if (Slot == InvalidMSLSlot)
        {
            continue;
        }

        FMetalSamplerStateRHI* SamplerState = Cache.SamplerStates[EShaderVisibility::Compute][Index];

        id<MTLSamplerState>  MTLSampler = SamplerState ? SamplerState->GetMTLSamplerState() : nil;
        [Encoder setSamplerState:MTLSampler atIndex:Slot];
    }

    Cache.ClearResourcesDirty(EShaderVisibility::Compute);
}

void FMetalCommandContextState::BindComputeShaderConstants()
{
    id<MTLComputeCommandEncoder> Encoder = Context.GetComputeEncoder();
    if (Encoder == nil)
    {
        return;
    }

    const FMetalPipelineBindingLayout* Layout = GetBoundLayout(EShaderVisibility::Compute);
    if (!Layout)
    {
        return;
    }

    FMetalShaderConstantsCache& Cache = CommonState.ShaderConstantsCache;
    const uint32 NumConstants = Cache.NumConstants[EShaderVisibility::Compute];
    if (NumConstants == 0)
    {
        return;
    }

    const uint8 Slot = Layout->GetSlot(EShaderVisibility::Compute, EMSLBindingType::ShaderConstants, 0);
    if (Slot == InvalidMSLSlot)
    {
        return;
    }

    const NSUInteger ByteLength = NumConstants * sizeof(uint32);
    [Encoder setBytes:Cache.Constants[EShaderVisibility::Compute] length:ByteLength atIndex:Slot];
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
