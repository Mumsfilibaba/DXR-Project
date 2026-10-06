#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalResource.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalSwapChain.h"
#include "MetalRHI/MetalPipelineState.h"
#include "MetalRHI/MetalFence.h"
#include "MetalRHI/MetalUAVClear.h"
#include "MetalRHI/MetalQuery.h"
#include "MetalRHI/MetalCapabilities.h"
#include "MetalRHI/MetalRayTracing.h"
#include "RHI/RHIIndirect.h"
#include "RHI/RHICore.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformTLS.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

static TAutoConsoleVariable<bool> CVarDrawBreadcrumbs(
    "MetalRHI.DrawBreadcrumbs",
    "Records a breadcrumb and inserts a debug signpost for every draw and dispatch",
    false);

static void InsertDrawBreadcrumb(FMetalEncoderManager& Encoders, id<MTLCommandEncoder> Encoder, const CHAR* Name)
{
    if (CVarDrawBreadcrumbs.GetValue())
    {
        Encoders.GetCommands().Breadcrumbs.Push(StringView(Name));
        [Encoder insertDebugSignpost:[NSString stringWithUTF8String:Name]];
    }
}

FMetalCommandContext::FMetalCommandContext(FMetalDevice* InDevice, FMetalQueue& InQueue)
    : FMetalDeviceChild(InDevice)
    , IRHICommandContext()
    , Queue(InQueue)
    , Encoders(InQueue)
    , ContextState(InDevice, Encoders)
    , RecordingPool(nil)
    , ActiveOcclusionQuery(nullptr)
    , LastUsedFrame(0)
    , bIsRecording(false)
    , DebugGroups()
#if METAL_VALIDATE_CONTEXT_THREAD_OWNERSHIP
    , OwnerThreadID(CORE_INVALID_THREAD_ID)
#endif
{
}

FMetalCommandContext::~FMetalCommandContext()
{
    CHECK(!bIsRecording);
}

void FMetalCommandContext::StartContext()
{
    CHECK(!bIsRecording);

    AcquireOwnership();

    if (GetDevice()->HasPendingDefragMoves())
    {
        GetDevice()->FinalizeDefragMoves();
    }

    bIsRecording  = true;
    RecordingPool = [NSAutoreleasePool new];

    // The tables hold raw pointers, so bindings from the previous recording may name resources that no longer exist
    Encoders.BeginCommandBuffer(Queue.ObtainCommands());
    ContextState.ResetState();
    ContextState.BeginCommandBuffer();
}

void FMetalCommandContext::FinishContext()
{
    CHECK(bIsRecording);

    Submit(EMetalSubmitFlags::None);
    bIsRecording = false;

    [RecordingPool release];
    RecordingPool = nil;

    ReleaseOwnership();
}

uint64 FMetalCommandContext::Submit(EMetalSubmitFlags Flags)
{
    ContextState.EndCommandBuffer();

    if (Encoders.HasCommands())
    {
        FMetalDeviceRHI::Get()->FlushDeletionQueue(&Encoders.GetCommands());
    }

    FMetalCommands* Finished = Encoders.EndCommandBuffer();
    for (int32 Index = 0; Index < DebugGroups.Size(); ++Index)
    {
        [Finished->CommandBuffer popDebugGroup];
    }

    const uint64 SubmissionValue = Queue.SubmitCommands(Finished);

    if (IsEnumFlagSet(Flags, EMetalSubmitFlags::Reopen))
    {
        Encoders.BeginCommandBuffer(Queue.ObtainCommands());
        ContextState.BeginCommandBuffer();

        for (const String& GroupName : DebugGroups)
        {
            [Encoders.GetCommands().CommandBuffer pushDebugGroup:GroupName.GetNSString()];
        }
    }
    else
    {
        DebugGroups.Clear();
        UpdateScopePath();
    }

    if (IsEnumFlagSet(Flags, EMetalSubmitFlags::Wait))
    {
        Queue.WaitForValue(SubmissionValue);
    }

    return SubmissionValue;
}

void FMetalCommandContext::QueryTimestamp(FRHIQuery* Query)
{
    FMetalQueryRHI* MetalQuery = FMetalDeviceRHI::ResourceCast(Query);
    CHECK(MetalQuery != nullptr);

    if (MetalQuery->GetType() != EQueryType::Timestamp)
    {
        METAL_ERROR("QueryTimestamp requires a timestamp query");
        return;
    }

    FMetalTimestampQueries& Timestamps = GetDevice()->GetTimestampQueries();

    if (!Timestamps.IsAvailable())
    {
        METAL_ERROR("Timestamp queries are unavailable on this Metal device");
        return;
    }

    if (!Timestamps.Allocate(*MetalQuery))
    {
        return;
    }

#if METAL_ASSUME_APPLE_GPU
    Encoders.ScheduleTimestamp(MetalQuery->SampleIndex);
#else
    if (MetalRHI::SamplesTimestampsAtStageBoundary())
    {
        Encoders.ScheduleTimestamp(MetalQuery->SampleIndex);
    }
    else
    {
        Encoders.SampleCounters(Timestamps.GetSampleBuffer(), MetalQuery->SampleIndex);
    }
#endif

    AddPendingQuery(MetalQuery);
}

void FMetalCommandContext::BeginFrame()
{
    GetDevice()->FinalizeDefragMoves();
    FMetalDeviceRHI::Get()->BeginFrame();
}

void FMetalCommandContext::EndFrame()
{
    if (bIsRecording && GetDevice()->RecordDefragMoves(*this) > 0)
    {
        GetDevice()->SetDefragWaitValues(Submit(EMetalSubmitFlags::Reopen));
    }

    FMetalDeviceRHI::Get()->EndFrame();
}

void FMetalCommandContext::BeginQuery(FRHIQuery* Query)
{
    FMetalQueryRHI* MetalQuery = FMetalDeviceRHI::ResourceCast(Query);
    CHECK(MetalQuery != nullptr);

    const EQueryType Type = MetalQuery->GetType();

    if (Type == EQueryType::Occlusion)
    {
        id<MTLRenderCommandEncoder> Encoder = Encoders.GetRenderEncoder();

        if (!Encoder)
        {
            METAL_ERROR("BeginQuery for occlusion requires an open render encoder");
            return;
        }

        if (ActiveOcclusionQuery)
        {
            METAL_ERROR("BeginQuery for occlusion cannot nest");
            return;
        }

        if (!GetDevice()->GetOcclusionQueries().Allocate(*MetalQuery))
        {
            return;
        }

        const NSUInteger Offset = NSUInteger(MetalQuery->SampleIndex) * sizeof(uint64);
        [Encoder setVisibilityResultMode:MTLVisibilityResultModeCounting offset:Offset];
        ActiveOcclusionQuery = MetalQuery;
        AddPendingQuery(MetalQuery);
        return;
    }

    if (Type == EQueryType::PipelineStatistics)
    {
    #if METAL_ASSUME_APPLE_GPU
        METAL_ERROR("Pipeline statistics queries are not supported on Apple GPUs");
    #else
        FMetalStatisticQueries& Statistics = GetDevice()->GetStatisticQueries();

        if (!Statistics.IsAvailable())
        {
            METAL_ERROR("Pipeline statistics queries are unavailable on this Metal device");
            return;
        }

        if (Encoders.GetActiveStatisticsQuery())
        {
            METAL_ERROR("BeginQuery for pipeline statistics cannot nest");
            return;
        }

        Statistics.Begin(*MetalQuery);
        Encoders.SetActiveStatisticsQuery(MetalQuery);
    #endif
        return;
    }

    METAL_ERROR("BeginQuery is not supported for this query type");
}

