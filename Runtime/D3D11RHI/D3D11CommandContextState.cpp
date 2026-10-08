#include "D3D11RHI/D3D11CommandContextState.h"
#include "D3D11RHI/D3D11CommandContext.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11RHI.h"
#include "D3D11RHI/D3D11Stats.h"

template<typename ObjectType>
static bool SetIfChanged(ObjectType& BoundObject, ObjectType NewObject)
{
    if (BoundObject == NewObject)
    {
        return false;
    }

    BoundObject = NewObject;
    return true;
}

template<typename ObjectType>
static bool FindChangedRange(ObjectType* const* BoundObjects, ObjectType* const* NewObjects, uint32 NumObjects, uint32& OutFirst, uint32& OutCount)
{
    uint32 First = NumObjects;
    uint32 Last  = 0;
    for (uint32 Index = 0; Index < NumObjects; Index++)
    {
        if (BoundObjects[Index] != NewObjects[Index])
        {
            First = Math::Min(First, Index);
            Last  = Math::Max(Last, Index);
        }
    }

    if (First == NumObjects)
    {
        return false;
    }

    OutFirst = First;
    OutCount = Last - First + 1;
    return true;
}

static bool IsBufferDimension(D3D_SRV_DIMENSION Dimension)
{
    return Dimension == D3D_SRV_DIMENSION_BUFFER || Dimension == D3D_SRV_DIMENSION_BUFFEREX;
}

static bool IsViewDimensionCompatible(D3D_SRV_DIMENSION DeclaredDimension, D3D11_SRV_DIMENSION ViewDimension)
{
    const D3D_SRV_DIMENSION Dimension = static_cast<D3D_SRV_DIMENSION>(ViewDimension);
    if (IsBufferDimension(DeclaredDimension) && IsBufferDimension(Dimension))
    {
        return true;
    }

    return DeclaredDimension != D3D_SRV_DIMENSION_UNKNOWN && DeclaredDimension == Dimension;
}

FD3D11CommandContextState::FD3D11CommandContextState(FD3D11Device* InDevice, FD3D11CommandContext& InContext)
    : FD3D11DeviceChild(InDevice)
    , Context(InContext)
    , StateChanges()
    , CommonGraphicsState()
    , GraphicsState()
    , ComputeState()
    , CommonState()
{
}

FD3D11CommandContextState::~FD3D11CommandContextState() = default;

bool FD3D11CommandContextState::Initialize()
{
    D3D11_BUFFER_DESC ConstantsBufferDesc = {};
    ConstantsBufferDesc.ByteWidth      = static_cast<UINT>(D3D11_MAX_32BIT_SHADER_CONSTANTS_COUNT * sizeof(uint32));
    ConstantsBufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
    ConstantsBufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    ConstantsBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    for (int32 Index = 0; Index < EShaderConstantsPipeline::Count; Index++)
    {
        const HRESULT Result = GetDevice()->GetD3D11Device()->CreateBuffer(&ConstantsBufferDesc, nullptr, &CommonState.ShaderConstantsBuffers[Index]);
        if (FAILED(Result))
        {
            D3D11_ERROR_CRITICAL("[FD3D11CommandContextState]: FAILED to create the ShaderConstants buffer (0x%08X)", static_cast<uint32>(Result));
            return false;
        }

        D3D11SetDebugName(CommonState.ShaderConstantsBuffers[Index].Get(), Index == EShaderConstantsPipeline::Graphics ? "Graphics ShaderConstants" : "Compute ShaderConstants");
    }

    ResetState();
    return true;
}

