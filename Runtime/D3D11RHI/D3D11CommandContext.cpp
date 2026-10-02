#include "D3D11RHI/D3D11CommandContext.h"
#include "D3D11RHI/D3D11Buffer.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11Fence.h"
#include "D3D11RHI/D3D11RHI.h"
#include "D3D11RHI/D3D11SwapChain.h"
#include "D3D11RHI/D3D11Texture.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

static UINT GetDepthStencilClearFlags(ID3D11DepthStencilView* View)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC ViewDesc;
    View->GetDesc(&ViewDesc);
    return D3D11_CLEAR_DEPTH | (IsStencilFormat(ViewDesc.Format) ? D3D11_CLEAR_STENCIL : 0);
}

static bool BoxesOverlap(const D3D11_BOX& First, const D3D11_BOX& Second)
{
    return First.left < Second.right && Second.left < First.right && First.top < Second.bottom && Second.top < First.bottom && First.front < Second.back && Second.front < First.back;
}

static void ReportGraphicsUnorderedAccessViews()
{
    static bool bReported = false;
    if (!bReported)
    {
        D3D11_ERROR("[FD3D11CommandContext]: D3D11RHI does not support UnorderedAccessViews in graphics shaders yet");
        bReported = true;
    }
}

FD3D11CommandContext::FD3D11CommandContext(FD3D11Device* InDevice)
    : IRHICommandContext()
    , FD3D11DeviceChild(InDevice)
    , ContextState(InDevice, *this)
    , Annotation(nullptr)
    , ReadbackTextures()
    , PendingReadbacks()
    , TimestampDisjointQuery(nullptr)
{
}

FD3D11CommandContext::~FD3D11CommandContext() = default;

bool FD3D11CommandContext::Initialize()
{
    if (!ContextState.Initialize())
    {
        return false;
    }

    GetD3D11Context()->QueryInterface(IID_PPV_ARGS(&Annotation));
    return true;
}

ID3D11DeviceContext* FD3D11CommandContext::GetD3D11Context() const
{
    return GetDevice()->GetD3D11Context();
}

void FD3D11CommandContext::BeginFrame()
{
    FD3D11DeviceRHI::Get()->BeginFrame();
}

void FD3D11CommandContext::EndFrame()
{
    FD3D11DeviceRHI::Get()->EndFrame();
}

void FD3D11CommandContext::StartContext()
{
}

void FD3D11CommandContext::FinishContext()
{
    ResolvePendingReadbacks();
    EndTimestampDisjointQuery();
}

void FD3D11CommandContext::BeginQuery(FRHIQuery* Query)
{
    FD3D11QueryRHI* D3D11Query = FD3D11DeviceRHI::ResourceCast(Query);
    CHECK(D3D11Query != nullptr);

    if (D3D11Query->GetType() == EQueryType::Timestamp)
    {
        D3D11_ERROR("BeginQuery is not supported for this query type");
        return;
    }

    D3D11Query->bResultReady.Store(0);
    D3D11Query->Query->Begin(GetD3D11Context());
}

void FD3D11CommandContext::EndQuery(FRHIQuery* Query)
{
    FD3D11QueryRHI* D3D11Query = FD3D11DeviceRHI::ResourceCast(Query);
    CHECK(D3D11Query != nullptr);

    D3D11Query->Query->End(GetD3D11Context());
}

void FD3D11CommandContext::QueryTimestamp(FRHIQuery* Query)
{
    FD3D11QueryRHI* D3D11Query = FD3D11DeviceRHI::ResourceCast(Query);
    CHECK(D3D11Query != nullptr);

    if (!TimestampDisjointQuery)
    {
        FD3D11QueryRef NewDisjointQuery = new FD3D11Query(GetDevice(), D3D11_QUERY_TIMESTAMP_DISJOINT);
        if (!NewDisjointQuery->Initialize())
        {
            return;
        }

        NewDisjointQuery->SetDebugName("Timestamp Disjoint Query");
        NewDisjointQuery->Begin(GetD3D11Context());
        TimestampDisjointQuery = NewDisjointQuery;
    }

    D3D11Query->bResultReady.Store(0);
    D3D11Query->DisjointQuery = TimestampDisjointQuery;
    D3D11Query->Query->End(GetD3D11Context());
}

void FD3D11CommandContext::ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor)
{
    CHECK(RenderTargetView != nullptr);

    if (ID3D11RenderTargetView* D3D11View = static_cast<ID3D11RenderTargetView*>(RenderTargetView->GetRHINativeHandle()))
    {
        GetD3D11Context()->ClearRenderTargetView(D3D11View, ClearColor.XYZW);
    }
}

void FD3D11CommandContext::ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil)
{
    CHECK(DepthStencilView != nullptr);

    if (ID3D11DepthStencilView* D3D11View = static_cast<ID3D11DepthStencilView*>(DepthStencilView->GetRHINativeHandle()))
    {
        GetD3D11Context()->ClearDepthStencilView(D3D11View, GetDepthStencilClearFlags(D3D11View), Depth, Stencil);
    }
}

void FD3D11CommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor)
{
    FD3D11UnorderedAccessViewRHI* D3D11UnorderedAccessView = FD3D11DeviceRHI::ResourceCast(UnorderedAccessView);
    CHECK(D3D11UnorderedAccessView != nullptr);

    GetD3D11Context()->ClearUnorderedAccessViewFloat(D3D11UnorderedAccessView->GetD3D11View(), ClearColor.XYZW);
}