void FMetalCommandContext::EndQuery(FRHIQuery* Query)
{
    FMetalQueryRHI* MetalQuery = FMetalDeviceRHI::ResourceCast(Query);
    CHECK(MetalQuery != nullptr);

#if !METAL_ASSUME_APPLE_GPU
    if (MetalQuery->GetType() == EQueryType::PipelineStatistics)
    {
        if (MetalQuery != Encoders.GetActiveStatisticsQuery())
        {
            METAL_ERROR("EndQuery does not match the active pipeline statistics query");
            return;
        }

        Encoders.SetActiveStatisticsQuery(nullptr);
        AddPendingQuery(MetalQuery);
        return;
    }
#endif

    if (MetalQuery->GetType() != EQueryType::Occlusion)
    {
        METAL_ERROR("EndQuery is only valid for pipeline statistics and occlusion queries");
        return;
    }

    if (MetalQuery != ActiveOcclusionQuery)
    {
        METAL_ERROR("EndQuery does not match the active occlusion query");
        return;
    }

    if (id<MTLRenderCommandEncoder> Encoder = Encoders.GetRenderEncoder())
    {
        [Encoder setVisibilityResultMode:MTLVisibilityResultModeDisabled offset:0];
    }

    ActiveOcclusionQuery = nullptr;
}

void* FMetalCommandContext::GetRHINativeCommandList()
{
    return nullptr;
}

void FMetalCommandContext::ClearRenderTargetView(FRHIRenderTargetView* RenderTargetView, const Vector4& ClearColor)
{
    SCOPED_AUTORELEASE_POOL();

    FMetalRenderTargetViewRHI* MetalRTV = static_cast<FMetalRenderTargetViewRHI*>(RenderTargetView);
    CHECK(MetalRTV != nullptr);

    MTLRenderPassColorAttachmentDescriptor* ColorAttachment = [[MTLRenderPassColorAttachmentDescriptor new] autorelease];
    MetalRTV->ApplyToAttachment(ColorAttachment);
    Encoders.UpdateResidency(MetalRTV->GetAttachmentResidencyEntry());

    FMetalPendingClear Clear;
    Clear.Color   = MTLClearColorMake(ClearColor.X, ClearColor.Y, ClearColor.Z, ClearColor.W);
    Clear.Aspects = EMetalClearAspects::Color;
    Encoders.AddPendingClear(ColorAttachment, Clear);
}

void FMetalCommandContext::ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil)
{
    SCOPED_AUTORELEASE_POOL();

    FMetalDepthStencilViewRHI* MetalDSV = static_cast<FMetalDepthStencilViewRHI*>(DepthStencilView);
    CHECK(MetalDSV != nullptr);

    id<MTLTexture> Texture = MetalDSV->GetAttachmentTexture();
    CHECK(Texture != nil);

    MTLRenderPassDepthAttachmentDescriptor* DepthAttachment = [[MTLRenderPassDepthAttachmentDescriptor new] autorelease];
    MetalDSV->ApplyToAttachment(DepthAttachment);
    Encoders.UpdateResidency(MetalDSV->GetAttachmentResidencyEntry());

    FMetalPendingClear Clear;
    Clear.Depth   = Depth;
    Clear.Stencil = Stencil;
    Clear.Aspects = MetalRHI::IsStencilPixelFormat(Texture.pixelFormat)
        ? (EMetalClearAspects::Depth | EMetalClearAspects::Stencil)
        : EMetalClearAspects::Depth;
    Encoders.AddPendingClear(DepthAttachment, Clear);
}

void FMetalCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor)
{
    MetalUAVClear::Clear(*this, static_cast<FMetalUnorderedAccessViewRHI*>(UnorderedAccessView), reinterpret_cast<const uint32*>(ClearColor.XYZW));
}

void FMetalCommandContext::ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4])
{
    MetalUAVClear::Clear(*this, static_cast<FMetalUnorderedAccessViewRHI*>(UnorderedAccessView), Values);
}

void FMetalCommandContext::BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc)
{
    SCOPED_AUTORELEASE_POOL();

    MTLRenderPassDescriptor* Descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    FillRenderPassDescriptor(Descriptor, BeginRenderPassDesc);

    Encoders.BeginRenderEncoder(Descriptor, "Render");
    ContextState.SetRenderPassInfo(GetRenderPassInfo(Descriptor, BeginRenderPassDesc.NumRenderTargets));
}

void FMetalCommandContext::EndRenderPass()
{
    id<MTLRenderCommandEncoder> Encoder = Encoders.GetRenderEncoder();
    CHECK(Encoder != nil);

    if (ActiveOcclusionQuery)
    {
        [Encoder setVisibilityResultMode:MTLVisibilityResultModeDisabled offset:0];
        ActiveOcclusionQuery = nullptr;
    }

    Encoders.EndEncoder();
}

void FMetalCommandContext::SetViewport(const FViewportRegion& ViewportRegion)
{
    MTLViewport Viewport;
    Viewport.width   = ViewportRegion.Width;
    Viewport.height  = ViewportRegion.Height;
    Viewport.originX = ViewportRegion.PositionX;
    Viewport.originY = ViewportRegion.PositionY;
    Viewport.znear   = ViewportRegion.MinDepth;
    Viewport.zfar    = ViewportRegion.MaxDepth;

    ContextState.SetViewports(&Viewport, 1);
}

void FMetalCommandContext::SetScissorRect(const FScissorRegion& ScissorRegion)
{
    MTLScissorRect Rect;
    Rect.x      = static_cast<NSUInteger>(Math::Max(static_cast<int32>(ScissorRegion.PositionX), 0));
    Rect.y      = static_cast<NSUInteger>(Math::Max(static_cast<int32>(ScissorRegion.PositionY), 0));
    Rect.width  = static_cast<NSUInteger>(Math::Max(static_cast<int32>(ScissorRegion.Width), 0));
    Rect.height = static_cast<NSUInteger>(Math::Max(static_cast<int32>(ScissorRegion.Height), 0));

    ContextState.SetScissorRects(&Rect, 1);
}

void FMetalCommandContext::SetBlendFactor(const Vector4& Color)
{
    const float BlendFactor[4] = { Color.X, Color.Y, Color.Z, Color.W };
    ContextState.SetBlendFactor(BlendFactor);
}

void FMetalCommandContext::SetStencilRef(uint32 StencilRef)
{
    ContextState.SetStencilRef(StencilRef);
}

void FMetalCommandContext::SetDepthBias(float DepthBias, float DepthBiasClamp, float SlopeScaledDepthBias)
{
    ContextState.SetDepthBias(DepthBias, DepthBiasClamp, SlopeScaledDepthBias);
}

void FMetalCommandContext::SetDepthBounds(float MinDepth, float MaxDepth)
{
    if (!GMetalSupportsDepthBoundsTest)
    {
        METAL_ERROR("SetDepthBounds: depth bounds tests require macOS 26");
        return;
    }

    ContextState.SetDepthBounds(MinDepth, MaxDepth);
}

void FMetalCommandContext::SetSamplePositions(const FRHISamplePositionsDesc& SamplePositionsDesc)
{
    ContextState.SetSamplePositions(SamplePositionsDesc);
}

void FMetalCommandContext::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
    UNREFERENCED_VARIABLE(Offsets);

    if (!Buffers.IsEmpty())
    {
        METAL_ERROR("SetStreamOutputTargets: stream output is not supported by the Metal backend");
    }
}

void FMetalCommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 BufferIndex = 0; BufferIndex < InVertexBuffers.Size(); ++BufferIndex)
    {
        ContextState.SetVertexBuffer(static_cast<FMetalBufferRHI*>(InVertexBuffers[BufferIndex]), BufferSlot + BufferIndex);
    }
}

void FMetalCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    ContextState.SetIndexBuffer(static_cast<FMetalBufferRHI*>(IndexBuffer), MetalRHI::ConvertIndexFormat(IndexFormat));
}

void FMetalCommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    ContextState.SetRenderPipelineState(static_cast<FMetalGraphicsPipelineStateRHI*>(PipelineState));
}

void FMetalCommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
    ContextState.SetComputePipelineState(static_cast<FMetalComputePipelineStateRHI*>(PipelineState));
}

