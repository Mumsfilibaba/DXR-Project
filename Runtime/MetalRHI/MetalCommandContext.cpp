#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalSwapChain.h"
#include "MetalRHI/MetalPipelineState.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

static MTLIndexType ConvertIndexFormat(EIndexFormat IndexFormat)
{
    return (IndexFormat == EIndexFormat::uint32) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16;
}

static NSUInteger GetMipExtent(NSUInteger Extent, uint32 MipLevel)
{
    const NSUInteger MipExtent = Extent >> MipLevel;
    return (MipExtent > 0) ? MipExtent : 1;
}

static NSUInteger ResolveMipCopyExtent(uint32 RequestedEnd, NSUInteger SrcOrigin, NSUInteger DstOrigin, NSUInteger SrcExtent, NSUInteger DstExtent)
{
    if (SrcOrigin >= SrcExtent || DstOrigin >= DstExtent)
    {
        return 0;
    }

    const NSUInteger Requested = (NSUInteger(RequestedEnd) > SrcOrigin) ? (NSUInteger(RequestedEnd) - SrcOrigin) : 1;
    const NSUInteger Available = Math::Min<NSUInteger>(SrcExtent - SrcOrigin, DstExtent - DstOrigin);
    return Math::Min<NSUInteger>(Requested, Available);
}

FMetalCommandContext::FMetalCommandContext(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , IRHICommandContext()
    , CommandBuffer(nil)
    , Commands(nullptr)
    , GraphicsEncoder(nil)
    , ComputeEncoder(nil)
    , CopyContext()
    , ContextState(InDevice, *this)
{
}

FMetalCommandContext::~FMetalCommandContext() = default;

bool FMetalCommandContext::Initialize()
{
    if (!ContextState.Initialize())
    {
        METAL_ERROR_CRITICAL("Failed to initialize ContextState");
        return false;
    }

    return true;
}

void FMetalCommandContext::StartContext() 
{
    CHECK(CommandBuffer == nil);

    FMetalQueue* Queue = GetDevice()->GetQueue();
    Commands      = Queue->ObtainCommands();
    CommandBuffer = Commands->CommandBuffer;

    ContextState.BeginCommandBuffer();
}

void FMetalCommandContext::FinishContext()
{
    CHECK(CommandBuffer != nil);

    ContextState.EndCommandBuffer();

    FinishEncoders();

    FMetalDeviceRHI::Get()->FlushDeletionQueue(Commands);
    GetDevice()->GetQueue()->SubmitCommands(Commands);

    Commands      = nullptr;
    CommandBuffer = nil;
}

void FMetalCommandContext::QueryTimestamp(FRHIQuery* Query)
{
}

void FMetalCommandContext::BeginFrame()
{
}

void FMetalCommandContext::EndFrame()
{
}

void FMetalCommandContext::BeginQuery(FRHIQuery* Query)
{
}

void FMetalCommandContext::EndQuery(FRHIQuery* Query)
{
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

    FMetalTextureRHI* RTVTexture = GetMetalTexture(static_cast<FRHITexture*>(MetalRTV->GetResource()));

    MTLRenderPassDescriptor* RenderPassDescriptor = [MTLRenderPassDescriptor new];
    MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPassDescriptor.colorAttachments[0];

    ColorAttachment.texture            = RTVTexture->GetMTLTexture();
    ColorAttachment.loadAction         = MTLLoadActionClear;
    ColorAttachment.clearColor         = MTLClearColorMake(ClearColor.X, ClearColor.Y, ClearColor.Z, ClearColor.W);
    ColorAttachment.level              = MetalRTV->GetMipLevel();
    ColorAttachment.slice              = MetalRTV->GetArrayIndex();
    ColorAttachment.storeActionOptions = MTLStoreActionOptionNone;
    ColorAttachment.storeAction        = MTLStoreActionStore;

    FinishEncoders();

    id<MTLRenderCommandEncoder> ClearEncoder = [CommandBuffer renderCommandEncoderWithDescriptor:RenderPassDescriptor];

    [RenderPassDescriptor release];

    [ClearEncoder endEncoding];
}

void FMetalCommandContext::ClearDepthStencilView(FRHIDepthStencilView* DepthStencilView, const float Depth, uint8 Stencil)
{
}

void FMetalCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const Vector4& ClearColor)
{
}