void FD3D11CommandContextState::BindGraphicsState()
{
    FD3D11GraphicsPipelineStateRHI* PipelineState = GraphicsState.PipelineState.Get();
    CHECK(PipelineState != nullptr);

    ID3D11DeviceContext*         D3D11Context  = Context.GetD3D11Context();
    FD3D11BoundGraphicsPipeline& BoundPipeline = GraphicsState.BoundPipeline;
    if (GraphicsState.bBindPipelineState)
    {
        FD3D11VertexShaderRHI* VertexShader = PipelineState->GetVertexShader();
        FD3D11HullShaderRHI*   HullShader   = PipelineState->GetHullShader();
        FD3D11DomainShaderRHI* DomainShader = PipelineState->GetDomainShader();
        FD3D11PixelShaderRHI*  PixelShader  = PipelineState->GetPixelShader();

        if (SetIfChanged(BoundPipeline.InputLayout, PipelineState->GetInputLayout()))
        {
            D3D11Context->IASetInputLayout(BoundPipeline.InputLayout);
            StateChanges[ED3D11StateChange::InputAssembler]++;
        }

        if (SetIfChanged(BoundPipeline.PrimitiveTopology, PipelineState->GetPrimitiveTopology()))
        {
            D3D11Context->IASetPrimitiveTopology(BoundPipeline.PrimitiveTopology);
            StateChanges[ED3D11StateChange::InputAssembler]++;
        }

        if (SetIfChanged(BoundPipeline.VertexShader, VertexShader ? VertexShader->GetD3D11Shader() : nullptr))
        {
            D3D11Context->VSSetShader(BoundPipeline.VertexShader, nullptr, 0);
            StateChanges[ED3D11StateChange::Shader]++;
        }

        if (SetIfChanged(BoundPipeline.HullShader, HullShader ? HullShader->GetD3D11Shader() : nullptr))
        {
            D3D11Context->HSSetShader(BoundPipeline.HullShader, nullptr, 0);
            StateChanges[ED3D11StateChange::Shader]++;
        }

        if (SetIfChanged(BoundPipeline.DomainShader, DomainShader ? DomainShader->GetD3D11Shader() : nullptr))
        {
            D3D11Context->DSSetShader(BoundPipeline.DomainShader, nullptr, 0);
            StateChanges[ED3D11StateChange::Shader]++;
        }

        if (SetIfChanged(BoundPipeline.GeometryShader, PipelineState->GetD3D11GeometryShader()))
        {
            D3D11Context->GSSetShader(BoundPipeline.GeometryShader, nullptr, 0);
            StateChanges[ED3D11StateChange::Shader]++;
        }

        if (SetIfChanged(BoundPipeline.PixelShader, PixelShader ? PixelShader->GetD3D11Shader() : nullptr))
        {
            D3D11Context->PSSetShader(BoundPipeline.PixelShader, nullptr, 0);
            StateChanges[ED3D11StateChange::Shader]++;
        }

        GraphicsState.bBindPipelineState = false;
    }

    if (CommonGraphicsState.bBindRasterizerState)
    {
        if (SetIfChanged(BoundPipeline.RasterizerState, GetRasterizerState(PipelineState)))
        {
            D3D11Context->RSSetState(BoundPipeline.RasterizerState);
            StateChanges[ED3D11StateChange::Rasterizer]++;
        }

        CommonGraphicsState.bBindRasterizerState = false;
    }

    if (CommonGraphicsState.bBindBlendState)
    {
        ID3D11BlendState* BlendState = PipelineState->GetBlendState()->GetD3D11State();
        const uint32      SampleMask = PipelineState->GetSampleMask();

        const bool bBlendFactorChanged = Memory::Memcmp(BoundPipeline.BlendFactor, CommonGraphicsState.BlendFactor, sizeof(BoundPipeline.BlendFactor)) != 0;
        if (BoundPipeline.BlendState != BlendState || BoundPipeline.SampleMask != SampleMask || bBlendFactorChanged)
        {
            BoundPipeline.BlendState = BlendState;
            BoundPipeline.SampleMask = SampleMask;
            Memory::Memcpy(BoundPipeline.BlendFactor, CommonGraphicsState.BlendFactor, sizeof(BoundPipeline.BlendFactor));

            D3D11Context->OMSetBlendState(BlendState, BoundPipeline.BlendFactor, SampleMask);
            StateChanges[ED3D11StateChange::OutputMerger]++;
        }

        CommonGraphicsState.bBindBlendState = false;
    }

    if (CommonGraphicsState.bBindDepthStencilState)
    {
        ID3D11DepthStencilState* DepthStencilState = PipelineState->GetDepthStencilState()->GetD3D11State();
        if (BoundPipeline.DepthStencilState != DepthStencilState || BoundPipeline.StencilRef != CommonGraphicsState.StencilRef)
        {
            BoundPipeline.DepthStencilState = DepthStencilState;
            BoundPipeline.StencilRef        = CommonGraphicsState.StencilRef;

            D3D11Context->OMSetDepthStencilState(DepthStencilState, BoundPipeline.StencilRef);
            StateChanges[ED3D11StateChange::OutputMerger]++;
        }

        CommonGraphicsState.bBindDepthStencilState = false;
    }

    if (CommonGraphicsState.bBindRenderTargets)
    {
        BindRenderTargets();
        CommonGraphicsState.bBindRenderTargets = false;
    }

    if (CommonGraphicsState.bBindViewports)
    {
        D3D11Context->RSSetViewports(CommonGraphicsState.NumViewports, CommonGraphicsState.Viewports);
        StateChanges[ED3D11StateChange::Rasterizer]++;
        CommonGraphicsState.bBindViewports = false;
    }

    if (CommonGraphicsState.bBindScissorRects)
    {
        D3D11Context->RSSetScissorRects(CommonGraphicsState.NumScissorRects, CommonGraphicsState.ScissorRects);
        StateChanges[ED3D11StateChange::Rasterizer]++;
        CommonGraphicsState.bBindScissorRects = false;
    }

    if (GraphicsState.bBindVertexBuffers)
    {
        const FD3D11VertexBufferCache& VertexBufferCache = GraphicsState.VertexBufferCache;
        D3D11Context->IASetVertexBuffers(0, VertexBufferCache.NumVertexBuffers, VertexBufferCache.VertexBuffers, VertexBufferCache.Strides, VertexBufferCache.Offsets);
        StateChanges[ED3D11StateChange::InputAssembler]++;
        GraphicsState.bBindVertexBuffers = false;
    }

    if (GraphicsState.bBindIndexBuffer)
    {
        const FD3D11IndexBufferCache& IndexBufferCache = GraphicsState.IndexBufferCache;
        D3D11Context->IASetIndexBuffer(IndexBufferCache.IndexBuffer, IndexBufferCache.IndexFormat, 0);
        StateChanges[ED3D11StateChange::InputAssembler]++;
        GraphicsState.bBindIndexBuffer = false;
    }

    if (GraphicsState.bBindStreamOutputTargets)
    {
        const FD3D11StreamOutputCache& StreamOutputCache = GraphicsState.StreamOutputCache;
        D3D11Context->SOSetTargets(D3D11_MAX_STREAM_OUTPUT_BUFFER_COUNT, StreamOutputCache.Buffers, StreamOutputCache.Offsets);
        StateChanges[ED3D11StateChange::OutputMerger]++;
        GraphicsState.bBindStreamOutputTargets = false;
    }

    if (GraphicsState.bBindShaderConstants)
    {
        BindShaderConstants(EShaderConstantsPipeline::Graphics);
        GraphicsState.bBindShaderConstants = false;
    }

    for (int32 Index = EShaderVisibility::Vertex; Index <= EShaderVisibility::Pixel; Index++)
    {
        const EShaderVisibility::Type ShaderStage = static_cast<EShaderVisibility::Type>(Index);
        FD3D11Shader* Shader = PipelineState->GetShader(ShaderStage);
        BindConstantBuffers(ShaderStage, Shader);
        BindShaderResourceViews(ShaderStage, Shader);
        BindSamplers(ShaderStage, PipelineState->GetStaticSamplers());
    }
}

void FD3D11CommandContextState::BindComputeState()
{
    FD3D11ComputePipelineStateRHI* PipelineState = ComputeState.PipelineState.Get();
    CHECK(PipelineState != nullptr);

    if (ComputeState.bBindPipelineState)
    {
        if (SetIfChanged(ComputeState.BoundComputeShader, PipelineState->GetComputeShader()->GetD3D11Shader()))
        {
            Context.GetD3D11Context()->CSSetShader(ComputeState.BoundComputeShader, nullptr, 0);
            StateChanges[ED3D11StateChange::Shader]++;
        }

        ComputeState.bBindPipelineState = false;
    }

    if (ComputeState.bBindShaderConstants)
    {
        BindShaderConstants(EShaderConstantsPipeline::Compute);
        ComputeState.bBindShaderConstants = false;
    }

    FD3D11ComputeShaderRHI* ComputeShader = PipelineState->GetComputeShader();
    BindUnorderedAccessViews(ComputeShader);
    BindConstantBuffers(EShaderVisibility::Compute, ComputeShader);
    BindShaderResourceViews(EShaderVisibility::Compute, ComputeShader);
    BindSamplers(EShaderVisibility::Compute, PipelineState->GetStaticSamplers());
}