void FD3D11CommandContext::ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4])
{
    FD3D11UnorderedAccessViewRHI* D3D11UnorderedAccessView = FD3D11DeviceRHI::ResourceCast(UnorderedAccessView);
    CHECK(D3D11UnorderedAccessView != nullptr);

    GetD3D11Context()->ClearUnorderedAccessViewUint(D3D11UnorderedAccessView->GetD3D11View(), Values);
}

void FD3D11CommandContext::BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc)
{
    ID3D11DeviceContext*  D3D11Context  = GetD3D11Context();
    ID3D11DeviceContext1* D3D11Context1 = GetDevice()->GetD3D11Context1();

    FD3D11RenderTargetViewRHI* RenderTargetViews[D3D11_MAX_RENDER_TARGET_COUNT] = {};

    const uint32 NumRenderTargets = Math::Min<uint32>(BeginRenderPassDesc.NumRenderTargets, D3D11_MAX_RENDER_TARGET_COUNT);
    for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetAttachment& CurrentAttachment = BeginRenderPassDesc.RenderTargets[Index];

        RenderTargetViews[Index] = FD3D11DeviceRHI::ResourceCast(CurrentAttachment.View.Get());

        ID3D11RenderTargetView* D3D11View = RenderTargetViews[Index] ? RenderTargetViews[Index]->GetD3D11View() : nullptr;
        if (!D3D11View)
        {
            continue;
        }

        if (CurrentAttachment.LoadAction == EAttachmentLoadAction::Clear)
        {
            D3D11Context->ClearRenderTargetView(D3D11View, CurrentAttachment.ClearValue.RGBA);
        }
        else if (CurrentAttachment.LoadAction == EAttachmentLoadAction::DontCare && D3D11Context1)
        {
            D3D11Context1->DiscardView(D3D11View);
        }
    }

    const FRHIDepthStencilAttachment& CurrentDSAttachment = BeginRenderPassDesc.DepthStencilAttachment;

    FD3D11DepthStencilViewRHI* DepthStencilView = FD3D11DeviceRHI::ResourceCast(CurrentDSAttachment.View.Get());

    ID3D11DepthStencilView* D3D11DepthStencilView = DepthStencilView ? DepthStencilView->GetD3D11View() : nullptr;
    if (D3D11DepthStencilView)
    {
        if (CurrentDSAttachment.LoadAction == EAttachmentLoadAction::Clear)
        {
            const UINT ClearFlags = GetDepthStencilClearFlags(D3D11DepthStencilView);
            D3D11Context->ClearDepthStencilView(D3D11DepthStencilView, ClearFlags, CurrentDSAttachment.ClearValue.Depth, static_cast<UINT8>(CurrentDSAttachment.ClearValue.Stencil));
        }
        else if (CurrentDSAttachment.LoadAction == EAttachmentLoadAction::DontCare && D3D11Context1)
        {
            D3D11Context1->DiscardView(D3D11DepthStencilView);
        }
    }

    ContextState.SetRenderTargets(RenderTargetViews, NumRenderTargets, DepthStencilView);
}

void FD3D11CommandContext::EndRenderPass()
{
}

void FD3D11CommandContext::SetViewport(const FViewportRegion& ViewportRegion)
{
    D3D11_VIEWPORT Viewport = {};
    Viewport.Width    = ViewportRegion.Width;
    Viewport.Height   = ViewportRegion.Height;
    Viewport.TopLeftX = ViewportRegion.PositionX;
    Viewport.TopLeftY = ViewportRegion.PositionY;
    Viewport.MaxDepth = ViewportRegion.MaxDepth;
    Viewport.MinDepth = ViewportRegion.MinDepth;

    ContextState.SetViewports(&Viewport, 1);
}

void FD3D11CommandContext::SetScissorRect(const FScissorRegion& ScissorRegion)
{
    D3D11_RECT ScissorRect = {};
    ScissorRect.left   = LONG(ScissorRegion.PositionX);
    ScissorRect.right  = LONG(ScissorRegion.PositionX) + LONG(ScissorRegion.Width);
    ScissorRect.top    = LONG(ScissorRegion.PositionY);
    ScissorRect.bottom = LONG(ScissorRegion.PositionY) + LONG(ScissorRegion.Height);

    ContextState.SetScissorRects(&ScissorRect, 1);
}

void FD3D11CommandContext::SetBlendFactor(const Vector4& Color)
{
    ContextState.SetBlendFactor(Color.XYZW);
}

void FD3D11CommandContext::SetStencilRef(uint32 StencilRef)
{
    ContextState.SetStencilRef(StencilRef);
}

void FD3D11CommandContext::SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias)
{
}

void FD3D11CommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 Index = 0; Index < InVertexBuffers.Size(); ++Index)
    {
        FD3D11BufferRHI* D3DVertexBuffer = FD3D11DeviceRHI::ResourceCast(InVertexBuffers[Index]);
        ContextState.SetVertexBuffer(D3DVertexBuffer, BufferSlot + Index);
    }
}

void FD3D11CommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    FD3D11BufferRHI* D3DIndexBuffer = FD3D11DeviceRHI::ResourceCast(IndexBuffer);
    ContextState.SetIndexBuffer(D3DIndexBuffer, ConvertIndexFormat(IndexFormat));
}

void FD3D11CommandContext::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
}

void FD3D11CommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    FD3D11GraphicsPipelineStateRHI* GraphicsPipelineState = FD3D11DeviceRHI::ResourceCast(PipelineState);
    ContextState.SetGraphicsPipelineState(GraphicsPipelineState);
}

void FD3D11CommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
    FD3D11ComputePipelineStateRHI* ComputePipelineState = FD3D11DeviceRHI::ResourceCast(PipelineState);
    ContextState.SetComputePipelineState(ComputePipelineState);
}