void FMetalCommandContext::SetMeshletPipelineState(FRHIMeshletPipelineState* PipelineState)
{
    ContextState.SetRenderPipelineState(static_cast<FMetalMeshletPipelineStateRHI*>(PipelineState));
}

void FMetalCommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    CHECK(Shader != nullptr);
    ContextState.SetShaderConstants(MetalRHI::GetShaderVisibility(Shader->GetShaderStage()), reinterpret_cast<const uint32*>(ShaderConstants), NumShaderConstants);
}

void FMetalCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex)
{
    SetShaderResourceViews(Shader, MakeArrayView(&ShaderResourceView, 1), RegisterIndex);
}

void FMetalCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex)
{
    CHECK(Shader != nullptr);

    const EShaderVisibility::Type Stage = MetalRHI::GetShaderVisibility(Shader->GetShaderStage());
    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        ContextState.SetSRV(static_cast<FMetalShaderResourceViewRHI*>(InShaderResourceViews[Index]), Stage, RegisterIndex + Index);
    }
}

void FMetalCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex)
{
    SetUnorderedAccessViews(Shader, MakeArrayView(&UnorderedAccessView, 1), RegisterIndex);
}

void FMetalCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex)
{
    CHECK(Shader != nullptr);

    const EShaderVisibility::Type Stage = MetalRHI::GetShaderVisibility(Shader->GetShaderStage());
    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        ContextState.SetUAV(static_cast<FMetalUnorderedAccessViewRHI*>(InUnorderedAccessViews[Index]), Stage, RegisterIndex + Index);
    }
}

void FMetalCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex)
{
    SetConstantBuffers(Shader, MakeArrayView(&ConstantBuffer, 1), RegisterIndex);
}

void FMetalCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex)
{
    CHECK(Shader != nullptr);

    const EShaderVisibility::Type Stage = MetalRHI::GetShaderVisibility(Shader->GetShaderStage());
    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        ContextState.SetCBV(static_cast<FMetalBufferRHI*>(InConstantBuffers[Index]), Stage, RegisterIndex + Index);
    }
}

void FMetalCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex)
{
    SetSamplerStates(Shader, MakeArrayView(&SamplerState, 1), RegisterIndex);
}

void FMetalCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex)
{
    CHECK(Shader != nullptr);

    const EShaderVisibility::Type Stage = MetalRHI::GetShaderVisibility(Shader->GetShaderStage());
    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        ContextState.SetSampler(static_cast<FMetalSamplerStateRHI*>(InSamplerStates[Index]), Stage, RegisterIndex + Index);
    }
}

id<MTLBuffer> FMetalCommandContext::CreateStagingBuffer(uint64 Size, FMetalResourceStorage& OutStorage)
{
    OutStorage.Reset();

    if (Size == 0)
    {
        return nil;
    }

    void* Mapped = GetDevice()->GetStagingBufferAllocator()->Allocate(Size, BUFFER_ALIGNMENT, &Queue, OutStorage);

    if (!Mapped)
    {
        METAL_ERROR("Failed to allocate a %llu byte staging buffer", Size);
        OutStorage.Reset();
        return nil;
    }

    return OutStorage.GetBuffer();
}

void FMetalCommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SourceData)
{
    FMetalBufferRHI* MetalDst = GetMetalBuffer(Dst);

    if (!MetalDst || !SourceData || BufferRegion.Size == 0)
    {
        return;
    }

    id<MTLBuffer> DstBuffer = MetalDst->GetMTLBuffer();
    CHECK(DstBuffer != nil);

    const uint64 Size = BufferRegion.IsWholeResource() ? MetalDst->GetDesc().Size : BufferRegion.Size;

    if (MetalDst->GetDesc().IsTransient())
    {
        if (MetalDst->RelocateTransientStorage(Size, SourceData, &Queue))
        {
            ContextState.OnBufferRelocated(MetalDst);
        }

        return;
    }

    if (MetalRHI::GetMetalMemoryClass(MetalDst->GetDesc()) != EMetalMemoryClass::GPUOnly)
    {
        const uint64 DstOffset = MetalDst->GetMetalBindOffset() + BufferRegion.Offset;
        Memory::Memcpy(reinterpret_cast<uint8*>(DstBuffer.contents) + DstOffset, SourceData, Size);
        MetalRHI::FlushCPUWrite(DstBuffer, DstOffset, Size);
        return;
    }

    FMetalResourceStorage StagingStorage(GetDevice());

    if (!CreateStagingBuffer(Size, StagingStorage))
    {
        return;
    }

    Memory::Memcpy(StagingStorage.GetMappedBaseAddress(), SourceData, Size);
    Encoders.UpdateResidency(MetalDst->GetResidencyEntry());

    [Encoders.RequireBlitEncoder() copyFromBuffer:StagingStorage.GetBuffer()
                                     sourceOffset:StagingStorage.GetResourceOffset()
                                         toBuffer:DstBuffer
                                destinationOffset:MetalDst->GetMetalBindOffset() + BufferRegion.Offset
                                             size:Size];
}

void FMetalCommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SourceData, uint32 SrcRowPitch)
{
    const FTextureRegion3D Region3D(TextureRegion.Width, TextureRegion.Height, 1, TextureRegion.PositionX, TextureRegion.PositionY, 0);
    UpdateTexture3D(Dst, Region3D, MipLevel, SourceData, SrcRowPitch, SrcRowPitch * TextureRegion.Height);
}

void FMetalCommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
    FMetalTextureRHI* MetalDst = GetMetalTexture(Dst);

    if (!MetalDst || !SrcData || TextureRegion.Width == 0 || TextureRegion.Height == 0)
    {
        return;
    }

    id<MTLTexture> DstTexture = MetalDst->GetMTLTexture();
    CHECK(DstTexture != nil);

    Encoders.FlushPendingClears(DstTexture);

    const FRHITextureDesc& DstDesc      = MetalDst->GetDesc();
    const bool             bIsTexture1D = DstDesc.IsTexture1D() || DstDesc.IsTexture1DArray();
    const bool             bIsTexture3D = DstDesc.IsTexture3D();
    const uint32           Depth        = Math::Max(TextureRegion.Depth, 1u);
    const uint64           DataSize     = uint64(SrcDepthPitch) * Depth;

    FMetalResourceStorage StagingStorage(GetDevice());

    if (!CreateStagingBuffer(DataSize, StagingStorage))
    {
        return;
    }

    Memory::Memcpy(StagingStorage.GetMappedBaseAddress(), SrcData, DataSize);
    Encoders.UpdateResidency(MetalDst->GetResidencyEntry());

    [Encoders.RequireBlitEncoder() copyFromBuffer:StagingStorage.GetBuffer()
                                     sourceOffset:StagingStorage.GetResourceOffset()
                                sourceBytesPerRow:(bIsTexture1D ? 0 : SrcRowPitch)
                              sourceBytesPerImage:(bIsTexture3D ? SrcDepthPitch : 0)
                                       sourceSize:MTLSizeMake(TextureRegion.Width, TextureRegion.Height, Depth)
                                        toTexture:DstTexture
                                 destinationSlice:0
                                 destinationLevel:MipLevel
                                destinationOrigin:MTLOriginMake(TextureRegion.PositionX, TextureRegion.PositionY, TextureRegion.PositionZ)];
}

void FMetalCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
    SCOPED_AUTORELEASE_POOL();

    FMetalTextureRHI* MetalDst = GetMetalTexture(Dst);
    FMetalTextureRHI* MetalSrc = GetMetalTexture(Src);
    CHECK(MetalDst != nullptr);
    CHECK(MetalSrc != nullptr);

    id<MTLTexture> SrcTexture = MetalSrc->GetMTLTexture();
    id<MTLTexture> DstTexture = MetalDst->GetMTLTexture();
    CHECK(SrcTexture != nil);
    CHECK(DstTexture != nil);

    if (SrcTexture.sampleCount <= 1)
    {
        METAL_ERROR("ResolveTexture requires a multisampled source");
        return;
    }

    Encoders.FlushPendingClears(SrcTexture);
    Encoders.FlushPendingClears(DstTexture);
    Encoders.UpdateResidency(MetalSrc->GetResidencyEntry());
    Encoders.UpdateResidency(MetalDst->GetResidencyEntry());

    MTLRenderPassDescriptor* Descriptor = [MTLRenderPassDescriptor renderPassDescriptor];

    const bool bIsStencil = MetalRHI::IsStencilPixelFormat(SrcTexture.pixelFormat);
    const bool bIsDepth   = (SrcTexture.pixelFormat == MTLPixelFormatDepth16Unorm)
        || (SrcTexture.pixelFormat == MTLPixelFormatDepth32Float)
        || (SrcTexture.pixelFormat == MTLPixelFormatDepth32Float_Stencil8)
        || (SrcTexture.pixelFormat == MTLPixelFormatDepth24Unorm_Stencil8);

    if (bIsDepth)
    {
        MTLRenderPassDepthAttachmentDescriptor* DepthAttachment = Descriptor.depthAttachment;
        DepthAttachment.texture        = SrcTexture;
        DepthAttachment.resolveTexture = DstTexture;
        DepthAttachment.loadAction     = MTLLoadActionLoad;
        DepthAttachment.storeAction    = MTLStoreActionMultisampleResolve;

        if (bIsStencil && GMetalSupportsStencilResolve)
        {
            MTLRenderPassStencilAttachmentDescriptor* StencilAttachment = Descriptor.stencilAttachment;
            StencilAttachment.texture        = SrcTexture;
            StencilAttachment.resolveTexture = DstTexture;
            StencilAttachment.loadAction     = MTLLoadActionLoad;
            StencilAttachment.storeAction    = MTLStoreActionMultisampleResolve;
        }
        else if (bIsStencil)
        {
            METAL_WARNING("ResolveTexture: this device cannot resolve the stencil half, resolving depth only");
        }
    }
    else if (bIsStencil)
    {
        if (!GMetalSupportsStencilResolve)
        {
            METAL_ERROR("ResolveTexture: stencil resolve is not supported by this device");
            return;
        }

        MTLRenderPassStencilAttachmentDescriptor* StencilAttachment = Descriptor.stencilAttachment;
        StencilAttachment.texture        = SrcTexture;
        StencilAttachment.resolveTexture = DstTexture;
        StencilAttachment.loadAction     = MTLLoadActionLoad;
        StencilAttachment.storeAction    = MTLStoreActionMultisampleResolve;
    }
    else
    {
        MTLRenderPassColorAttachmentDescriptor* ColorAttachment = Descriptor.colorAttachments[0];
        ColorAttachment.texture        = SrcTexture;
        ColorAttachment.resolveTexture = DstTexture;
        ColorAttachment.loadAction     = MTLLoadActionLoad;
        ColorAttachment.storeAction    = MTLStoreActionMultisampleResolve;
    }

    Encoders.EncodeLoadStorePass(Descriptor, "ResolveTexture");
}

void FMetalCommandContext::TranscodeSamplerFeedback(FRHITexture* Dst, uint32 DstSubresource, FRHITexture* Src, uint32 SrcSubresource, ESamplerFeedbackTranscodeMode Mode)
{
    UNREFERENCED_VARIABLE(Dst);
    UNREFERENCED_VARIABLE(DstSubresource);
    UNREFERENCED_VARIABLE(Src);
    UNREFERENCED_VARIABLE(SrcSubresource);
    UNREFERENCED_VARIABLE(Mode);

    METAL_ERROR("TranscodeSamplerFeedback: sampler feedback is not supported by the Metal backend");
}

void FMetalCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    FMetalBufferRHI* MetalDst = GetMetalBuffer(Dst);
    FMetalBufferRHI* MetalSrc = GetMetalBuffer(Src);
    CHECK(MetalDst != nullptr);
    CHECK(MetalSrc != nullptr);
    Encoders.UpdateResidency(MetalSrc->GetResidencyEntry());
    Encoders.UpdateResidency(MetalDst->GetResidencyEntry());

    [Encoders.RequireBlitEncoder() copyFromBuffer:MetalSrc->GetMTLBuffer()
                                     sourceOffset:CopyDesc.SrcOffset + MetalSrc->GetMetalBindOffset()
                                         toBuffer:MetalDst->GetMTLBuffer()
                                destinationOffset:CopyDesc.DstOffset + MetalDst->GetMetalBindOffset()
                                             size:CopyDesc.Size];
}

void FMetalCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FMetalTextureRHI* MetalDst = GetMetalTexture(Dst);
    FMetalTextureRHI* MetalSrc = GetMetalTexture(Src);
    CHECK(MetalDst != nullptr);
    CHECK(MetalSrc != nullptr);

    Encoders.FlushPendingClears(MetalSrc->GetMTLTexture());
    Encoders.FlushPendingClears(MetalDst->GetMTLTexture());
    Encoders.UpdateResidency(MetalSrc->GetResidencyEntry());
    Encoders.UpdateResidency(MetalDst->GetResidencyEntry());

    [Encoders.RequireBlitEncoder() copyFromTexture:MetalSrc->GetMTLTexture() toTexture:MetalDst->GetMTLTexture()];
}

void FMetalCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc)
{
    FMetalTextureRHI* MetalDst = GetMetalTexture(Dst);
    FMetalTextureRHI* MetalSrc = GetMetalTexture(Src);
    CHECK(MetalDst != nullptr);
    CHECK(MetalSrc != nullptr);

    id<MTLTexture> SrcTexture = MetalSrc->GetMTLTexture();
    id<MTLTexture> DstTexture = MetalDst->GetMTLTexture();

    const uint32 NumArraySlices = Math::Max(CopyDesc.NumArraySlices, 1u);
    const uint32 NumMipLevels   = Math::Max(CopyDesc.NumMipLevels, 1u);

    Encoders.FlushPendingClears(SrcTexture);
    Encoders.FlushPendingClears(DstTexture);
    Encoders.UpdateResidency(MetalSrc->GetResidencyEntry());
    Encoders.UpdateResidency(MetalDst->GetResidencyEntry());

    id<MTLBlitCommandEncoder> Encoder = Encoders.RequireBlitEncoder();
    for (uint32 ArrayIndex = 0; ArrayIndex < NumArraySlices; ++ArrayIndex)
    {
        for (uint32 MipIndex = 0; MipIndex < NumMipLevels; ++MipIndex)
        {
            const uint32 SrcMipLevel = CopyDesc.SrcMipSlice + MipIndex;
            const uint32 DstMipLevel = CopyDesc.DstMipSlice + MipIndex;

            const MTLOrigin SrcOrigin = MTLOriginMake(CopyDesc.SrcPosition.X >> MipIndex, CopyDesc.SrcPosition.Y >> MipIndex, CopyDesc.SrcPosition.Z >> MipIndex);
            const MTLOrigin DstOrigin = MTLOriginMake(CopyDesc.DstPosition.X >> MipIndex, CopyDesc.DstPosition.Y >> MipIndex, CopyDesc.DstPosition.Z >> MipIndex);

            const MTLSize Size = MTLSizeMake(
                MetalRHI::ResolveMipCopyExtent(uint32(CopyDesc.SrcPosition.X + CopyDesc.Size.X) >> MipIndex, SrcOrigin.x, DstOrigin.x, MetalRHI::GetMipExtent(SrcTexture.width,  SrcMipLevel), MetalRHI::GetMipExtent(DstTexture.width,  DstMipLevel)),
                MetalRHI::ResolveMipCopyExtent(uint32(CopyDesc.SrcPosition.Y + CopyDesc.Size.Y) >> MipIndex, SrcOrigin.y, DstOrigin.y, MetalRHI::GetMipExtent(SrcTexture.height, SrcMipLevel), MetalRHI::GetMipExtent(DstTexture.height, DstMipLevel)),
                MetalRHI::ResolveMipCopyExtent(uint32(CopyDesc.SrcPosition.Z + CopyDesc.Size.Z) >> MipIndex, SrcOrigin.z, DstOrigin.z, MetalRHI::GetMipExtent(SrcTexture.depth,  SrcMipLevel), MetalRHI::GetMipExtent(DstTexture.depth,  DstMipLevel)));

            if (Size.width == 0 || Size.height == 0 || Size.depth == 0)
            {
                continue;
            }

            [Encoder copyFromTexture:SrcTexture
                         sourceSlice:CopyDesc.SrcArraySlice + ArrayIndex
                         sourceLevel:SrcMipLevel
                        sourceOrigin:SrcOrigin
                          sourceSize:Size
                           toTexture:DstTexture
                    destinationSlice:CopyDesc.DstArraySlice + ArrayIndex
                    destinationLevel:DstMipLevel
                   destinationOrigin:DstOrigin];
        }
    }
}

void FMetalCommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel)
{
    const FTextureRegion3D Region3D(SrcRegion.Width, SrcRegion.Height, 1, SrcRegion.PositionX, SrcRegion.PositionY, 0);
    CopyTextureSubresourceToBuffer(Dst, DstOffset, Src, Region3D, SrcMipLevel, 0);
}

void FMetalCommandContext::CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice)
{
    FMetalBufferRHI*  MetalDst = GetMetalBuffer(Dst);
    FMetalTextureRHI* MetalSrc = GetMetalTexture(Src);
    CHECK(MetalDst != nullptr);
    CHECK(MetalSrc != nullptr);

    const FRHITextureDesc& SrcDesc      = MetalSrc->GetDesc();
    const bool             bIsTexture3D = SrcDesc.IsTexture3D();
    const uint32           Depth        = Math::Max(SrcRegion.Depth, 1u);
    const uint64           PixelStride  = GetByteStrideFromFormat(SrcDesc.Format);
    const uint64           RowPitch     = Math::AlignUp(uint64(SrcRegion.Width) * PixelStride, 256ull);
    const uint64           SlicePitch   = RowPitch * SrcRegion.Height;

    Encoders.FlushPendingClears(MetalSrc->GetMTLTexture());
    Encoders.UpdateResidency(MetalSrc->GetResidencyEntry());
    Encoders.UpdateResidency(MetalDst->GetResidencyEntry());

    [Encoders.RequireBlitEncoder() copyFromTexture:MetalSrc->GetMTLTexture()
                                       sourceSlice:SrcArraySlice
                                       sourceLevel:SrcMipLevel
                                      sourceOrigin:MTLOriginMake(SrcRegion.PositionX, SrcRegion.PositionY, SrcRegion.PositionZ)
                                        sourceSize:MTLSizeMake(SrcRegion.Width, SrcRegion.Height, Depth)
                                          toBuffer:MetalDst->GetMTLBuffer()
                                 destinationOffset:MetalDst->GetMetalBindOffset() + DstOffset
                            destinationBytesPerRow:RowPitch
                          destinationBytesPerImage:(bIsTexture3D ? SlicePitch : 0)];
}

void FMetalCommandContext::WriteFence(FRHIFence* Fence)
{
    FMetalFenceRHI* MetalFence = static_cast<FMetalFenceRHI*>(Fence);
    CHECK(MetalFence != nullptr);

    FMetalCommands& Commands = Encoders.GetCommands();
    Commands.PendingSignals.Add({ MetalFence->GetMTLSharedEvent(), MetalFence->SignalNextValue() });
    Submit(EMetalSubmitFlags::Reopen);
}

void FMetalCommandContext::DiscardContents(class FRHITexture* Texture)
{
    FMetalTextureRHI* MetalTexture = GetMetalTexture(Texture);

    if (id<MTLTexture> Handle = MetalTexture ? MetalTexture->GetMTLTexture() : nil)
    {
        Encoders.AddPendingDiscard(Handle);
    }
}

void FMetalCommandContext::BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc)
{
    FMetalSceneAccelerationStructureRHI* MetalScene = static_cast<FMetalSceneAccelerationStructureRHI*>(RayTracingScene);
    if (!MetalScene->Build(*this, BuildDesc))
    {
        METAL_ERROR("Failed to build the scene acceleration structure");
    }
}

void FMetalCommandContext::BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
    FMetalGeometryAccelerationStructureRHI* MetalGeometry = static_cast<FMetalGeometryAccelerationStructureRHI*>(RayTracingGeometry);
    if (!MetalGeometry->Build(*this, BuildDesc))
    {
        METAL_ERROR("Failed to build the geometry acceleration structure");
    }
}

void FMetalCommandContext::BuildOpacityMicromap(FRHIOpacityMicromap* /* OpacityMicromap */, const FRHIOpacityMicromapBuildDesc& /* BuildDesc */)
{
    METAL_ERROR("Metal has no opacity micromaps");
}

void FMetalCommandContext::ExecuteIndirectRayTracingAccelerationStructureOperations(const FRHIRayTracingAccelerationStructureOperationDesc* /* Operations */, uint32 /* NumOperations */)
{
    METAL_ERROR("Metal has no indirect acceleration structure operations");
}

void FMetalCommandContext::WriteAccelerationStructurePostBuildInfo(FRHIBuffer* DstBuffer, uint64 DstOffset, EAccelerationStructurePostBuildInfoType InfoType, FRHIRayTracingAccelerationStructure* const* Sources, uint32 NumSources)
{
    if (InfoType != EAccelerationStructurePostBuildInfoType::CompactedSize)
    {
        METAL_ERROR("Metal only reports the compacted size of an acceleration structure, not %s", ToString(InfoType));
        return;
    }

    FMetalBufferRHI* MetalBuffer = GetMetalBuffer(DstBuffer);
    CHECK(MetalBuffer != nullptr);
    CHECK(Sources != nullptr && NumSources > 0);

    Encoders.UpdateResidency(MetalBuffer->GetResidencyEntry());

    id<MTLAccelerationStructureCommandEncoder> Encoder = Encoders.RequireAccelerationStructureEncoder();
    for (uint32 Index = 0; Index < NumSources; ++Index)
    {
        FMetalAccelerationStructure* Source          = GetMetalAccelerationStructure(Sources[Index]);
        id<MTLAccelerationStructure> SourceStructure = Source ? Source->GetMTLAccelerationStructure() : nil;
        if (!SourceStructure)
        {
            METAL_ERROR("WriteAccelerationStructurePostBuildInfo source %u has not been built", Index);
            continue;
        }

        Encoders.UpdateResidency(Source->GetResidencyEntry());

        [Encoder writeCompactedAccelerationStructureSize:SourceStructure
                                                toBuffer:MetalBuffer->GetMTLBuffer()
                                                  offset:MetalBuffer->GetMetalBindOffset() + DstOffset + Index * sizeof(uint64)
                                            sizeDataType:MTLDataTypeULong];
    }
}

void FMetalCommandContext::CopyAccelerationStructure(FRHIRayTracingAccelerationStructure* Destination, FRHIRayTracingAccelerationStructure* Source, EAccelerationStructureCopyMode CopyMode)
{
    if (CopyMode != EAccelerationStructureCopyMode::Clone && CopyMode != EAccelerationStructureCopyMode::Compact)
    {
        METAL_ERROR("Metal acceleration structures only clone and compact, not %s", ToString(CopyMode));
        return;
    }

    FMetalAccelerationStructure* MetalDestination = GetMetalAccelerationStructure(Destination);
    FMetalAccelerationStructure* MetalSource      = GetMetalAccelerationStructure(Source);
    CHECK(MetalDestination != nullptr && MetalSource != nullptr);

    if (!MetalDestination->CopyFrom(*this, *MetalSource, CopyMode == EAccelerationStructureCopyMode::Compact))
    {
        METAL_ERROR("Failed to copy the acceleration structure");
    }
}