void FD3D11CommandContextState::ResetState()
{
    for (FD3D11ShaderConstantsCache& ConstantCache : CommonState.ShaderConstantsCache)
    {
        ConstantCache.Clear();
    }

    CommonState.ConstantBufferCache.Clear();
    CommonState.ShaderResourceViewCache.Clear();
    CommonState.SamplerStateCache.Clear();

    CommonGraphicsState.RenderTargetCache.Clear();
    GraphicsState.VertexBufferCache.Clear();
    GraphicsState.IndexBufferCache.Clear();
    GraphicsState.StreamOutputCache.Clear();
    ComputeState.UnorderedAccessViewCache.Clear();

    Memory::Memzero(CommonGraphicsState.BlendFactor, sizeof(CommonGraphicsState.BlendFactor));
    CommonGraphicsState.StencilRef = 0;

    Memory::Memzero(CommonGraphicsState.Viewports, sizeof(CommonGraphicsState.Viewports));
    CommonGraphicsState.NumViewports = 0;

    Memory::Memzero(CommonGraphicsState.ScissorRects, sizeof(CommonGraphicsState.ScissorRects));
    CommonGraphicsState.NumScissorRects = 0;

    Memory::Memzero(CommonGraphicsState.DepthBias, sizeof(CommonGraphicsState.DepthBias));
    CommonGraphicsState.DepthBiasRasterizerState.Reset();

    GraphicsState.BoundPipeline.Clear();
    ComputeState.BoundComputeShader = nullptr;

    GraphicsState.PipelineState                = nullptr;
    GraphicsState.bBindPipelineState           = true;
    GraphicsState.bBindVertexBuffers           = true;
    GraphicsState.bBindIndexBuffer             = true;
    GraphicsState.bBindStreamOutputTargets     = true;
    GraphicsState.bBindShaderConstants         = true;
    CommonGraphicsState.bBindRenderTargets     = true;
    CommonGraphicsState.bBindBlendState        = true;
    CommonGraphicsState.bBindDepthStencilState = true;
    CommonGraphicsState.bBindRasterizerState   = true;
    CommonGraphicsState.bBindScissorRects      = true;
    CommonGraphicsState.bBindViewports         = true;

    ComputeState.PipelineState        = nullptr;
    ComputeState.bBindPipelineState   = true;
    ComputeState.bBindShaderConstants = true;
}

void FD3D11CommandContextState::DirtyRenderTargets()
{
    CommonGraphicsState.bBindRenderTargets = true;
}

void FD3D11CommandContextState::UpdateStateChangeStats()
{
#if D3D11_ENABLE_STATS
    STAT_SET(STAT_D3D11_ShaderChanges,          StateChanges[ED3D11StateChange::Shader]);
    STAT_SET(STAT_D3D11_InputAssemblerChanges,  StateChanges[ED3D11StateChange::InputAssembler]);
    STAT_SET(STAT_D3D11_RasterizerChanges,      StateChanges[ED3D11StateChange::Rasterizer]);
    STAT_SET(STAT_D3D11_OutputMergerChanges,    StateChanges[ED3D11StateChange::OutputMerger]);
    STAT_SET(STAT_D3D11_ConstantBufferChanges,  StateChanges[ED3D11StateChange::ConstantBuffer]);
    STAT_SET(STAT_D3D11_ShaderResourceChanges,  StateChanges[ED3D11StateChange::ShaderResource]);
    STAT_SET(STAT_D3D11_SamplerChanges,         StateChanges[ED3D11StateChange::Sampler]);
    STAT_SET(STAT_D3D11_UnorderedAccessChanges, StateChanges[ED3D11StateChange::UnorderedAccess]);
    STAT_SET(STAT_D3D11_ShaderConstantUploads,  StateChanges[ED3D11StateChange::ShaderConstantUpload]);
#endif

    Memory::Memzero(StateChanges, sizeof(StateChanges));
}

void FD3D11CommandContextState::SetGraphicsPipelineState(FD3D11GraphicsPipelineStateRHI* InGraphicsPipelineState)
{
    if (GraphicsState.PipelineState.Get() != InGraphicsPipelineState)
    {
        GraphicsState.PipelineState                = MakeSharedRef<FD3D11GraphicsPipelineStateRHI>(InGraphicsPipelineState);
        GraphicsState.bBindPipelineState           = true;
        CommonGraphicsState.bBindBlendState        = true;
        CommonGraphicsState.bBindDepthStencilState = true;
        CommonGraphicsState.bBindRasterizerState   = true;

        Memory::Memzero(CommonGraphicsState.DepthBias, sizeof(CommonGraphicsState.DepthBias));

        DirtyAllResources();
    }
}

void FD3D11CommandContextState::SetComputePipelineState(FD3D11ComputePipelineStateRHI* InComputePipelineState)
{
    if (ComputeState.PipelineState.Get() != InComputePipelineState)
    {
        ComputeState.PipelineState                    = MakeSharedRef<FD3D11ComputePipelineStateRHI>(InComputePipelineState);
        ComputeState.bBindPipelineState               = true;
        ComputeState.UnorderedAccessViewCache.bDirty = true;

        DirtyAllResources();
    }
}

void FD3D11CommandContextState::SetRenderTargets(FD3D11RenderTargetViewRHI* const* RenderTargets, uint32 NumRenderTargets, FD3D11DepthStencilViewRHI* DepthStencil)
{
    FD3D11RenderTargetCache& RenderTargetCache = CommonGraphicsState.RenderTargetCache;

    ID3D11DepthStencilView* D3D11DepthStencilView = DepthStencil ? DepthStencil->GetD3D11View() : nullptr;
    if (RenderTargetCache.DepthStencilView != D3D11DepthStencilView)
    {
        RenderTargetCache.DepthStencilView      = MakeComPtr<ID3D11DepthStencilView>(D3D11DepthStencilView);
        RenderTargetCache.DepthStencilRange     = DepthStencil ? DepthStencil->GetSubresourceRange() : FD3D11SubresourceRange();
        RenderTargetCache.bDepthStencilReadOnly = DepthStencil ? DepthStencil->IsReadOnly() : false;
        CommonGraphicsState.bBindRenderTargets  = true;
    }

    CHECK(NumRenderTargets <= D3D11_MAX_RENDER_TARGET_COUNT);
    if (RenderTargetCache.NumRenderTargets != NumRenderTargets)
    {
        RenderTargetCache.NumRenderTargets     = NumRenderTargets;
        CommonGraphicsState.bBindRenderTargets = true;
    }

    for (uint32 Index = 0; Index < NumRenderTargets; Index++)
    {
        FD3D11RenderTargetViewRHI* RenderTargetView = RenderTargets[Index];

        ID3D11RenderTargetView* D3D11RenderTargetView = RenderTargetView ? RenderTargetView->GetD3D11View() : nullptr;
        if (RenderTargetCache.RenderTargetViews[Index] != D3D11RenderTargetView)
        {
            RenderTargetCache.RenderTargetViews[Index]  = MakeComPtr<ID3D11RenderTargetView>(D3D11RenderTargetView);
            RenderTargetCache.RenderTargetRanges[Index] = RenderTargetView ? RenderTargetView->GetSubresourceRange() : FD3D11SubresourceRange();
            CommonGraphicsState.bBindRenderTargets      = true;
        }
    }
}