void FD3D11CommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    MAYBE_UNUSED FD3D11Shader* D3D11Shader = GetD3D11Shader(Shader);
    CHECK(D3D11Shader != nullptr);

    ContextState.SetShaderConstants(Shader->GetShaderStage(), reinterpret_cast<const uint32*>(ShaderConstants), NumShaderConstants);
}

void FD3D11CommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex)
{
    FD3D11Shader* D3D11Shader = GetD3D11Shader(Shader);
    CHECK(D3D11Shader != nullptr);

    FD3D11ShaderResourceViewRHI* D3D11ShaderResourceView = FD3D11DeviceRHI::ResourceCast(ShaderResourceView);

    CHECK(RegisterIndex < D3D11_MAX_SHADER_RESOURCE_VIEWS);
    ContextState.SetSRV(D3D11ShaderResourceView, D3D11Shader->GetShaderVisibility(), RegisterIndex);
}

void FD3D11CommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex)
{
    FD3D11Shader* D3D11Shader = GetD3D11Shader(Shader);
    CHECK(D3D11Shader != nullptr);

    CHECK(RegisterIndex + InShaderResourceViews.Size() <= D3D11_MAX_SHADER_RESOURCE_VIEWS);
    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        FD3D11ShaderResourceViewRHI* D3D11ShaderResourceView = FD3D11DeviceRHI::ResourceCast(InShaderResourceViews[Index]);
        ContextState.SetSRV(D3D11ShaderResourceView, D3D11Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D11CommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex)
{
    FD3D11Shader* D3D11Shader = GetD3D11Shader(Shader);
    CHECK(D3D11Shader != nullptr);

    if (D3D11Shader->GetShaderVisibility() != EShaderVisibility::Compute)
    {
        ReportGraphicsUnorderedAccessViews();
        return;
    }

    FD3D11UnorderedAccessViewRHI* D3D11UnorderedAccessView = FD3D11DeviceRHI::ResourceCast(UnorderedAccessView);

    CHECK(RegisterIndex < D3D11_MAX_UNORDERED_ACCESS_VIEWS);
    ContextState.SetUAV(D3D11UnorderedAccessView, RegisterIndex);
}

void FD3D11CommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex)
{
    FD3D11Shader* D3D11Shader = GetD3D11Shader(Shader);
    CHECK(D3D11Shader != nullptr);

    if (D3D11Shader->GetShaderVisibility() != EShaderVisibility::Compute)
    {
        ReportGraphicsUnorderedAccessViews();
        return;
    }

    CHECK(RegisterIndex + InUnorderedAccessViews.Size() <= D3D11_MAX_UNORDERED_ACCESS_VIEWS);
    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        FD3D11UnorderedAccessViewRHI* D3D11UnorderedAccessView = FD3D11DeviceRHI::ResourceCast(InUnorderedAccessViews[Index]);
        ContextState.SetUAV(D3D11UnorderedAccessView, RegisterIndex + Index);
    }
}

void FD3D11CommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex)
{
    FD3D11Shader* D3D11Shader = GetD3D11Shader(Shader);
    CHECK(D3D11Shader != nullptr);

    FD3D11BufferRHI* D3D11Buffer = ConstantBuffer ? FD3D11DeviceRHI::ResourceCast(ConstantBuffer) : nullptr;

    CHECK(RegisterIndex < D3D11_MAX_CONSTANT_BUFFERS);
    ContextState.SetCBV(D3D11Buffer, D3D11Shader->GetShaderVisibility(), RegisterIndex);
}

void FD3D11CommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex)
{
    FD3D11Shader* D3D11Shader = GetD3D11Shader(Shader);
    CHECK(D3D11Shader != nullptr);
    CHECK(RegisterIndex + InConstantBuffers.Size() <= D3D11_MAX_CONSTANT_BUFFERS);

    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        FD3D11BufferRHI* D3D11Buffer = InConstantBuffers[Index] ? FD3D11DeviceRHI::ResourceCast(InConstantBuffers[Index]) : nullptr;
        ContextState.SetCBV(D3D11Buffer, D3D11Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D11CommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex)
{
    FD3D11Shader* D3D11Shader = GetD3D11Shader(Shader);
    CHECK(D3D11Shader != nullptr);

    FD3D11SamplerStateRHI* D3D11SamplerState = FD3D11DeviceRHI::ResourceCast(SamplerState);

    CHECK(RegisterIndex < D3D11_MAX_SAMPLER_STATES);
    ContextState.SetSampler(D3D11SamplerState, D3D11Shader->GetShaderVisibility(), RegisterIndex);
}

void FD3D11CommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex)
{
    FD3D11Shader* D3D11Shader = GetD3D11Shader(Shader);
    CHECK(D3D11Shader != nullptr);
    CHECK(RegisterIndex + InSamplerStates.Size() <= D3D11_MAX_SAMPLER_STATES);

    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        FD3D11SamplerStateRHI* D3D11SamplerState = FD3D11DeviceRHI::ResourceCast(InSamplerStates[Index]);
        ContextState.SetSampler(D3D11SamplerState, D3D11Shader->GetShaderVisibility(), RegisterIndex + Index);
    }
}