void FMetalCommandContext::CompactAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure, uint64 CompactedSizeInBytes)
{
    if (!AccelerationStructure || CompactedSizeInBytes == 0)
    {
        return;
    }

    FMetalAccelerationStructure* MetalAccelerationStructure = GetMetalAccelerationStructure(AccelerationStructure);
    if (!MetalAccelerationStructure->CompactInPlace(*this, CompactedSizeInBytes))
    {
        METAL_ERROR("Failed to compact the acceleration structure to %llu bytes", CompactedSizeInBytes);
    }
}

void FMetalCommandContext::SerializeAccelerationStructure(FRHIRayTracingAccelerationStructure* /* Source */, FRHIBuffer* /* DstBuffer */, uint64 /* DstOffset */)
{
    METAL_ERROR("Metal has no acceleration structure serialization");
}

void FMetalCommandContext::DeserializeAccelerationStructure(FRHIRayTracingAccelerationStructure* /* Destination */, FRHIBuffer* /* SourceBuffer */, uint64 /* SourceOffset */)
{
    METAL_ERROR("Metal has no acceleration structure serialization");
}

void FMetalCommandContext::TransitionBarrier(TArrayView<const FRHITransitionBarrierDesc> TransitionDescs)
{
    if (Encoders.HasPendingClears() && Encoders.GetEncoderType() != EMetalEncoderType::Render)
    {
        static constexpr ERHIResourceState AttachmentStates = ERHIResourceState::RenderTarget | ERHIResourceState::DepthWrite;
        for (const FRHITransitionBarrierDesc& Desc : TransitionDescs)
        {
            if (Desc.ResourceType != ERHIBarrierResourceType::Texture || IsEnumFlagSet(Desc.AfterState, AttachmentStates))
            {
                continue;
            }

            if (FMetalTextureRHI* MetalTexture = GetMetalTexture(Desc.Texture.Resource))
            {
                if (id<MTLTexture> Texture = MetalTexture->GetMTLTexture())
                {
                    Encoders.FlushPendingClears(Texture);
                }
            }
        }
    }

    for (const FRHITransitionBarrierDesc& Desc : TransitionDescs)
    {
        if (Desc.IsSplit())
        {
            continue;
        }

        const bool bAfterUAV    = IsEnumFlagSet(Desc.AfterState, ERHIResourceState::UnorderedAccess);
        const bool bBeforeUAV   = IsEnumFlagSet(Desc.BeforeState, ERHIResourceState::UnorderedAccess);
        const bool bWriteToRead = !RHIIsReadOnlyState(Desc.BeforeState) && RHIIsReadOnlyState(Desc.AfterState);

        if (bAfterUAV || bBeforeUAV || bWriteToRead)
        {
            Encoders.MemoryBarrier();
            return;
        }
    }
}

void FMetalCommandContext::UnorderedAccessBarrier(TArrayView<const FRHIUnorderedAccessBarrierDesc> BarrierDescs)
{
    UNREFERENCED_VARIABLE(BarrierDescs);
    Encoders.MemoryBarrier();
}

void FMetalCommandContext::AddPendingQuery(FMetalQueryRHI* Query)
{
    Encoders.GetCommands().PendingQueries.Add(Query);
}

void FMetalCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
}

void FMetalCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void FMetalCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    if (VertexCountPerInstance == 0 || InstanceCount == 0)
    {
        return;
    }

    id<MTLRenderCommandEncoder> Encoder = Encoders.GetRenderEncoder();
    ContextState.BindRenderState<EMetalRenderPipelineType::Graphics>(Encoder);

    [Encoder drawPrimitives:ContextState.GetRenderPipeline()->GetPrimitiveType()
                vertexStart:StartVertexLocation
                vertexCount:VertexCountPerInstance
              instanceCount:InstanceCount
               baseInstance:StartInstanceLocation];

    InsertDrawBreadcrumb(Encoders, Encoder, "DrawInstanced");
}

void FMetalCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    if (IndexCountPerInstance == 0 || InstanceCount == 0)
    {
        return;
    }

    id<MTLRenderCommandEncoder> Encoder = Encoders.GetRenderEncoder();
    ContextState.BindRenderState<EMetalRenderPipelineType::Graphics>(Encoder);

    const FMetalIndexBufferCache& IndexBuffer = ContextState.GetIndexBuffer();
    CHECK(IndexBuffer.IndexBuffer != nullptr);
    Encoders.UpdateResidency(IndexBuffer.IndexBuffer->GetResidencyEntry());

    const NSUInteger IndexStride = (IndexBuffer.IndexType == MTLIndexTypeUInt16) ? 2 : 4;
    [Encoder drawIndexedPrimitives:ContextState.GetRenderPipeline()->GetPrimitiveType()
                        indexCount:IndexCountPerInstance
                         indexType:IndexBuffer.IndexType
                       indexBuffer:IndexBuffer.IndexBuffer->GetMTLBuffer()
                 indexBufferOffset:IndexBuffer.IndexBuffer->GetMetalBindOffset() + StartIndexLocation * IndexStride
                     instanceCount:InstanceCount
                        baseVertex:BaseVertexLocation
                      baseInstance:StartInstanceLocation];

    InsertDrawBreadcrumb(Encoders, Encoder, "DrawIndexedInstanced");
}

void FMetalCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    if (WorkGroupsX == 0 || WorkGroupsY == 0 || WorkGroupsZ == 0)
    {
        return;
    }

    id<MTLComputeCommandEncoder> Encoder = Encoders.RequireComputeEncoder();
    ContextState.BindComputeState(Encoder);

    [Encoder dispatchThreadgroups:MTLSizeMake(WorkGroupsX, WorkGroupsY, WorkGroupsZ)
            threadsPerThreadgroup:ContextState.GetComputePipeline()->GetThreadsPerThreadgroup()];

    InsertDrawBreadcrumb(Encoders, Encoder, "Dispatch");
}

void FMetalCommandContext::DrawIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
    if (CommandCount == 0)
    {
        return;
    }

    FMetalBufferRHI* Arguments = GetMetalBuffer(ArgumentBuffer);
    CHECK(Arguments != nullptr);
    Encoders.UpdateResidency(Arguments->GetResidencyEntry());

    id<MTLRenderCommandEncoder> Encoder = Encoders.GetRenderEncoder();
    ContextState.BindRenderState<EMetalRenderPipelineType::Graphics>(Encoder);

    const MTLPrimitiveType PrimitiveType     = ContextState.GetRenderPipeline()->GetPrimitiveType();
    id<MTLBuffer>          ArgumentMTLBuffer = Arguments->GetMTLBuffer();
    const uint64           BaseOffset        = ArgumentBufferOffset + Arguments->GetMetalBindOffset();

    for (uint32 CommandIndex = 0; CommandIndex < CommandCount; ++CommandIndex)
    {
        [Encoder drawPrimitives:PrimitiveType
                 indirectBuffer:ArgumentMTLBuffer
           indirectBufferOffset:BaseOffset + CommandIndex * sizeof(FRHIDrawIndirectParameters)];
    }

    InsertDrawBreadcrumb(Encoders, Encoder, "DrawIndirect");
}