void FD3D11CommandContextState::SetViewports(const D3D11_VIEWPORT* Viewports, uint32 NumViewports)
{
    CHECK(NumViewports <= D3D11_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ViewportArraySize = sizeof(D3D11_VIEWPORT) * NumViewports;
    if (CommonGraphicsState.NumViewports != NumViewports || Memory::Memcmp(CommonGraphicsState.Viewports, Viewports, ViewportArraySize) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.Viewports, Viewports, ViewportArraySize);

        CommonGraphicsState.NumViewports   = NumViewports;
        CommonGraphicsState.bBindViewports = true;
    }
}

void FD3D11CommandContextState::SetScissorRects(const D3D11_RECT* ScissorRects, uint32 NumScissorRects)
{
    CHECK(NumScissorRects <= D3D11_MAX_VIEWPORT_AND_SCISSORRECT_COUNT);

    const uint32 ScissorRectArraySize = sizeof(D3D11_RECT) * NumScissorRects;
    if (CommonGraphicsState.NumScissorRects != NumScissorRects || Memory::Memcmp(CommonGraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.ScissorRects, ScissorRects, ScissorRectArraySize);

        CommonGraphicsState.NumScissorRects   = NumScissorRects;
        CommonGraphicsState.bBindScissorRects = true;
    }
}

void FD3D11CommandContextState::SetBlendFactor(const float BlendFactor[4])
{
    if (Memory::Memcmp(CommonGraphicsState.BlendFactor, BlendFactor, sizeof(CommonGraphicsState.BlendFactor)) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.BlendFactor, BlendFactor, sizeof(CommonGraphicsState.BlendFactor));
        CommonGraphicsState.bBindBlendState = true;
    }
}

void FD3D11CommandContextState::SetStencilRef(uint32 InStencilRef)
{
    if (CommonGraphicsState.StencilRef != InStencilRef)
    {
        CommonGraphicsState.StencilRef             = InStencilRef;
        CommonGraphicsState.bBindDepthStencilState = true;
    }
}

void FD3D11CommandContextState::SetDepthBias(float InDepthBias, float InDepthBiasClamp, float InSlopeScaledDepthBias)
{
    const float NewValues[3] =
    {
        InDepthBias,
        InDepthBiasClamp,
        InSlopeScaledDepthBias
    };

    if (Memory::Memcmp(CommonGraphicsState.DepthBias, NewValues, sizeof(NewValues)) != 0)
    {
        Memory::Memcpy(CommonGraphicsState.DepthBias, NewValues, sizeof(NewValues));
        CommonGraphicsState.bBindRasterizerState = true;
    }
}

void FD3D11CommandContextState::SetVertexBuffer(FD3D11BufferRHI* VertexBuffer, uint32 VertexBufferSlot)
{
    CHECK(VertexBufferSlot < D3D11_MAX_VERTEX_BUFFER_SLOTS);

    ID3D11Buffer* D3D11Buffer = VertexBuffer ? VertexBuffer->GetD3D11Buffer() : nullptr;
    const UINT    Stride      = VertexBuffer ? VertexBuffer->GetDesc().Stride : 0;

    FD3D11VertexBufferCache& VertexBufferCache = GraphicsState.VertexBufferCache;
    if (VertexBufferCache.VertexBuffers[VertexBufferSlot] != D3D11Buffer || VertexBufferCache.Strides[VertexBufferSlot] != Stride)
    {
        VertexBufferCache.VertexBuffers[VertexBufferSlot] = D3D11Buffer;
        VertexBufferCache.Strides[VertexBufferSlot]       = Stride;
        VertexBufferCache.NumVertexBuffers = Math::Max<uint32>(VertexBufferCache.NumVertexBuffers, VertexBufferSlot + 1);
        GraphicsState.bBindVertexBuffers   = true;
    }
}

void FD3D11CommandContextState::SetIndexBuffer(FD3D11BufferRHI* IndexBuffer, DXGI_FORMAT IndexFormat)
{
    ID3D11Buffer* D3D11Buffer = IndexBuffer ? IndexBuffer->GetD3D11Buffer() : nullptr;

    FD3D11IndexBufferCache& IndexBufferCache = GraphicsState.IndexBufferCache;
    if (IndexBufferCache.IndexBuffer != D3D11Buffer || IndexBufferCache.IndexFormat != IndexFormat)
    {
        IndexBufferCache.IndexBuffer   = D3D11Buffer;
        IndexBufferCache.IndexFormat   = IndexFormat;
        GraphicsState.bBindIndexBuffer = true;
    }
}

void FD3D11CommandContextState::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    FD3D11StreamOutputCache& StreamOutputCache = GraphicsState.StreamOutputCache;
    StreamOutputCache.Clear();

    const uint32 NumBuffers = Math::Min<uint32>(static_cast<uint32>(Buffers.Size()), D3D11_MAX_STREAM_OUTPUT_BUFFER_COUNT);
    for (uint32 Index = 0; Index < NumBuffers; ++Index)
    {
        FD3D11BufferRHI* D3D11Buffer = FD3D11DeviceRHI::ResourceCast(Buffers[Index]);
        StreamOutputCache.Buffers[Index] = D3D11Buffer ? D3D11Buffer->GetD3D11Buffer() : nullptr;
        StreamOutputCache.Offsets[Index] = Offsets ? static_cast<UINT>(Offsets[Index]) : 0;
    }

    GraphicsState.bBindStreamOutputTargets = true;
}