void FD3D11CommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)
{
    FD3D11BufferRHI* D3D11Buffer = FD3D11DeviceRHI::ResourceCast(Dst);
    CHECK(D3D11Buffer != nullptr);
    CHECK(SrcData != nullptr);

    const FRHIBufferDesc& BufferDesc = D3D11Buffer->GetDesc();

    const uint64 Offset = BufferRegion.Offset;
    const uint64 Size   = BufferRegion.IsWholeResource() ? BufferDesc.Size : BufferRegion.Size;
    if (Size == 0)
    {
        return;
    }

    if (Offset + Size > BufferDesc.Size)
    {
        D3D11_ERROR("[FD3D11CommandContext]: UpdateBuffer writes %llu bytes at offset %llu into a buffer of %llu bytes", Size, Offset, BufferDesc.Size);
        return;
    }

    ID3D11DeviceContext* D3D11Context = GetD3D11Context();
    if (BufferDesc.IsDynamic() || BufferDesc.IsTransient())
    {
        D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
        const HRESULT Result = D3D11Context->Map(D3D11Buffer->GetD3D11Resource(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
        if (FAILED(Result))
        {
            D3D11_ERROR("[FD3D11CommandContext]: FAILED to map buffer for UpdateBuffer (0x%08X)", static_cast<uint32>(Result));
            return;
        }

        Memory::Memcpy(static_cast<uint8*>(MappedSubresource.pData) + Offset, SrcData, Size);
        D3D11Context->Unmap(D3D11Buffer->GetD3D11Resource(), 0);
        return;
    }

    if (!BufferDesc.IsConstantBuffer())
    {
        const D3D11_BOX Box = { static_cast<UINT>(Offset), 0, 0, static_cast<UINT>(Offset + Size), 1, 1 };
        D3D11Context->UpdateSubresource(D3D11Buffer->GetD3D11Resource(), 0, &Box, SrcData, 0, 0);
        return;
    }

    if ((Offset % D3D11_CONSTANT_BUFFER_ELEMENT_SIZE) != 0)
    {
        D3D11_ERROR("[FD3D11CommandContext]: Constant buffer updates must start on a %u byte boundary, got offset %llu", static_cast<uint32>(D3D11_CONSTANT_BUFFER_ELEMENT_SIZE), Offset);
        return;
    }

    const uint64 AlignedSize = Math::AlignUp<uint64>(Size, D3D11_CONSTANT_BUFFER_ELEMENT_SIZE);
    const uint64 ByteWidth   = Math::AlignUp<uint64>(BufferDesc.Size, D3D11_CONSTANT_BUFFER_ELEMENT_SIZE);

    TArray<uint8> PaddedData;
    const void* UpdateData = SrcData;
    if (AlignedSize != Size)
    {
        PaddedData.Resize(static_cast<int32>(AlignedSize));
        Memory::Memzero(PaddedData.Data(), PaddedData.Size());
        Memory::Memcpy(PaddedData.Data(), SrcData, Size);
        UpdateData = PaddedData.Data();
    }

    if (Offset == 0 && AlignedSize == ByteWidth)
    {
        D3D11Context->UpdateSubresource(D3D11Buffer->GetD3D11Resource(), 0, nullptr, UpdateData, 0, 0);
        return;
    }

    D3D11_BUFFER_DESC ScratchDesc = {};
    ScratchDesc.ByteWidth = static_cast<UINT>(AlignedSize);
    ScratchDesc.Usage     = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA ScratchData = {};
    ScratchData.pSysMem = UpdateData;

    TComPtr<ID3D11Buffer> ScratchBuffer;
    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateBuffer(&ScratchDesc, &ScratchData, &ScratchBuffer);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11CommandContext]: FAILED to create the scratch buffer for a partial constant buffer update (0x%08X)", static_cast<uint32>(Result));
        return;
    }

    D3D11Context->CopySubresourceRegion(D3D11Buffer->GetD3D11Resource(), 0, static_cast<UINT>(Offset), 0, 0, ScratchBuffer.Get(), 0, nullptr);
}

void FD3D11CommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch)
{
    FD3D11TextureRHI* D3D11Texture = FD3D11DeviceRHI::ResourceCast(Dst);
    CHECK(D3D11Texture != nullptr);
    CHECK(SrcData != nullptr);

    const uint32 NumMips = Math::Max<uint32>(D3D11Texture->GetDesc().NumMipLevels, 1);

    D3D11_BOX Box = {};
    Box.left   = TextureRegion.PositionX;
    Box.top    = TextureRegion.PositionY;
    Box.front  = 0;
    Box.right  = TextureRegion.PositionX + TextureRegion.Width;
    Box.bottom = TextureRegion.PositionY + TextureRegion.Height;
    Box.back   = 1;

    const UINT Subresource = D3D11CalcSubresource(MipLevel, 0, NumMips);
    GetD3D11Context()->UpdateSubresource(D3D11Texture->GetD3D11Resource(), Subresource, &Box, SrcData, SrcRowPitch, 0);
}

void FD3D11CommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
    FD3D11TextureRHI* D3D11Texture = FD3D11DeviceRHI::ResourceCast(Dst);
    CHECK(D3D11Texture != nullptr);
    CHECK(SrcData != nullptr);

    D3D11_BOX Box = {};
    Box.left   = TextureRegion.PositionX;
    Box.top    = TextureRegion.PositionY;
    Box.front  = TextureRegion.PositionZ;
    Box.right  = TextureRegion.PositionX + TextureRegion.Width;
    Box.bottom = TextureRegion.PositionY + TextureRegion.Height;
    Box.back   = TextureRegion.PositionZ + TextureRegion.Depth;

    GetD3D11Context()->UpdateSubresource(D3D11Texture->GetD3D11Resource(), MipLevel, &Box, SrcData, SrcRowPitch, SrcDepthPitch);
}