void FMetalCommandContext::ClearUnorderedAccessViewUint(FRHIUnorderedAccessView* UnorderedAccessView, const uint32 Values[4])
{
}

void FMetalCommandContext::BeginRenderPass(const FRHIBeginRenderPassDesc& BeginRenderPassDesc)
{
    SCOPED_AUTORELEASE_POOL();
    
    CHECK(GraphicsEncoder == nil);
    
    FinishEncoders();

    FMetalRenderTargetViewRHI* CachedRenderTargets[RHI_MAX_RENDER_TARGETS] = { };
    const uint32 NumRenderTargets = BeginRenderPassDesc.NumRenderTargets;
    for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
    {
        CachedRenderTargets[Index] = static_cast<FMetalRenderTargetViewRHI*>(BeginRenderPassDesc.RenderTargets[Index].View.Get());
    }

    FMetalDepthStencilViewRHI* MetalDSV = static_cast<FMetalDepthStencilViewRHI*>(BeginRenderPassDesc.DepthStencilAttachment.View.Get());
    ContextState.SetRenderTargets(CachedRenderTargets, NumRenderTargets, MetalDSV);

    FMetalTextureRHI* DSVTexture = MetalDSV ? GetMetalTexture(static_cast<FRHITexture*>(MetalDSV->GetResource())) : nullptr;
    METAL_ERROR_COND((NumRenderTargets > 0) || (DSVTexture != nullptr), "A RenderPass needs a valid RenderTargetView or DepthStencilView");
    
    MTLRenderPassDescriptor* RenderPassDescriptor = [MTLRenderPassDescriptor new];
    RenderPassDescriptor.defaultRasterSampleCount = 1;
    RenderPassDescriptor.renderTargetArrayLength  = 1;
    
    for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetAttachment& Attachment = BeginRenderPassDesc.RenderTargets[Index];
        FMetalRenderTargetViewRHI* MetalRTV = static_cast<FMetalRenderTargetViewRHI*>(Attachment.View.Get());
        METAL_ERROR_COND(MetalRTV != nullptr, "RenderTargetView cannot be nullptr");

        FMetalTextureRHI* RTVTexture = GetMetalTexture(static_cast<FRHITexture*>(MetalRTV->GetResource()));
        METAL_ERROR_COND(RTVTexture != nullptr, "Texture cannot be nullptr");

        MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPassDescriptor.colorAttachments[Index];
        ColorAttachment.texture            = RTVTexture->GetMTLTexture();
        ColorAttachment.loadAction         = MetalRHI::ConvertAttachmentLoadAction(Attachment.LoadAction);
        ColorAttachment.level              = MetalRTV->GetMipLevel();
        ColorAttachment.slice              = MetalRTV->GetArrayIndex();
        ColorAttachment.storeActionOptions = MTLStoreActionOptionNone;
        ColorAttachment.storeAction        = MetalRHI::ConvertAttachmentStoreAction(Attachment.StoreAction);
        ColorAttachment.clearColor         = MTLClearColorMake(Attachment.ClearValue.R, Attachment.ClearValue.G, Attachment.ClearValue.B, Attachment.ClearValue.A);
    }

    if (DSVTexture)
    {
        const FRHIDepthStencilAttachment& DepthStencilAttachment = BeginRenderPassDesc.DepthStencilAttachment;

        MTLRenderPassDepthAttachmentDescriptor* DepthAttachment = RenderPassDescriptor.depthAttachment;
        DepthAttachment.texture            = DSVTexture->GetMTLTexture();
        DepthAttachment.loadAction         = MetalRHI::ConvertAttachmentLoadAction(DepthStencilAttachment.LoadAction);
        DepthAttachment.clearDepth         = DepthStencilAttachment.ClearValue.Depth;
        DepthAttachment.level              = MetalDSV->GetMipLevel();
        DepthAttachment.slice              = MetalDSV->GetArrayIndex();
        DepthAttachment.storeActionOptions = MTLStoreActionOptionNone;
        DepthAttachment.storeAction        = MetalRHI::ConvertAttachmentStoreAction(DepthStencilAttachment.StoreAction);
    }
    
    // TODO: Stencil Attachment

    CHECK(RenderPassDescriptor != nil);
    GraphicsEncoder = [CommandBuffer renderCommandEncoderWithDescriptor:RenderPassDescriptor];
    [GraphicsEncoder retain];
    
    [RenderPassDescriptor release];
}