void FD3D11CommandContextState::SetSRV(FD3D11ShaderResourceViewRHI* ShaderResourceView, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    ID3D11ShaderResourceView* D3D11View = ShaderResourceView ? ShaderResourceView->GetD3D11View() : nullptr;

    FD3D11ShaderResourceViewCache& SRVCache = CommonState.ShaderResourceViewCache;
    if (SRVCache.ResourceViews[ShaderStage][ResourceIndex] != D3D11View)
    {
        SRVCache.ResourceViews[ShaderStage][ResourceIndex] = MakeComPtr<ID3D11ShaderResourceView>(D3D11View);
        SRVCache.Ranges[ShaderStage][ResourceIndex]        = ShaderResourceView ? ShaderResourceView->GetSubresourceRange() : FD3D11SubresourceRange();
        SRVCache.Dimensions[ShaderStage][ResourceIndex]    = ShaderResourceView ? ShaderResourceView->GetViewDimension() : D3D11_SRV_DIMENSION_UNKNOWN;
        SRVCache.NumViews[ShaderStage] = Math::Max<uint8>(SRVCache.NumViews[ShaderStage], static_cast<uint8>(ResourceIndex + 1));
        SRVCache.DirtyResources(ShaderStage);
    }
}

void FD3D11CommandContextState::SetUAV(FD3D11UnorderedAccessViewRHI* UnorderedAccessView, uint32 ResourceIndex)
{
    ID3D11UnorderedAccessView* D3D11View = UnorderedAccessView ? UnorderedAccessView->GetD3D11View() : nullptr;

    FD3D11UnorderedAccessViewCache& UAVCache = ComputeState.UnorderedAccessViewCache;
    if (UAVCache.UnorderedAccessViews[ResourceIndex] != D3D11View)
    {
        UAVCache.UnorderedAccessViews[ResourceIndex] = MakeComPtr<ID3D11UnorderedAccessView>(D3D11View);
        UAVCache.Ranges[ResourceIndex]               = UnorderedAccessView ? UnorderedAccessView->GetSubresourceRange() : FD3D11SubresourceRange();
        UAVCache.NumViews = Math::Max<uint32>(UAVCache.NumViews, ResourceIndex + 1);
        UAVCache.bDirty   = true;
    }
}

void FD3D11CommandContextState::SetCBV(FD3D11BufferRHI* Buffer, EShaderVisibility::Type ShaderStage, uint32 ResourceIndex)
{
    ID3D11Buffer* D3D11Buffer = Buffer ? Buffer->GetD3D11Buffer() : nullptr;

    FD3D11ConstantBufferCache& CBVCache = CommonState.ConstantBufferCache;
    if (CBVCache.ConstantBuffers[ShaderStage][ResourceIndex] != D3D11Buffer)
    {
        CBVCache.ConstantBuffers[ShaderStage][ResourceIndex] = D3D11Buffer;
        CBVCache.NumBuffers[ShaderStage] = Math::Max<uint8>(CBVCache.NumBuffers[ShaderStage], static_cast<uint8>(ResourceIndex + 1));
        CBVCache.DirtyResources(ShaderStage);
    }
}

void FD3D11CommandContextState::SetSampler(FD3D11SamplerStateRHI* SamplerState, EShaderVisibility::Type ShaderStage, uint32 SamplerIndex)
{
    ID3D11SamplerState* D3D11SamplerState = SamplerState ? SamplerState->GetD3D11SamplerState() : nullptr;

    FD3D11SamplerStateCache& SamplerCache = CommonState.SamplerStateCache;
    if (SamplerCache.SamplerStates[ShaderStage][SamplerIndex] != D3D11SamplerState)
    {
        SamplerCache.SamplerStates[ShaderStage][SamplerIndex] = D3D11SamplerState;
        SamplerCache.NumSamplers[ShaderStage] = Math::Max<uint8>(SamplerCache.NumSamplers[ShaderStage], static_cast<uint8>(SamplerIndex + 1));
        SamplerCache.DirtyResources(ShaderStage);
    }
}

void FD3D11CommandContextState::SetShaderConstants(EShaderStage ShaderStage, const uint32* ShaderConstants, uint32 NumShaderConstants)
{
    CHECK(NumShaderConstants <= D3D11_MAX_32BIT_SHADER_CONSTANTS_COUNT);

    const EShaderConstantsPipeline::Type Pipeline = GetShaderConstantsPipeline(ShaderStage);

    FD3D11ShaderConstantsCache& ConstantCache = CommonState.ShaderConstantsCache[Pipeline];
    if (NumShaderConstants != ConstantCache.NumConstants || Memory::Memcmp(ShaderConstants, ConstantCache.Constants, sizeof(uint32) * NumShaderConstants) != 0)
    {
        Memory::Memcpy(ConstantCache.Constants, ShaderConstants, sizeof(uint32) * NumShaderConstants);
        ConstantCache.NumConstants = NumShaderConstants;

        DirtyShaderConstants(Pipeline);
    }
}

ID3D11RasterizerState* FD3D11CommandContextState::GetRasterizerState(FD3D11GraphicsPipelineStateRHI* PipelineState)
{
    FD3D11RasterizerStateRHI* RasterizerState = PipelineState->GetRasterizerState();

    const FRHIRasterizerStateDesc& RasterizerDesc = RasterizerState->GetDesc();
    if (!RasterizerDesc.bEnableDepthBias)
    {
        return RasterizerState->GetD3D11State();
    }

    FRHIRasterizerStateDesc VariantDesc = RasterizerDesc;
    VariantDesc.DepthBias            = CommonGraphicsState.DepthBias[0];
    VariantDesc.DepthBiasClamp       = CommonGraphicsState.DepthBias[1];
    VariantDesc.SlopeScaledDepthBias = CommonGraphicsState.DepthBias[2];

    if (VariantDesc == RasterizerDesc)
    {
        return RasterizerState->GetD3D11State();
    }

    FD3D11RasterizerStateRHI* CurrentVariant = CommonGraphicsState.DepthBiasRasterizerState.Get();
    if (!CurrentVariant || CurrentVariant->GetDesc() != VariantDesc)
    {
        CommonGraphicsState.DepthBiasRasterizerState = static_cast<FD3D11RasterizerStateRHI*>(FD3D11DeviceRHI::Get()->CreateRasterizerState(VariantDesc));
        if (!CommonGraphicsState.DepthBiasRasterizerState)
        {
            return RasterizerState->GetD3D11State();
        }
    }

    return CommonGraphicsState.DepthBiasRasterizerState->GetD3D11State();
}