void FD3D11CommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FD3D11TextureRHI* D3D11Source      = FD3D11DeviceRHI::ResourceCast(Src);
    FD3D11TextureRHI* D3D11Destination = FD3D11DeviceRHI::ResourceCast(Dst);

    const DXGI_FORMAT DstFormat = D3D11CastShaderResourceFormat(D3D11Destination->GetDXGIFormat());
    const DXGI_FORMAT SrcFormat = D3D11CastShaderResourceFormat(D3D11Source->GetDXGIFormat());

    if (DstFormat != SrcFormat)
    {
        D3D11_ERROR("[FD3D11CommandContext]: Dst and Src must have compatible formats for resolve");
        return;
    }

    GetD3D11Context()->ResolveSubresource(D3D11Destination->GetD3D11Resource(), 0, D3D11Source->GetD3D11Resource(), 0, DstFormat);
}

void FD3D11CommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FD3D11BufferRHI* D3D11Destination = FD3D11DeviceRHI::ResourceCast(Dst);
    CHECK(D3D11Destination != nullptr);

    FD3D11BufferRHI* D3D11Source = FD3D11DeviceRHI::ResourceCast(Src);
    CHECK(D3D11Source != nullptr);

    ResolvePendingReadbacks();

    D3D11_BOX SourceBox = {};
    SourceBox.left   = static_cast<UINT>(CopyDesc.SrcOffset);
    SourceBox.right  = static_cast<UINT>(CopyDesc.SrcOffset + CopyDesc.Size);
    SourceBox.top    = 0;
    SourceBox.bottom = 1;
    SourceBox.front  = 0;
    SourceBox.back   = 1;

    GetD3D11Context()->CopySubresourceRegion(D3D11Destination->GetD3D11Resource(), 0, static_cast<UINT>(CopyDesc.DstOffset), 0, 0, D3D11Source->GetD3D11Resource(), 0, &SourceBox);
}

void FD3D11CommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FD3D11TextureRHI* D3D11Destination = FD3D11DeviceRHI::ResourceCast(Dst);
    CHECK(D3D11Destination != nullptr);

    FD3D11TextureRHI* D3D11Source = FD3D11DeviceRHI::ResourceCast(Src);
    CHECK(D3D11Source != nullptr);

    GetD3D11Context()->CopyResource(D3D11Destination->GetD3D11Resource(), D3D11Source->GetD3D11Resource());
}

void FD3D11CommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& InCopyDesc)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FD3D11TextureRHI* D3D11Destination = FD3D11DeviceRHI::ResourceCast(Dst);
    CHECK(D3D11Destination != nullptr);

    FD3D11TextureRHI* D3D11Source = FD3D11DeviceRHI::ResourceCast(Src);
    CHECK(D3D11Source != nullptr);

    const uint32 NumSrcMipLevels = Src->GetDesc().NumMipLevels;
    const uint32 NumDstMipLevels = Dst->GetDesc().NumMipLevels;

    for (uint32 ArraySlice = 0; ArraySlice < InCopyDesc.NumArraySlices; ArraySlice++)
    {
        for (uint32 MipLevel = 0; MipLevel < InCopyDesc.NumMipLevels; MipLevel++)
        {
            const UINT SrcSubresource = D3D11CalcSubresource(InCopyDesc.SrcMipSlice + MipLevel, InCopyDesc.SrcArraySlice + ArraySlice, NumSrcMipLevels);
            const UINT DstSubresource = D3D11CalcSubresource(InCopyDesc.DstMipSlice + MipLevel, InCopyDesc.DstArraySlice + ArraySlice, NumDstMipLevels);

            D3D11_BOX SourceBox;
            SourceBox.left   = InCopyDesc.SrcPosition.X >> MipLevel;
            SourceBox.right  = Math::Max((InCopyDesc.SrcPosition.X + InCopyDesc.Size.X) >> MipLevel, 1);
            SourceBox.top    = InCopyDesc.SrcPosition.Y >> MipLevel;
            SourceBox.bottom = Math::Max((InCopyDesc.SrcPosition.Y + InCopyDesc.Size.Y) >> MipLevel, 1);
            SourceBox.front  = InCopyDesc.SrcPosition.Z >> MipLevel;
            SourceBox.back   = Math::Max((InCopyDesc.SrcPosition.Z + InCopyDesc.Size.Z) >> MipLevel, 1);

            const UINT DestPositionX = InCopyDesc.DstPosition.X >> MipLevel;
            const UINT DestPositionY = InCopyDesc.DstPosition.Y >> MipLevel;
            const UINT DestPositionZ = InCopyDesc.DstPosition.Z >> MipLevel;

            GetD3D11Context()->CopySubresourceRegion(D3D11Destination->GetD3D11Resource(), DstSubresource, DestPositionX, DestPositionY, DestPositionZ, D3D11Source->GetD3D11Resource(), SrcSubresource, &SourceBox);
        }
    }
}

void FD3D11CommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FD3D11BufferRHI* D3D11Destination = FD3D11DeviceRHI::ResourceCast(Dst);
    CHECK(D3D11Destination != nullptr);

    FD3D11TextureRHI* D3D11Source = FD3D11DeviceRHI::ResourceCast(Src);
    CHECK(D3D11Source != nullptr);

    const uint32 SrcLeft = SrcRegion.PositionX >> SrcMipLevel;
    const uint32 SrcTop  = SrcRegion.PositionY >> SrcMipLevel;

    D3D11_BOX SourceBox = {};
    SourceBox.left   = SrcLeft;
    SourceBox.right  = Math::Max((SrcRegion.PositionX + SrcRegion.Width) >> SrcMipLevel, SrcLeft + 1);
    SourceBox.top    = SrcTop;
    SourceBox.bottom = Math::Max((SrcRegion.PositionY + SrcRegion.Height) >> SrcMipLevel, SrcTop + 1);
    SourceBox.front  = 0;
    SourceBox.back   = 1;

    const UINT SrcSubresource = D3D11CalcSubresource(SrcMipLevel, 0, Src->GetDesc().NumMipLevels);
    CopyTextureToBuffer(D3D11Destination, DstOffset, D3D11Source, SrcSubresource, SourceBox);
}