void FMetalCommandContext::EndRenderPass()
{
    CHECK(GraphicsEncoder != nil);
        
    [GraphicsEncoder endEncoding];
    [GraphicsEncoder release];
    GraphicsEncoder = nil;
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
    // TODO: Forward the scissor rectangle once the ImGui path supplies valid bounds.
    UNREFERENCED_VARIABLE(ScissorRegion);
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
    UNREFERENCED_VARIABLE(MinDepth);
    UNREFERENCED_VARIABLE(MaxDepth);
}

void FMetalCommandContext::SetSamplePositions(const FRHISamplePositionsDesc& SamplePositionsDesc)
{
    UNREFERENCED_VARIABLE(SamplePositionsDesc);
}

void FMetalCommandContext::SetStreamOutputTargets(const TArrayView<FRHIBuffer* const> Buffers, const uint64* Offsets)
{
}

void FMetalCommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 BufferIndex = 0; BufferIndex < InVertexBuffers.Size(); ++BufferIndex)
    {
        FMetalBufferRHI* Buffer = static_cast<FMetalBufferRHI*>(InVertexBuffers[BufferIndex]);
        ContextState.SetVertexBuffer(Buffer, BufferSlot + BufferIndex);
    }
}

void FMetalCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    FMetalBufferRHI* MetalIndexBuffer = static_cast<FMetalBufferRHI*>(IndexBuffer);
    ContextState.SetIndexBuffer(MetalIndexBuffer, ConvertIndexFormat(IndexFormat));
}

void FMetalCommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    FMetalGraphicsPipelineStateRHI* MetalPipelineState = static_cast<FMetalGraphicsPipelineStateRHI*>(PipelineState);
    ContextState.SetGraphicsPipelineState(MetalPipelineState);
}

void FMetalCommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
    FMetalComputePipelineStateRHI* MetalPipelineState = static_cast<FMetalComputePipelineStateRHI*>(PipelineState);
    ContextState.SetComputePipelineState(MetalPipelineState);
}

void FMetalCommandContext::SetMeshletPipelineState(FRHIMeshletPipelineState* PipelineState)
{
    FMetalMeshletPipelineStateRHI* MetalPipelineState = static_cast<FMetalMeshletPipelineStateRHI*>(PipelineState);
    ContextState.SetMeshletPipelineState(MetalPipelineState);
}

void FMetalCommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    if (!MetalShader)
    {
        return;
    }

    ContextState.SetShaderConstants(MetalShader->GetVisibility(), reinterpret_cast<const uint32*>(ShaderConstants), NumShaderConstants);
}

void FMetalCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 RegisterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);

    FMetalShaderResourceViewRHI* MetalSRV = static_cast<FMetalShaderResourceViewRHI*>(ShaderResourceView);
    ContextState.SetSRV(MetalSRV, MetalShader->GetVisibility(), RegisterIndex);
}

void FMetalCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 RegisterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);

    const EShaderVisibility::Type Visibility = MetalShader->GetVisibility();
    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        FMetalShaderResourceViewRHI* MetalSRV = static_cast<FMetalShaderResourceViewRHI*>(InShaderResourceViews[Index]);
        ContextState.SetSRV(MetalSRV, Visibility, RegisterIndex + Index);
    }
}

void FMetalCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 RegisterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);

    FMetalUnorderedAccessViewRHI* MetalUAV = static_cast<FMetalUnorderedAccessViewRHI*>(UnorderedAccessView);
    ContextState.SetUAV(MetalUAV, MetalShader->GetVisibility(), RegisterIndex);
}

void FMetalCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 RegisterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);

    const EShaderVisibility::Type Visibility = MetalShader->GetVisibility();
    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        FMetalUnorderedAccessViewRHI* MetalUAV = static_cast<FMetalUnorderedAccessViewRHI*>(InUnorderedAccessViews[Index]);
        ContextState.SetUAV(MetalUAV, Visibility, RegisterIndex + Index);
    }
}

void FMetalCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 RegisterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);

    FMetalBufferRHI* MetalBuffer = static_cast<FMetalBufferRHI*>(ConstantBuffer);
    ContextState.SetCBV(MetalBuffer, MetalShader->GetVisibility(), RegisterIndex);
}

void FMetalCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 RegisterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);

    const EShaderVisibility::Type Visibility = MetalShader->GetVisibility();
    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        FMetalBufferRHI* MetalBuffer = static_cast<FMetalBufferRHI*>(InConstantBuffers[Index]);
        ContextState.SetCBV(MetalBuffer, Visibility, RegisterIndex + Index);
    }
}

void FMetalCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 RegisterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);

    FMetalSamplerStateRHI* MetalSamplerState = static_cast<FMetalSamplerStateRHI*>(SamplerState);
    ContextState.SetSampler(MetalSamplerState, MetalShader->GetVisibility(), RegisterIndex);
}

void FMetalCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 RegisterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);

    const EShaderVisibility::Type Visibility = MetalShader->GetVisibility();
    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        FMetalSamplerStateRHI* MetalSamplerState = static_cast<FMetalSamplerStateRHI*>(InSamplerStates[Index]);
        ContextState.SetSampler(MetalSamplerState, Visibility, RegisterIndex + Index);
    }
}

id<MTLBuffer> FMetalCommandContext::CreateStagingBuffer(uint64 Size)
{
    if (!Commands || Size == 0)
    {
        return nil;
    }

    id<MTLDevice> DeviceHandle  = GetDevice()->GetMTLDevice();
    id<MTLBuffer> StagingBuffer = [DeviceHandle newBufferWithLength:Size options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache];
    if (!StagingBuffer)
    {
        METAL_ERROR("Failed to allocate a %llu byte staging buffer", Size);
        return nil;
    }

    Commands->DeferredObjects.Emplace(StagingBuffer);
    [StagingBuffer release];
    return StagingBuffer;
}

void FMetalCommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SourceData)
{
    SCOPED_AUTORELEASE_POOL();

    FMetalBufferRHI* MetalDst = GetMetalBuffer(Dst);
    if (!MetalDst || !SourceData || BufferRegion.Size == 0)
    {
        return;
    }

    id<MTLBuffer> DstBuffer = MetalDst->GetMTLBuffer();
    CHECK(DstBuffer != nil);

    const uint64 Size = BufferRegion.IsWholeResource() ? MetalDst->GetDesc().Size : BufferRegion.Size;

    if (DstBuffer.storageMode == MTLStorageModeShared)
    {
        Memory::Memcpy(reinterpret_cast<uint8*>(DstBuffer.contents) + BufferRegion.Offset, SourceData, Size);
        return;
    }

    CHECK(CommandBuffer != nil);

    id<MTLBuffer> StagingBuffer = CreateStagingBuffer(Size);
    if (!StagingBuffer)
    {
        return;
    }

    Memory::Memcpy(StagingBuffer.contents, SourceData, Size);

    StartCopyEncoder();
    [CopyContext.GetMTLCopyEncoder() copyFromBuffer:StagingBuffer
                                       sourceOffset:0
                                           toBuffer:DstBuffer
                                  destinationOffset:BufferRegion.Offset
                                               size:Size];
}

void FMetalCommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SourceData, uint32 SrcRowPitch)
{
    const FTextureRegion3D Region3D(TextureRegion.Width, TextureRegion.Height, 1, TextureRegion.PositionX, TextureRegion.PositionY, 0);
    UpdateTexture3D(Dst, Region3D, MipLevel, SourceData, SrcRowPitch, SrcRowPitch * TextureRegion.Height);
}

void FMetalCommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
    SCOPED_AUTORELEASE_POOL();

    FMetalTextureRHI* MetalDst = GetMetalTexture(Dst);
    if (!MetalDst || !SrcData || TextureRegion.Width == 0 || TextureRegion.Height == 0)
    {
        return;
    }

    id<MTLTexture> DstTexture = MetalDst->GetMTLTexture();
    CHECK(CommandBuffer != nil);
    CHECK(DstTexture    != nil);

    const FRHITextureDesc& DstDesc = MetalDst->GetDesc();
    const bool   bIsTexture1D = DstDesc.IsTexture1D() || DstDesc.IsTexture1DArray();
    const bool   bIsTexture3D = DstDesc.IsTexture3D();
    const uint32 Depth        = Math::Max(TextureRegion.Depth, 1u);
    const uint64 DataSize     = uint64(SrcDepthPitch) * Depth;

    id<MTLBuffer> StagingBuffer = CreateStagingBuffer(DataSize);
    if (!StagingBuffer)
    {
        return;
    }

    Memory::Memcpy(StagingBuffer.contents, SrcData, DataSize);

    StartCopyEncoder();
    [CopyContext.GetMTLCopyEncoder() copyFromBuffer:StagingBuffer
                                       sourceOffset:0
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
}