void FD3D11CommandContextState::BindRenderTargets()
{
    FD3D11RenderTargetCache& RenderTargetCache = CommonGraphicsState.RenderTargetCache;
    for (uint32 Index = 0; Index < RenderTargetCache.NumRenderTargets; Index++)
    {
        UnbindShaderResourceViews(RenderTargetCache.RenderTargetRanges[Index]);
        UnbindUnorderedAccessViews(RenderTargetCache.RenderTargetRanges[Index]);

        RenderTargetCache.BoundRenderTargetViews[Index]  = RenderTargetCache.RenderTargetViews[Index].Get();
        RenderTargetCache.BoundRenderTargetRanges[Index] = RenderTargetCache.RenderTargetRanges[Index];
    }

    if (!RenderTargetCache.bDepthStencilReadOnly)
    {
        UnbindShaderResourceViews(RenderTargetCache.DepthStencilRange);
    }

    UnbindUnorderedAccessViews(RenderTargetCache.DepthStencilRange);

    RenderTargetCache.BoundDepthStencilView      = RenderTargetCache.DepthStencilView.Get();
    RenderTargetCache.BoundDepthStencilRange     = RenderTargetCache.DepthStencilRange;
    RenderTargetCache.bBoundDepthStencilReadOnly = RenderTargetCache.bDepthStencilReadOnly;
    RenderTargetCache.NumBoundRenderTargets  = RenderTargetCache.NumRenderTargets;

    Context.GetD3D11Context()->OMSetRenderTargets(RenderTargetCache.NumBoundRenderTargets, RenderTargetCache.BoundRenderTargetViews, RenderTargetCache.BoundDepthStencilView);
    StateChanges[ED3D11StateChange::OutputMerger]++;

    // Shader resources hidden by the previous targets can be bound again
    for (int32 Index = EShaderVisibility::Vertex; Index <= EShaderVisibility::Pixel; Index++)
    {
        CommonState.ShaderResourceViewCache.DirtyResources(static_cast<EShaderVisibility::Type>(Index));
    }
}

void FD3D11CommandContextState::BindUnorderedAccessViews(const FD3D11Shader* Shader)
{
    FD3D11UnorderedAccessViewCache& UAVCache = ComputeState.UnorderedAccessViewCache;
    if (!UAVCache.bDirty)
    {
        return;
    }

    const FD3D11ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();

    const uint32 NumViews = Math::Max(UAVCache.NumViews, UAVCache.NumBoundViews);
    for (uint32 Index = 0; Index < NumViews; Index++)
    {
        ID3D11UnorderedAccessView*   View  = BindingInfo.IsUnorderedAccessViewDeclared(Index) ? UAVCache.UnorderedAccessViews[Index].Get() : nullptr;
        const FD3D11SubresourceRange Range = View ? UAVCache.Ranges[Index] : FD3D11SubresourceRange();

        UnbindShaderResourceViews(Range);
        UnbindRenderTargets(Range);

        UAVCache.BoundViews[Index]  = View;
        UAVCache.BoundRanges[Index] = Range;
    }

    UAVCache.NumBoundViews = NumViews;
    UAVCache.bDirty        = false;

    Context.GetD3D11Context()->CSSetUnorderedAccessViews(0, UAVCache.NumBoundViews, UAVCache.BoundViews, nullptr);
    StateChanges[ED3D11StateChange::UnorderedAccess]++;

    // Shader resources hidden by the previous UAVs can be bound again
    CommonState.ShaderResourceViewCache.DirtyResources(EShaderVisibility::Compute);
}

void FD3D11CommandContextState::BindShaderConstants(EShaderConstantsPipeline::Type Pipeline)
{
    const FD3D11ShaderConstantsCache& ConstantCache = CommonState.ShaderConstantsCache[Pipeline];
    if (ConstantCache.NumConstants == 0)
    {
        return;
    }

    ID3D11DeviceContext* D3D11Context = Context.GetD3D11Context();
    ID3D11Buffer*        D3D11Buffer  = CommonState.ShaderConstantsBuffers[Pipeline].Get();

    D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
    const HRESULT Result = D3D11Context->Map(D3D11Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11CommandContextState]: FAILED to map the ShaderConstants buffer (0x%08X)", static_cast<uint32>(Result));
        return;
    }

    Memory::Memcpy(MappedSubresource.pData, ConstantCache.Constants, sizeof(uint32) * ConstantCache.NumConstants);
    D3D11Context->Unmap(D3D11Buffer, 0);
    StateChanges[ED3D11StateChange::ShaderConstantUpload]++;
}

void FD3D11CommandContextState::BindConstantBuffers(EShaderVisibility::Type ShaderStage, const FD3D11Shader* Shader)
{
    FD3D11ConstantBufferCache& CBVCache = CommonState.ConstantBufferCache;
    if (!CBVCache.IsResourcesDirty(ShaderStage))
    {
        return;
    }

    ID3D11Buffer* ConstantBuffers[D3D11_MAX_CONSTANT_BUFFERS];
    Memory::Memcpy(ConstantBuffers, CBVCache.ConstantBuffers[ShaderStage], sizeof(ConstantBuffers));

    uint32 NumBuffers = CBVCache.NumBuffers[ShaderStage];

    const int32 ShaderConstantsSlot = Shader ? Shader->GetBindingInfo().ShaderConstantsSlot : -1;
    if (ShaderConstantsSlot >= 0)
    {
        const EShaderConstantsPipeline::Type Pipeline = ShaderStage == EShaderVisibility::Compute ? EShaderConstantsPipeline::Compute : EShaderConstantsPipeline::Graphics;
        ConstantBuffers[ShaderConstantsSlot] = CommonState.ShaderConstantsBuffers[Pipeline].Get();
        NumBuffers = Math::Max<uint32>(NumBuffers, ShaderConstantsSlot + 1);
    }

    ID3D11Buffer** BoundBuffers = CBVCache.BoundBuffers[ShaderStage];
    const uint32   NumSlots     = Math::Max<uint32>(NumBuffers, CBVCache.NumBoundBuffers[ShaderStage]);

    uint32 FirstSlot = 0;
    uint32 NumSet    = 0;
    if (FindChangedRange(BoundBuffers, ConstantBuffers, NumSlots, FirstSlot, NumSet))
    {
        Memory::Memcpy(&BoundBuffers[FirstSlot], &ConstantBuffers[FirstSlot], sizeof(ID3D11Buffer*) * NumSet);
        InternalSetConstantBuffers(ShaderStage, FirstSlot, NumSet, &BoundBuffers[FirstSlot]);
    }

    CBVCache.NumBoundBuffers[ShaderStage] = static_cast<uint8>(NumBuffers);
    CBVCache.ClearResourcesDirty(ShaderStage);
}