void FD3D11CommandContext::CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice)
{
    CHECK(Dst != nullptr);
    CHECK(Src != nullptr);

    FD3D11BufferRHI* D3D11Destination = FD3D11DeviceRHI::ResourceCast(Dst);
    CHECK(D3D11Destination != nullptr);

    FD3D11TextureRHI* D3D11Source = FD3D11DeviceRHI::ResourceCast(Src);
    CHECK(D3D11Source != nullptr);

    D3D11_BOX SourceBox = {};
    SourceBox.left   = SrcRegion.PositionX;
    SourceBox.right  = SrcRegion.PositionX + SrcRegion.Width;
    SourceBox.top    = SrcRegion.PositionY;
    SourceBox.bottom = SrcRegion.PositionY + Math::Max(SrcRegion.Height, 1u);
    SourceBox.front  = SrcRegion.PositionZ;
    SourceBox.back   = SrcRegion.PositionZ + Math::Max(SrcRegion.Depth, 1u);

    const UINT SrcSubresource = D3D11CalcSubresource(SrcMipLevel, SrcArraySlice, Src->GetDesc().NumMipLevels);
    CopyTextureToBuffer(D3D11Destination, DstOffset, D3D11Source, SrcSubresource, SourceBox);
}

void FD3D11CommandContext::WriteFence(FRHIFence* Fence)
{
    CHECK(Fence != nullptr);

    FD3D11FenceRHI* D3D11Fence = FD3D11DeviceRHI::ResourceCast(Fence);

    ResolvePendingReadbacks();
    D3D11Fence->Signal();

    GetD3D11Context()->Flush();
}

void FD3D11CommandContext::DiscardContents(FRHITexture* Texture)
{
    FD3D11TextureRHI* D3D11Texture = FD3D11DeviceRHI::ResourceCast(Texture);
    if (!D3D11Texture)
    {
        return;
    }

    if (ID3D11DeviceContext1* D3D11Context1 = GetDevice()->GetD3D11Context1())
    {
        D3D11Context1->DiscardResource(D3D11Texture->GetD3D11Resource());
    }
}

void FD3D11CommandContext::TransitionBarrier(TArrayView<const FRHITransitionBarrierDesc> TransitionDescs)
{
}

void FD3D11CommandContext::UnorderedAccessBarrier(TArrayView<const FRHIUnorderedAccessBarrierDesc> BarrierDescs)
{
}

void FD3D11CommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    ContextState.BindGraphicsState();
    GetD3D11Context()->DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
}

void FD3D11CommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    ContextState.BindGraphicsState();
    GetD3D11Context()->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void FD3D11CommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    ContextState.BindGraphicsState();
    GetD3D11Context()->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FD3D11CommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    ContextState.BindGraphicsState();
    GetD3D11Context()->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void FD3D11CommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    ContextState.BindComputeState();
    GetD3D11Context()->Dispatch(WorkGroupsX, WorkGroupsY, WorkGroupsZ);
}

void FD3D11CommandContext::DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
}

void FD3D11CommandContext::DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
}

void FD3D11CommandContext::DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset)
{
}

void FD3D11CommandContext::AcquireNextBackBuffer(FRHISwapChain* SwapChain)
{
}

void FD3D11CommandContext::PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync)
{
    FD3D11SwapChainRHI* D3D11SwapChain = FD3D11DeviceRHI::ResourceCast(SwapChain);
    D3D11SwapChain->Present(bVerticalSync);

    ContextState.DirtyRenderTargets();
}

void FD3D11CommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace)
{
    FD3D11SwapChainRHI* D3D11SwapChain = FD3D11DeviceRHI::ResourceCast(SwapChain);
    D3D11SwapChain->Resize(Width, Height, Format, ColorSpace);
}

void FD3D11CommandContext::SetSwapChainHDRMetadata(FRHISwapChain* SwapChain, const FRHIHDRMetadata& Metadata)
{
    FD3D11SwapChainRHI* D3D11SwapChain = FD3D11DeviceRHI::ResourceCast(SwapChain);
    D3D11SwapChain->SetHDRMetadata(Metadata);
}

void FD3D11CommandContext::PushEvent(const StringView& Name)
{
#if D3D11_ENABLE_ANNOTATIONS
    if (Annotation)
    {
        Annotation->BeginEvent(*CharToWide(Name));
    }
#endif
}

void FD3D11CommandContext::PopEvent()
{
#if D3D11_ENABLE_ANNOTATIONS
    if (Annotation)
    {
        Annotation->EndEvent();
    }
#endif
}

void FD3D11CommandContext::ClearState()
{
    ResolvePendingReadbacks();

    GetD3D11Context()->ClearState();
    ContextState.ResetState();
}

void FD3D11CommandContext::Flush()
{
    ResolvePendingReadbacks();
    GetD3D11Context()->Flush();
}

void* FD3D11CommandContext::GetRHINativeCommandList()
{
    return GetD3D11Context();
}