void FMetalCommandContext::DrawIndexedIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
    if (CommandCount == 0)
    {
        return;
    }

    FMetalBufferRHI* Arguments = GetMetalBuffer(ArgumentBuffer);
    CHECK(Arguments != nullptr);
    Encoders.UpdateResidency(Arguments->GetResidencyEntry());

    id<MTLRenderCommandEncoder> Encoder = Encoders.GetRenderEncoder();
    ContextState.BindRenderState<EMetalRenderPipelineType::Graphics>(Encoder);

    const FMetalIndexBufferCache& IndexBuffer = ContextState.GetIndexBuffer();
    CHECK(IndexBuffer.IndexBuffer != nullptr);
    Encoders.UpdateResidency(IndexBuffer.IndexBuffer->GetResidencyEntry());

    const MTLPrimitiveType PrimitiveType     = ContextState.GetRenderPipeline()->GetPrimitiveType();
    id<MTLBuffer>          ArgumentMTLBuffer = Arguments->GetMTLBuffer();
    const uint64           BaseOffset        = ArgumentBufferOffset + Arguments->GetMetalBindOffset();
    id<MTLBuffer>          IndexMTLBuffer    = IndexBuffer.IndexBuffer->GetMTLBuffer();
    const NSUInteger       IndexOffset       = IndexBuffer.IndexBuffer->GetMetalBindOffset();

    for (uint32 CommandIndex = 0; CommandIndex < CommandCount; ++CommandIndex)
    {
        [Encoder drawIndexedPrimitives:PrimitiveType
                             indexType:IndexBuffer.IndexType
                           indexBuffer:IndexMTLBuffer
                     indexBufferOffset:IndexOffset
                        indirectBuffer:ArgumentMTLBuffer
                  indirectBufferOffset:BaseOffset + CommandIndex * sizeof(FRHIDrawIndexedIndirectParameters)];
    }

    InsertDrawBreadcrumb(Encoders, Encoder, "DrawIndexedIndirect");
}

void FMetalCommandContext::DispatchIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset)
{
    FMetalBufferRHI* Arguments = GetMetalBuffer(ArgumentBuffer);
    CHECK(Arguments != nullptr);
    Encoders.UpdateResidency(Arguments->GetResidencyEntry());

    id<MTLComputeCommandEncoder> Encoder = Encoders.RequireComputeEncoder();
    ContextState.BindComputeState(Encoder);

    [Encoder dispatchThreadgroupsWithIndirectBuffer:Arguments->GetMTLBuffer()
                               indirectBufferOffset:ArgumentBufferOffset + Arguments->GetMetalBindOffset()
                              threadsPerThreadgroup:ContextState.GetComputePipeline()->GetThreadsPerThreadgroup()];

    InsertDrawBreadcrumb(Encoders, Encoder, "DispatchIndirect");
}

void FMetalCommandContext::DispatchMesh(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    if (ThreadGroupCountX == 0 || ThreadGroupCountY == 0 || ThreadGroupCountZ == 0)
    {
        return;
    }

    if (!GMetalSupportsMeshShaders)
    {
        METAL_ERROR("DispatchMesh: mesh shaders are not supported by this Metal device");
        return;
    }

    id<MTLRenderCommandEncoder> Encoder = Encoders.GetRenderEncoder();
    ContextState.BindRenderState<EMetalRenderPipelineType::Meshlet>(Encoder);

    const FMetalMeshletPipelineStateRHI* PipelineState = static_cast<const FMetalMeshletPipelineStateRHI*>(ContextState.GetRenderPipelineState());
    [Encoder drawMeshThreadgroups:MTLSizeMake(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ)
      threadsPerObjectThreadgroup:PipelineState->GetObjectThreadgroupSize()
        threadsPerMeshThreadgroup:PipelineState->GetMeshThreadgroupSize()];

    InsertDrawBreadcrumb(Encoders, Encoder, "DispatchMesh");
}

void FMetalCommandContext::DispatchMeshIndirect(FRHIBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, uint32 CommandCount)
{
    if (CommandCount == 0)
    {
        return;
    }

    if (!GMetalSupportsMeshShaders)
    {
        METAL_ERROR("DispatchMeshIndirect: mesh shaders are not supported by this Metal device");
        return;
    }

    FMetalBufferRHI* Arguments = GetMetalBuffer(ArgumentBuffer);
    CHECK(Arguments != nullptr);
    Encoders.UpdateResidency(Arguments->GetResidencyEntry());

    id<MTLRenderCommandEncoder> Encoder = Encoders.GetRenderEncoder();
    ContextState.BindRenderState<EMetalRenderPipelineType::Meshlet>(Encoder);

    const FMetalMeshletPipelineStateRHI* PipelineState     = static_cast<const FMetalMeshletPipelineStateRHI*>(ContextState.GetRenderPipelineState());
    id<MTLBuffer>                        ArgumentMTLBuffer = Arguments->GetMTLBuffer();
    const uint64                         BaseOffset        = ArgumentBufferOffset + Arguments->GetMetalBindOffset();

    for (uint32 CommandIndex = 0; CommandIndex < CommandCount; ++CommandIndex)
    {
        [Encoder drawMeshThreadgroupsWithIndirectBuffer:ArgumentMTLBuffer
                                   indirectBufferOffset:BaseOffset + CommandIndex * sizeof(FRHIDispatchMeshIndirectParameters)
                            threadsPerObjectThreadgroup:PipelineState->GetObjectThreadgroupSize()
                              threadsPerMeshThreadgroup:PipelineState->GetMeshThreadgroupSize()];
    }

    InsertDrawBreadcrumb(Encoders, Encoder, "DispatchMeshIndirect");
}

void FMetalCommandContext::AcquireNextBackBuffer(FRHISwapChain* SwapChain)
{
    static_cast<FMetalSwapChainRHI*>(SwapChain)->AcquireNextBackBuffer();
}

void FMetalCommandContext::PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync)
{
    FMetalSwapChainRHI* MetalSwapChain = static_cast<FMetalSwapChainRHI*>(SwapChain);

    if (!Encoders.HasCommands())
    {
        MetalSwapChain->Present(nil, bVerticalSync);
        return;
    }

    Encoders.EndEncoder();
    MetalSwapChain->Present(Encoders.GetCommands().CommandBuffer, bVerticalSync);
    Submit(EMetalSubmitFlags::Reopen);
}

void FMetalCommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat Format, EColorSpace ColorSpace)
{
    Encoders.EndEncoder();
    static_cast<FMetalSwapChainRHI*>(SwapChain)->Resize(Width, Height, Format, ColorSpace);
}

void FMetalCommandContext::SetSwapChainHDRMetadata(FRHISwapChain* SwapChain, const FRHIHDRMetadata& Metadata)
{
    static_cast<FMetalSwapChainRHI*>(SwapChain)->SetHDRMetadata(Metadata);
}

void FMetalCommandContext::ClearState()
{
    ContextState.ResetState();
    Flush();
}

void FMetalCommandContext::Flush()
{
    if (Encoders.HasCommands())
    {
        Submit(EMetalSubmitFlags::Reopen);
    }

    GetDevice()->WaitForGPU();
    FMetalDeviceRHI::Get()->FlushDeferredDeletions();
}

void FMetalCommandContext::PushEvent(const StringView& Name)
{
    if (!Encoders.HasCommands())
    {
        return;
    }

    const String GroupName(Name);
    [Encoders.GetCommands().CommandBuffer pushDebugGroup:GroupName.GetNSString()];
    Encoders.GetCommands().Breadcrumbs.Push(Name);
    DebugGroups.Add(GroupName);
    UpdateScopePath();
}

void FMetalCommandContext::PopEvent()
{
    if (!Encoders.HasCommands() || DebugGroups.IsEmpty())
    {
        return;
    }

    [Encoders.GetCommands().CommandBuffer popDebugGroup];
    DebugGroups.Pop();
    UpdateScopePath();
}

void FMetalCommandContext::UpdateScopePath()
{
    String ScopePath;
    for (const String& GroupName : DebugGroups)
    {
        if (!ScopePath.IsEmpty())
        {
            ScopePath += '/';
        }

        ScopePath += GroupName;
    }

    Encoders.SetScopePath(ScopePath);
}

void FMetalCommandContext::BeginParallelChild(FMetalCommands& ParentCommands, id<MTLRenderCommandEncoder> SubEncoder)
{
    CHECK(!bIsRecording);

    AcquireOwnership();
    RecordingPool = [NSAutoreleasePool new];
    bIsRecording  = true;

    Encoders.BeginCommandBuffer(&ParentCommands);
    Encoders.AdoptRenderEncoder(SubEncoder, "ParallelRender");
    ContextState.ResetState();
    ContextState.BeginCommandBuffer();
}