void FMetalCommandContext::TranscodeSamplerFeedback(FRHITexture* Dst, uint32 DstSubresource, FRHITexture* Src, uint32 SrcSubresource, ESamplerFeedbackTranscodeMode Mode)
{
}

void FMetalCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    FMetalBufferRHI* MetalDst = GetMetalBuffer(Dst);
    FMetalBufferRHI* MetalSrc = GetMetalBuffer(Src);
    
    CHECK(CommandBuffer != nil);
    CHECK(MetalDst      != nullptr);
    CHECK(MetalSrc      != nullptr);
    
    StartCopyEncoder();
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder copyFromBuffer:MetalSrc->GetMTLBuffer()
                   sourceOffset:CopyDesc.SrcOffset
                       toBuffer:MetalDst->GetMTLBuffer()
              destinationOffset:CopyDesc.DstOffset
                           size:CopyDesc.Size];
}

void FMetalCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FMetalTextureRHI* MetalDst = GetMetalTexture(Dst);
    FMetalTextureRHI* MetalSrc = GetMetalTexture(Src);
    
    CHECK(CommandBuffer != nil);
    CHECK(MetalDst      != nullptr);
    CHECK(MetalSrc      != nullptr);
    
    StartCopyEncoder();
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder copyFromTexture:MetalSrc->GetMTLTexture() toTexture:MetalDst->GetMTLTexture()];
}

void FMetalCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc) 
{ 
    FMetalTextureRHI* MetalDst = GetMetalTexture(Dst);
    FMetalTextureRHI* MetalSrc = GetMetalTexture(Src);

    CHECK(CommandBuffer != nil);
    CHECK(MetalDst      != nullptr);
    CHECK(MetalSrc      != nullptr);

    StartCopyEncoder();

    id<MTLTexture> SrcTexture = MetalSrc->GetMTLTexture();
    id<MTLTexture> DstTexture = MetalDst->GetMTLTexture();

    const uint32 NumArraySlices = Math::Max(CopyDesc.NumArraySlices, 1u);
    const uint32 NumMipLevels   = Math::Max(CopyDesc.NumMipLevels, 1u);

    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    for (uint32 ArrayIndex = 0; ArrayIndex < NumArraySlices; ++ArrayIndex)
    {
        for (uint32 MipIndex = 0; MipIndex < NumMipLevels; ++MipIndex)
        {
            const uint32 SrcMipLevel = CopyDesc.SrcMipSlice + MipIndex;
            const uint32 DstMipLevel = CopyDesc.DstMipSlice + MipIndex;

            const MTLOrigin SrcOrigin = MTLOriginMake(CopyDesc.SrcPosition.X >> MipIndex, CopyDesc.SrcPosition.Y >> MipIndex, CopyDesc.SrcPosition.Z >> MipIndex);
            const MTLOrigin DstOrigin = MTLOriginMake(CopyDesc.DstPosition.X >> MipIndex, CopyDesc.DstPosition.Y >> MipIndex, CopyDesc.DstPosition.Z >> MipIndex);

            const MTLSize Size = MTLSizeMake(
                ResolveMipCopyExtent(uint32(CopyDesc.SrcPosition.X + CopyDesc.Size.X) >> MipIndex, SrcOrigin.x, DstOrigin.x, GetMipExtent(SrcTexture.width,  SrcMipLevel), GetMipExtent(DstTexture.width,  DstMipLevel)),
                ResolveMipCopyExtent(uint32(CopyDesc.SrcPosition.Y + CopyDesc.Size.Y) >> MipIndex, SrcOrigin.y, DstOrigin.y, GetMipExtent(SrcTexture.height, SrcMipLevel), GetMipExtent(DstTexture.height, DstMipLevel)),
                ResolveMipCopyExtent(uint32(CopyDesc.SrcPosition.Z + CopyDesc.Size.Z) >> MipIndex, SrcOrigin.z, DstOrigin.z, GetMipExtent(SrcTexture.depth,  SrcMipLevel), GetMipExtent(DstTexture.depth,  DstMipLevel)));

            if (Size.width == 0 || Size.height == 0 || Size.depth == 0)
            {
                continue;
            }

            [CopyEncoder copyFromTexture:SrcTexture
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

    CHECK(CommandBuffer != nil);
    CHECK(MetalDst      != nullptr);
    CHECK(MetalSrc      != nullptr);

    const FRHITextureDesc& SrcDesc = MetalSrc->GetDesc();
    const bool   bIsTexture3D = SrcDesc.IsTexture3D();
    const uint32 Depth        = Math::Max(SrcRegion.Depth, 1u);
    const uint64 RowPitch     = uint64(SrcRegion.Width) * GetByteStrideFromFormat(SrcDesc.Format);
    const uint64 SlicePitch   = RowPitch * SrcRegion.Height;

    StartCopyEncoder();

    [CopyContext.GetMTLCopyEncoder() copyFromTexture:MetalSrc->GetMTLTexture()
                                         sourceSlice:SrcArraySlice
                                         sourceLevel:SrcMipLevel
                                        sourceOrigin:MTLOriginMake(SrcRegion.PositionX, SrcRegion.PositionY, SrcRegion.PositionZ)
                                          sourceSize:MTLSizeMake(SrcRegion.Width, SrcRegion.Height, Depth)
                                            toBuffer:MetalDst->GetMTLBuffer()
                                   destinationOffset:DstOffset
                              destinationBytesPerRow:RowPitch
                            destinationBytesPerImage:(bIsTexture3D ? SlicePitch : 0)];
}

void FMetalCommandContext::WriteFence(FRHIFence* Fence) 
{ 
} 

void FMetalCommandContext::DiscardContents(class FRHITexture* Texture)
{
}

void FMetalCommandContext::BuildSceneAccelerationStructure(FRHISceneAccelerationStructure* RayTracingScene, const FRHISceneAccelerationStructureBuildDesc& BuildDesc)
{
}

void FMetalCommandContext::BuildGeometryAccelerationStructure(FRHIGeometryAccelerationStructure* RayTracingGeometry, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc)
{
}

void FMetalCommandContext::TransitionBarrier(TArrayView<const FRHITransitionBarrierDesc> TransitionDescs)
{
}

void FMetalCommandContext::UnorderedAccessBarrier(TArrayView<const FRHIUnorderedAccessBarrierDesc> BarrierDescs)
{
}

void FMetalCommandContext::PrepareForDraw()
{
    CHECK(GraphicsEncoder != nil);

    ContextState.PrepareGraphicsState();
    ContextState.BindGraphicsState();
}

void FMetalCommandContext::PrepareForDispatch()
{
    if (ComputeEncoder == nil)
    {
        CHECK(CommandBuffer != nil);

        FinishEncoders();

        ComputeEncoder = [CommandBuffer computeCommandEncoder];
        [ComputeEncoder retain];
    }

    ContextState.PrepareComputeState();
    ContextState.BindComputeState();
}

void FMetalCommandContext::StartCopyEncoder()
{
    CHECK(CommandBuffer != nil);

    if (GraphicsEncoder || ComputeEncoder)
    {
        FinishEncoders();
    }

    CopyContext.StartEncoder(CommandBuffer);
}

void FMetalCommandContext::FinishEncoders()
{
    if (GraphicsEncoder)
    {
        [GraphicsEncoder endEncoding];
        [GraphicsEncoder release];
        GraphicsEncoder = nil;
    }

    if (ComputeEncoder)
    {
        [ComputeEncoder endEncoding];
        [ComputeEncoder release];
        ComputeEncoder = nil;
    }

    CopyContext.FinishEncoder();
}

void FMetalCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    const MTLPrimitiveType PrimitiveType = ContextState.GetPrimitiveType();
    CHECK(PrimitiveType != MTLPrimitiveType(-1));
}

void FMetalCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    const FMetalIndexBufferCache& IndexBufferCache = ContextState.GetIndexBufferCache();
    const MTLPrimitiveType        PrimitiveType    = ContextState.GetPrimitiveType();
    CHECK(IndexBufferCache.IndexBuffer != nil);
    CHECK(PrimitiveType                != MTLPrimitiveType(-1));
}

void FMetalCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    const MTLPrimitiveType PrimitiveType = ContextState.GetPrimitiveType();
    CHECK(PrimitiveType != MTLPrimitiveType(-1));
}

void FMetalCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    const FMetalIndexBufferCache& IndexBufferCache = ContextState.GetIndexBufferCache();
    const MTLPrimitiveType        PrimitiveType    = ContextState.GetPrimitiveType();
    CHECK(IndexBufferCache.IndexBuffer != nil);
    CHECK(PrimitiveType                != MTLPrimitiveType(-1));
}

void FMetalCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    PrepareForDispatch();
}

void FMetalCommandContext::DispatchMesh(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
    // Mesh shaders are not yet implemented on Metal.
    UNREFERENCED_VARIABLE(ThreadGroupCountX);
    UNREFERENCED_VARIABLE(ThreadGroupCountY);
    UNREFERENCED_VARIABLE(ThreadGroupCountZ);
}

void FMetalCommandContext::AcquireNextBackBuffer(FRHISwapChain* SwapChain)
{
    FMetalSwapChainRHI* MetalSwapChain = static_cast<FMetalSwapChainRHI*>(SwapChain);
    MetalSwapChain->AcquireNextBackBuffer();
}

void FMetalCommandContext::PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync)
{
    FMetalSwapChainRHI* MetalSwapChain = static_cast<FMetalSwapChainRHI*>(SwapChain);
    MetalSwapChain->Present(bVerticalSync);
}

void FMetalCommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height, EFormat /*Format*/, EColorSpace /*ColorSpace*/)
{
    FMetalSwapChainRHI* MetalSwapChain = static_cast<FMetalSwapChainRHI*>(SwapChain);
    const uint32 ResolvedWidth  = (Width  > 0u) ? Width  : MetalSwapChain->GetDesc().Width;
    const uint32 ResolvedHeight = (Height > 0u) ? Height : MetalSwapChain->GetDesc().Height;
    MetalSwapChain->Resize(ResolvedWidth, ResolvedHeight);
}

void FMetalCommandContext::SetSwapChainHDRMetadata(FRHISwapChain* SwapChain, const FRHIHDRMetadata& Metadata)
{
    FMetalSwapChainRHI* MetalSwapChain = static_cast<FMetalSwapChainRHI*>(SwapChain);
    MetalSwapChain->SetHDRMetadata(Metadata);
}

void FMetalCommandContext::ClearState()
{
    ContextState.ResetState();
    Flush();
}

void FMetalCommandContext::Flush()
{
    if (Commands)
    {
        ContextState.EndCommandBuffer();
        FinishEncoders();

        FMetalDeviceRHI::Get()->FlushDeletionQueue(Commands);
        GetDevice()->GetQueue()->SubmitCommands(Commands);

        Commands      = nullptr;
        CommandBuffer = nil;
    }

    GetDevice()->GetQueue()->WaitForCompletion();
}

void FMetalCommandContext::PushEvent(const StringView& Name)
{
    SCOPED_AUTORELEASE_POOL();
    
    id<MTLCommandEncoder> Encoder = nil;
    if (GraphicsEncoder)
    {
        Encoder = GraphicsEncoder;
    }
    else if (ComputeEncoder)
    {
        Encoder = ComputeEncoder;
    }
    else
    {
        StartCopyEncoder();
        Encoder = CopyContext.GetMTLCopyEncoder();
    }

    [Encoder pushDebugGroup:String(Name).GetNSString()];
}

void FMetalCommandContext::PopEvent()
{
    SCOPED_AUTORELEASE_POOL();
    
    id<MTLCommandEncoder> Encoder = nil;
    if (GraphicsEncoder)
    {
        Encoder = GraphicsEncoder;
    }
    else if (ComputeEncoder)
    {
        Encoder = ComputeEncoder;
    }
    else
    {
        Encoder = CopyContext.GetMTLCopyEncoder();
    }

    if (Encoder)
    {
        [Encoder popDebugGroup];
    }
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