void FD3D11CommandContext::CopyTextureToBuffer(FD3D11BufferRHI* Dst, uint64 DstOffset, FD3D11TextureRHI* Src, uint32 SrcSubresource, const D3D11_BOX& SrcBox)
{
    const EFormat Format        = Src->GetDesc().Format;
    const uint32  BytesPerPixel = GetByteStrideFromFormat(Format);
    if (BytesPerPixel == 0 || IsBlockCompressed(Format))
    {
        D3D11_ERROR("[FD3D11CommandContext]: Copying a texture into a buffer requires a non-block-compressed, supported format. SrcFormat=%s", ToString(Format));
        return;
    }

    const uint32 RowPitch = Math::AlignUp<uint32>(BytesPerPixel * (SrcBox.right - SrcBox.left), D3D11_READBACK_ROW_PITCH_ALIGNMENT);
    const uint64 CopySize = uint64(RowPitch) * uint64(SrcBox.bottom - SrcBox.top) * uint64(SrcBox.back - SrcBox.front);

    if (DstOffset + CopySize > Dst->GetDesc().Size)
    {
        D3D11_ERROR("[FD3D11CommandContext]: Copying a texture into a buffer writes %llu bytes at offset %llu into a buffer of %llu bytes", CopySize, DstOffset, Dst->GetDesc().Size);
        return;
    }

    for (const FD3D11PendingReadback& PendingReadback : PendingReadbacks)
    {
        const FD3D11ReadbackTexture& ReadbackTexture = ReadbackTextures[PendingReadback.ReadbackTextureIndex];

        const bool bOverlapsTexture = ReadbackTexture.Source == Src->GetD3D11Resource() && ReadbackTexture.Subresource == SrcSubresource && BoxesOverlap(PendingReadback.Box, SrcBox);
        const bool bOverlapsBuffer  = PendingReadback.Destination == Dst && DstOffset < PendingReadback.DestinationOffset + PendingReadback.CopySize && PendingReadback.DestinationOffset < DstOffset + CopySize;
        if (bOverlapsTexture || bOverlapsBuffer)
        {
            ResolvePendingReadbacks();
            break;
        }
    }

    const int32 ReadbackTextureIndex = ObtainReadbackTexture(Src->GetD3D11Resource(), SrcSubresource);
    if (ReadbackTextureIndex < 0)
    {
        return;
    }

    ID3D11Resource* ReadbackTexture = ReadbackTextures[ReadbackTextureIndex].Texture.Get();
    GetD3D11Context()->CopySubresourceRegion(ReadbackTexture, 0, SrcBox.left, SrcBox.top, SrcBox.front, Src->GetD3D11Resource(), SrcSubresource, &SrcBox);

    FD3D11PendingReadback& PendingReadback = PendingReadbacks.Emplace();
    PendingReadback.Destination          = MakeSharedRef<FD3D11BufferRHI>(Dst);
    PendingReadback.DestinationOffset    = DstOffset;
    PendingReadback.CopySize             = CopySize;
    PendingReadback.Box                  = SrcBox;
    PendingReadback.BytesPerPixel        = BytesPerPixel;
    PendingReadback.RowPitch             = RowPitch;
    PendingReadback.ReadbackTextureIndex = ReadbackTextureIndex;

    // The CPU only reads readback buffers once the command list has finished, other buffers can be used by the next command
    if (!Dst->GetDesc().IsReadBack())
    {
        ResolvePendingReadbacks();
    }
}

void FD3D11CommandContext::ResolvePendingReadbacks()
{
    if (PendingReadbacks.IsEmpty())
    {
        return;
    }

    ID3D11DeviceContext* D3D11Context = GetD3D11Context();

    TArray<D3D11_MAPPED_SUBRESOURCE> MappedTextures;
    MappedTextures.Resize(ReadbackTextures.Size());

    for (int32 Index = 0; Index < ReadbackTextures.Size(); Index++)
    {
        const HRESULT Result = D3D11Context->Map(ReadbackTextures[Index].Texture.Get(), 0, D3D11_MAP_READ, 0, &MappedTextures[Index]);
        if (FAILED(Result))
        {
            D3D11_ERROR("[FD3D11CommandContext]: FAILED to map the readback texture (0x%08X)", static_cast<uint32>(Result));
            MappedTextures[Index].pData = nullptr;
        }
    }

    FD3D11BufferRHI* MappedBuffer     = nullptr;
    uint8*           MappedBufferData = nullptr;
    TArray<uint8>    ScratchData;

    for (const FD3D11PendingReadback& PendingReadback : PendingReadbacks)
    {
        const D3D11_MAPPED_SUBRESOURCE& MappedSource = MappedTextures[PendingReadback.ReadbackTextureIndex];
        if (!MappedSource.pData)
        {
            continue;
        }

        FD3D11BufferRHI* Destination = PendingReadback.Destination.Get();
        const bool       bIsReadBack = Destination->GetDesc().IsReadBack();

        uint8* DestinationData = nullptr;
        if (bIsReadBack)
        {
            if (MappedBuffer != Destination)
            {
                if (MappedBuffer)
                {
                    D3D11Context->Unmap(MappedBuffer->GetD3D11Resource(), 0);
                    MappedBuffer = nullptr;
                }

                D3D11_MAPPED_SUBRESOURCE MappedDestination = {};
                const HRESULT Result = D3D11Context->Map(Destination->GetD3D11Resource(), 0, D3D11_MAP_WRITE, 0, &MappedDestination);
                if (FAILED(Result))
                {
                    D3D11_ERROR("[FD3D11CommandContext]: FAILED to map the destination buffer (0x%08X)", static_cast<uint32>(Result));
                    continue;
                }

                MappedBuffer     = Destination;
                MappedBufferData = static_cast<uint8*>(MappedDestination.pData);
            }

            DestinationData = MappedBufferData + PendingReadback.DestinationOffset;
        }
        else
        {
            ScratchData.Resize(static_cast<int32>(PendingReadback.CopySize));
            DestinationData = ScratchData.Data();
        }

        const D3D11_BOX& Box     = PendingReadback.Box;
        const uint32     Height  = Box.bottom - Box.top;
        const uint32     Depth   = Box.back - Box.front;
        const uint32     RowSize = PendingReadback.BytesPerPixel * (Box.right - Box.left);

        for (uint32 Slice = 0; Slice < Depth; Slice++)
        {
            for (uint32 Row = 0; Row < Height; Row++)
            {
                const uint8* SourceRow = static_cast<const uint8*>(MappedSource.pData)
                    + uint64(Box.front + Slice) * MappedSource.DepthPitch
                    + uint64(Box.top + Row) * MappedSource.RowPitch
                    + uint64(Box.left) * PendingReadback.BytesPerPixel;

                uint8* DestinationRow = DestinationData + (uint64(Slice) * Height + Row) * PendingReadback.RowPitch;
                Memory::Memcpy(DestinationRow, SourceRow, RowSize);
            }
        }

        if (!bIsReadBack)
        {
            const D3D11_BOX DestinationBox = { static_cast<UINT>(PendingReadback.DestinationOffset), 0, 0, static_cast<UINT>(PendingReadback.DestinationOffset + PendingReadback.CopySize), 1, 1 };
            D3D11Context->UpdateSubresource(Destination->GetD3D11Resource(), 0, &DestinationBox, ScratchData.Data(), 0, 0);
        }
    }

    if (MappedBuffer)
    {
        D3D11Context->Unmap(MappedBuffer->GetD3D11Resource(), 0);
    }

    for (int32 Index = 0; Index < ReadbackTextures.Size(); Index++)
    {
        if (MappedTextures[Index].pData)
        {
            D3D11Context->Unmap(ReadbackTextures[Index].Texture.Get(), 0);
        }
    }

    PendingReadbacks.Clear();
    ReadbackTextures.Clear();
}