void FMetalCommandContext::EndParallelChild()
{
    CHECK(bIsRecording);

    ContextState.EndCommandBuffer();
    Encoders.EndCommandBuffer();
    DebugGroups.Clear();
    bIsRecording = false;

    [RecordingPool release];
    RecordingPool = nil;

    ReleaseOwnership();
}

FMetalRenderPassInfo FMetalCommandContext::GetRenderPassInfo(MTLRenderPassDescriptor* Descriptor, uint32 NumRenderTargets)
{
    MTLRenderPassAttachmentDescriptor* Attachment = (NumRenderTargets > 0)
        ? static_cast<MTLRenderPassAttachmentDescriptor*>(Descriptor.colorAttachments[0])
        : Descriptor.depthAttachment;

    FMetalRenderPassInfo Info;

    if (id<MTLTexture> Texture = Attachment.texture)
    {
        const uint32 MipLevel = static_cast<uint32>(Attachment.level);
        Info.Extent = MTLSizeMake(MetalRHI::GetMipExtent(Texture.width, MipLevel), MetalRHI::GetMipExtent(Texture.height, MipLevel), 1);
    }

    Info.ArrayLength      = Descriptor.renderTargetArrayLength;
    Info.NumRenderTargets = static_cast<uint8>(NumRenderTargets);
    Info.bHasDepthStencil = Descriptor.depthAttachment.texture != nil;
    return Info;
}

void FMetalCommandContext::FillRenderPassDescriptor(MTLRenderPassDescriptor* Descriptor, const FRHIBeginRenderPassDesc& Desc)
{
    const uint32 NumRenderTargets = Desc.NumRenderTargets;

    FMetalDepthStencilViewRHI* MetalDSV   = static_cast<FMetalDepthStencilViewRHI*>(Desc.DepthStencilAttachment.View.Get());
    id<MTLTexture>             DSVTexture = MetalDSV ? MetalDSV->GetAttachmentTexture() : nil;
    METAL_ERROR_COND((NumRenderTargets > 0) || (DSVTexture != nil), "A RenderPass needs a valid RenderTargetView or DepthStencilView");

    Descriptor.defaultRasterSampleCount = 1;

    NSUInteger ArrayLength = 1;
    for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetAttachment& Attachment = Desc.RenderTargets[Index];

        FMetalRenderTargetViewRHI* MetalRTV = static_cast<FMetalRenderTargetViewRHI*>(Attachment.View.Get());
        METAL_ERROR_COND(MetalRTV != nullptr, "RenderTargetView cannot be nullptr");

        MTLRenderPassColorAttachmentDescriptor* ColorAttachment = Descriptor.colorAttachments[Index];
        MetalRTV->ApplyToAttachment(ColorAttachment);
        Encoders.UpdateResidency(MetalRTV->GetAttachmentResidencyEntry());
        ColorAttachment.loadAction  = MetalRHI::ConvertAttachmentLoadAction(Attachment.LoadAction);
        ColorAttachment.storeAction = MetalRHI::ConvertAttachmentStoreAction(Attachment.StoreAction);
        ColorAttachment.clearColor  = MTLClearColorMake(Attachment.ClearValue.R, Attachment.ClearValue.G, Attachment.ClearValue.B, Attachment.ClearValue.A);

        if (MetalRTV->GetArrayIndex() == 0)
        {
            ArrayLength = Math::Max<NSUInteger>(ArrayLength, MetalRTV->GetNumSlices());
        }
    }

    if (DSVTexture)
    {
        const FRHIDepthStencilAttachment& DepthStencilAttachment = Desc.DepthStencilAttachment;

        MTLRenderPassDepthAttachmentDescriptor* DepthAttachment = Descriptor.depthAttachment;
        MetalDSV->ApplyToAttachment(DepthAttachment);
        Encoders.UpdateResidency(MetalDSV->GetAttachmentResidencyEntry());
        DepthAttachment.loadAction  = MetalRHI::ConvertAttachmentLoadAction(DepthStencilAttachment.LoadAction);
        DepthAttachment.storeAction = MetalRHI::ConvertAttachmentStoreAction(DepthStencilAttachment.StoreAction);
        DepthAttachment.clearDepth  = DepthStencilAttachment.ClearValue.Depth;

        if (MetalRHI::IsStencilPixelFormat(DSVTexture.pixelFormat))
        {
            const bool bReadOnlyStencil = IsEnumFlagSet(MetalDSV->GetFlags(), EDepthStencilViewFlags::ReadOnlyStencil);

            MTLRenderPassStencilAttachmentDescriptor* StencilAttachment = Descriptor.stencilAttachment;
            MetalDSV->ApplyToAttachment(StencilAttachment);
            StencilAttachment.loadAction   = MetalRHI::ConvertAttachmentLoadAction(DepthStencilAttachment.LoadAction);
            StencilAttachment.storeAction  = bReadOnlyStencil
                ? MTLStoreActionDontCare
                : MetalRHI::ConvertAttachmentStoreAction(DepthStencilAttachment.StoreAction);
            StencilAttachment.clearStencil = static_cast<uint32>(DepthStencilAttachment.ClearValue.Stencil);
        }

        if (MetalDSV->GetArrayIndex() == 0)
        {
            ArrayLength = Math::Max<NSUInteger>(ArrayLength, MetalDSV->GetNumSlices());
        }
    }

    if (id<MTLBuffer> VisibilityBuffer = GetDevice()->GetOcclusionQueries().GetBuffer())
    {
        Descriptor.visibilityResultBuffer = VisibilityBuffer;
    }

    const FRHIViewInstancingState& ViewInstancingState = Desc.ViewInstancingState;

    if (ViewInstancingState.bEnableViewInstancing && ViewInstancingState.NumArraySlices > 0)
    {
        const NSUInteger ViewInstancingLength = static_cast<NSUInteger>(ViewInstancingState.StartRenderTargetArrayIndex) + ViewInstancingState.NumArraySlices;
        ArrayLength = Math::Max(ArrayLength, ViewInstancingLength);

        id<MTLTexture> FirstTexture = (NumRenderTargets > 0)
            ? Descriptor.colorAttachments[0].texture
            : Descriptor.depthAttachment.texture;

        if (FirstTexture && FirstTexture.arrayLength > 0)
        {
            ArrayLength = Math::Min<NSUInteger>(ArrayLength, FirstTexture.arrayLength);
        }
    }

    Descriptor.renderTargetArrayLength = ArrayLength;

    const FRHISamplePositionsDesc& SamplePositions = ContextState.GetSamplePositions();

    if (SamplePositions.NumSamplesPerPixel > 0)
    {
        MTLSamplePosition Positions[RHI_MAX_SAMPLE_POSITIONS];
        Memory::Memzero(Positions, sizeof(Positions));

        const NSUInteger Count = Math::Min(static_cast<NSUInteger>(SamplePositions.NumSamplesPerPixel), static_cast<NSUInteger>(RHI_MAX_SAMPLE_POSITIONS));
        for (NSUInteger Index = 0; Index < Count; ++Index)
        {
            Positions[Index].x = Math::Clamp(0.5f + SamplePositions.Positions[Index].X, 0.0f, 1.0f);
            Positions[Index].y = Math::Clamp(0.5f + SamplePositions.Positions[Index].Y, 0.0f, 1.0f);
        }

        [Descriptor setSamplePositions:Positions count:Count];
    }
}

#if METAL_VALIDATE_CONTEXT_THREAD_OWNERSHIP
void FMetalCommandContext::AcquireOwnership()
{
    CHECK(OwnerThreadID.Load() == CORE_INVALID_THREAD_ID);
    OwnerThreadID.Store(FPlatformTLS::GetCurrentThreadID());
}

void FMetalCommandContext::ReleaseOwnership()
{
    VerifyOwnerThread();
    OwnerThreadID.Store(CORE_INVALID_THREAD_ID);
}

void FMetalCommandContext::VerifyOwnerThread() const
{
    CHECK(OwnerThreadID.Load() == FPlatformTLS::GetCurrentThreadID());
}
#endif

ENABLE_UNREFERENCED_VARIABLE_WARNING