void FD3D11CommandContextState::BindShaderResourceViews(EShaderVisibility::Type ShaderStage, const FD3D11Shader* Shader)
{
    FD3D11ShaderResourceViewCache& SRVCache = CommonState.ShaderResourceViewCache;
    if (!SRVCache.IsResourcesDirty(ShaderStage))
    {
        return;
    }

    const FD3D11RenderTargetCache&        RenderTargetCache = CommonGraphicsState.RenderTargetCache;
    const FD3D11UnorderedAccessViewCache& UAVCache          = ComputeState.UnorderedAccessViewCache;
    const bool                            bIsCompute        = ShaderStage == EShaderVisibility::Compute;

    const uint32 NumViews = Math::Max(SRVCache.NumViews[ShaderStage], SRVCache.NumBoundViews[ShaderStage]);

    uint32 FirstChangedView = NumViews;
    uint32 LastChangedView  = 0;
    for (uint32 Index = 0; Index < NumViews; Index++)
    {
        ID3D11ShaderResourceView*     View  = SRVCache.ResourceViews[ShaderStage][Index].Get();
        const FD3D11SubresourceRange& Range = SRVCache.Ranges[ShaderStage][Index];

        // Only the registers the shader declares get a view, and only one of the declared dimension
        if (View && !(Shader && IsViewDimensionCompatible(Shader->GetBindingInfo().GetShaderResourceViewDimension(Index), SRVCache.Dimensions[ShaderStage][Index])))
        {
            View = nullptr;
        }

        // Outputs of the pipeline that is about to run win, outputs left over from the other pipeline yield
        if (View && RenderTargetCache.IsBoundForWrite(Range))
        {
            if (bIsCompute)
            {
                UnbindRenderTargets(Range);
            }
            else
            {
                View = nullptr;
            }
        }

        if (View && UAVCache.IsBound(Range))
        {
            if (bIsCompute)
            {
                View = nullptr;
            }
            else
            {
                UnbindUnorderedAccessViews(Range);
            }
        }

        if (SRVCache.BoundViews[ShaderStage][Index] != View)
        {
            SRVCache.BoundViews[ShaderStage][Index]  = View;
            SRVCache.BoundRanges[ShaderStage][Index] = View ? Range : FD3D11SubresourceRange();
            FirstChangedView = Math::Min(FirstChangedView, Index);
            LastChangedView  = Math::Max(LastChangedView, Index);
        }
    }

    if (FirstChangedView < NumViews)
    {
        InternalSetShaderResources(ShaderStage, FirstChangedView, LastChangedView - FirstChangedView + 1, &SRVCache.BoundViews[ShaderStage][FirstChangedView]);
    }

    SRVCache.NumBoundViews[ShaderStage] = SRVCache.NumViews[ShaderStage];
    SRVCache.ClearResourcesDirty(ShaderStage);
}

void FD3D11CommandContextState::BindSamplers(EShaderVisibility::Type ShaderStage, const TArray<FD3D11StaticSampler>& StaticSamplers)
{
    FD3D11SamplerStateCache& SamplerCache = CommonState.SamplerStateCache;
    if (!SamplerCache.IsResourcesDirty(ShaderStage))
    {
        return;
    }

    ID3D11SamplerState* SamplerStates[D3D11_MAX_SAMPLER_STATES];
    Memory::Memcpy(SamplerStates, SamplerCache.SamplerStates[ShaderStage], sizeof(SamplerStates));

    uint32 NumSamplers = SamplerCache.NumSamplers[ShaderStage];

    for (const FD3D11StaticSampler& StaticSampler : StaticSamplers)
    {
        if (StaticSampler.Visibility == ShaderStage)
        {
            CHECK(StaticSampler.Register < D3D11_MAX_SAMPLER_STATES);
            SamplerStates[StaticSampler.Register] = StaticSampler.Sampler.Get();
            NumSamplers = Math::Max<uint32>(NumSamplers, StaticSampler.Register + 1);
        }
    }

    ID3D11SamplerState** BoundSamplerStates = SamplerCache.BoundSamplerStates[ShaderStage];
    const uint32         NumSlots           = Math::Max<uint32>(NumSamplers, SamplerCache.NumBoundSamplers[ShaderStage]);

    uint32 FirstSlot = 0;
    uint32 NumSet    = 0;
    if (FindChangedRange(BoundSamplerStates, SamplerStates, NumSlots, FirstSlot, NumSet))
    {
        Memory::Memcpy(&BoundSamplerStates[FirstSlot], &SamplerStates[FirstSlot], sizeof(ID3D11SamplerState*) * NumSet);
        InternalSetSamplers(ShaderStage, FirstSlot, NumSet, &BoundSamplerStates[FirstSlot]);
    }

    SamplerCache.NumBoundSamplers[ShaderStage] = static_cast<uint8>(NumSamplers);
    SamplerCache.ClearResourcesDirty(ShaderStage);
}

void FD3D11CommandContextState::UnbindRenderTargets(const FD3D11SubresourceRange& Range)
{
    if (!Range.Resource)
    {
        return;
    }

    FD3D11RenderTargetCache& RenderTargetCache = CommonGraphicsState.RenderTargetCache;

    bool bUnbound = false;
    for (uint32 Index = 0; Index < RenderTargetCache.NumBoundRenderTargets; Index++)
    {
        if (RenderTargetCache.BoundRenderTargetRanges[Index].Overlaps(Range))
        {
            RenderTargetCache.BoundRenderTargetViews[Index]  = nullptr;
            RenderTargetCache.BoundRenderTargetRanges[Index] = FD3D11SubresourceRange();
            bUnbound = true;
        }
    }

    if (RenderTargetCache.BoundDepthStencilRange.Overlaps(Range))
    {
        RenderTargetCache.BoundDepthStencilView      = nullptr;
        RenderTargetCache.BoundDepthStencilRange     = FD3D11SubresourceRange();
        RenderTargetCache.bBoundDepthStencilReadOnly = false;
        bUnbound = true;
    }

    if (bUnbound)
    {
        Context.GetD3D11Context()->OMSetRenderTargets(RenderTargetCache.NumBoundRenderTargets, RenderTargetCache.BoundRenderTargetViews, RenderTargetCache.BoundDepthStencilView);
        StateChanges[ED3D11StateChange::OutputMerger]++;
        CommonGraphicsState.bBindRenderTargets = true;
    }
}