int32 FD3D11CommandContext::ObtainReadbackTexture(ID3D11Resource* Source, uint32 Subresource)
{
    for (int32 Index = 0; Index < ReadbackTextures.Size(); Index++)
    {
        if (ReadbackTextures[Index].Source == Source && ReadbackTextures[Index].Subresource == Subresource)
        {
            return Index;
        }
    }

    D3D11_RESOURCE_DIMENSION Dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    Source->GetType(&Dimension);

    TComPtr<ID3D11Resource> NewTexture;
    if (Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
        D3D11_TEXTURE2D_DESC Desc;
        static_cast<ID3D11Texture2D*>(Source)->GetDesc(&Desc);

        if (Desc.SampleDesc.Count > 1)
        {
            D3D11_ERROR("[FD3D11CommandContext]: Multisampled textures must be resolved before they are copied into a buffer");
            return -1;
        }

        const uint32 MipLevel = Subresource % Desc.MipLevels;
        Desc.Width          = Math::Max<UINT>(Desc.Width >> MipLevel, 1);
        Desc.Height         = Math::Max<UINT>(Desc.Height >> MipLevel, 1);
        Desc.MipLevels      = 1;
        Desc.ArraySize      = 1;
        Desc.Usage          = D3D11_USAGE_STAGING;
        Desc.BindFlags      = 0;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        Desc.MiscFlags      = 0;

        TComPtr<ID3D11Texture2D> NewTexture2D;
        const HRESULT Result = GetDevice()->GetD3D11Device()->CreateTexture2D(&Desc, nullptr, &NewTexture2D);
        if (FAILED(Result))
        {
            D3D11_ERROR("[FD3D11CommandContext]: FAILED to create the readback texture (0x%08X)", static_cast<uint32>(Result));
            return -1;
        }

        NewTexture = NewTexture2D;
    }
    else if (Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
    {
        D3D11_TEXTURE3D_DESC Desc;
        static_cast<ID3D11Texture3D*>(Source)->GetDesc(&Desc);

        const uint32 MipLevel = Subresource % Desc.MipLevels;
        Desc.Width          = Math::Max<UINT>(Desc.Width >> MipLevel, 1);
        Desc.Height         = Math::Max<UINT>(Desc.Height >> MipLevel, 1);
        Desc.Depth          = Math::Max<UINT>(Desc.Depth >> MipLevel, 1);
        Desc.MipLevels      = 1;
        Desc.Usage          = D3D11_USAGE_STAGING;
        Desc.BindFlags      = 0;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        Desc.MiscFlags      = 0;

        TComPtr<ID3D11Texture3D> NewTexture3D;
        const HRESULT Result = GetDevice()->GetD3D11Device()->CreateTexture3D(&Desc, nullptr, &NewTexture3D);
        if (FAILED(Result))
        {
            D3D11_ERROR("[FD3D11CommandContext]: FAILED to create the readback texture (0x%08X)", static_cast<uint32>(Result));
            return -1;
        }

        NewTexture = NewTexture3D;
    }
    else
    {
        D3D11_ERROR("[FD3D11CommandContext]: Only 2D and 3D textures can be copied into a buffer");
        return -1;
    }

    FD3D11ReadbackTexture& ReadbackTexture = ReadbackTextures.Emplace();
    ReadbackTexture.Texture     = NewTexture;
    ReadbackTexture.Source      = MakeComPtr<ID3D11Resource>(Source);
    ReadbackTexture.Subresource = Subresource;
    return ReadbackTextures.Size() - 1;
}

void FD3D11CommandContext::EndTimestampDisjointQuery()
{
    if (TimestampDisjointQuery)
    {
        TimestampDisjointQuery->End(GetD3D11Context());
        TimestampDisjointQuery.Reset();
    }
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