void FD3D11CommandContextState::UnbindUnorderedAccessViews(const FD3D11SubresourceRange& Range)
{
    if (!Range.Resource)
    {
        return;
    }

    FD3D11UnorderedAccessViewCache& UAVCache = ComputeState.UnorderedAccessViewCache;
    for (uint32 Index = 0; Index < UAVCache.NumBoundViews; Index++)
    {
        if (UAVCache.BoundRanges[Index].Overlaps(Range))
        {
            ID3D11UnorderedAccessView* NullView = nullptr;
            Context.GetD3D11Context()->CSSetUnorderedAccessViews(Index, 1, &NullView, nullptr);
            StateChanges[ED3D11StateChange::UnorderedAccess]++;

            UAVCache.BoundViews[Index]  = nullptr;
            UAVCache.BoundRanges[Index] = FD3D11SubresourceRange();
            UAVCache.bDirty = true;
        }
    }
}

void FD3D11CommandContextState::UnbindShaderResourceViews(const FD3D11SubresourceRange& Range)
{
    if (!Range.Resource)
    {
        return;
    }

    FD3D11ShaderResourceViewCache& SRVCache = CommonState.ShaderResourceViewCache;
    for (int32 StageIndex = 0; StageIndex < EShaderVisibility::Count; StageIndex++)
    {
        const EShaderVisibility::Type ShaderStage = static_cast<EShaderVisibility::Type>(StageIndex);
        for (uint32 Index = 0; Index < SRVCache.NumBoundViews[ShaderStage]; Index++)
        {
            if (SRVCache.BoundRanges[ShaderStage][Index].Overlaps(Range))
            {
                ID3D11ShaderResourceView* NullView = nullptr;
                InternalSetShaderResources(ShaderStage, Index, 1, &NullView);

                SRVCache.BoundViews[ShaderStage][Index]  = nullptr;
                SRVCache.BoundRanges[ShaderStage][Index] = FD3D11SubresourceRange();
                SRVCache.DirtyResources(ShaderStage);
            }
        }
    }
}

void FD3D11CommandContextState::InternalSetConstantBuffers(EShaderVisibility::Type ShaderStage, uint32 StartSlot, uint32 NumBuffers, ID3D11Buffer* const* Buffers)
{
    ID3D11DeviceContext* D3D11Context = Context.GetD3D11Context();
    switch (ShaderStage)
    {
        case EShaderVisibility::Vertex:   D3D11Context->VSSetConstantBuffers(StartSlot, NumBuffers, Buffers); break;
        case EShaderVisibility::Hull:     D3D11Context->HSSetConstantBuffers(StartSlot, NumBuffers, Buffers); break;
        case EShaderVisibility::Domain:   D3D11Context->DSSetConstantBuffers(StartSlot, NumBuffers, Buffers); break;
        case EShaderVisibility::Geometry: D3D11Context->GSSetConstantBuffers(StartSlot, NumBuffers, Buffers); break;
        case EShaderVisibility::Pixel:    D3D11Context->PSSetConstantBuffers(StartSlot, NumBuffers, Buffers); break;
        case EShaderVisibility::Compute:  D3D11Context->CSSetConstantBuffers(StartSlot, NumBuffers, Buffers); break;
        default:                          break;
    }

    StateChanges[ED3D11StateChange::ConstantBuffer]++;
}

void FD3D11CommandContextState::InternalSetShaderResources(EShaderVisibility::Type ShaderStage, uint32 StartSlot, uint32 NumViews, ID3D11ShaderResourceView* const* Views)
{
    ID3D11DeviceContext* D3D11Context = Context.GetD3D11Context();
    switch (ShaderStage)
    {
        case EShaderVisibility::Vertex:   D3D11Context->VSSetShaderResources(StartSlot, NumViews, Views); break;
        case EShaderVisibility::Hull:     D3D11Context->HSSetShaderResources(StartSlot, NumViews, Views); break;
        case EShaderVisibility::Domain:   D3D11Context->DSSetShaderResources(StartSlot, NumViews, Views); break;
        case EShaderVisibility::Geometry: D3D11Context->GSSetShaderResources(StartSlot, NumViews, Views); break;
        case EShaderVisibility::Pixel:    D3D11Context->PSSetShaderResources(StartSlot, NumViews, Views); break;
        case EShaderVisibility::Compute:  D3D11Context->CSSetShaderResources(StartSlot, NumViews, Views); break;
        default:                          break;
    }

    StateChanges[ED3D11StateChange::ShaderResource]++;
}

void FD3D11CommandContextState::InternalSetSamplers(EShaderVisibility::Type ShaderStage, uint32 StartSlot, uint32 NumSamplers, ID3D11SamplerState* const* Samplers)
{
    ID3D11DeviceContext* D3D11Context = Context.GetD3D11Context();
    switch (ShaderStage)
    {
        case EShaderVisibility::Vertex:   D3D11Context->VSSetSamplers(StartSlot, NumSamplers, Samplers); break;
        case EShaderVisibility::Hull:     D3D11Context->HSSetSamplers(StartSlot, NumSamplers, Samplers); break;
        case EShaderVisibility::Domain:   D3D11Context->DSSetSamplers(StartSlot, NumSamplers, Samplers); break;
        case EShaderVisibility::Geometry: D3D11Context->GSSetSamplers(StartSlot, NumSamplers, Samplers); break;
        case EShaderVisibility::Pixel:    D3D11Context->PSSetSamplers(StartSlot, NumSamplers, Samplers); break;
        case EShaderVisibility::Compute:  D3D11Context->CSSetSamplers(StartSlot, NumSamplers, Samplers); break;
        default:                          break;
    }

    StateChanges[ED3D11StateChange::Sampler]++;
}

void FD3D11CommandContextState::DirtyShaderConstants(EShaderConstantsPipeline::Type Pipeline)
{
    if (Pipeline == EShaderConstantsPipeline::Graphics)
    {
        GraphicsState.bBindShaderConstants = true;
    }
    else
    {
        ComputeState.bBindShaderConstants = true;
    }
}

void FD3D11CommandContextState::DirtyAllResources()
{
    CommonState.ConstantBufferCache.DirtyResourcesAll();
    CommonState.ShaderResourceViewCache.DirtyResourcesAll();
    CommonState.SamplerStateCache.DirtyResourcesAll();
}
