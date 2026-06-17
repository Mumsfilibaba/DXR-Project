#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalSwapChain.h"
#include "MetalRHI/MetalPipelineState.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

static MTLIndexType ConvertIndexFormat(EIndexFormat IndexFormat)
{
    return (IndexFormat == EIndexFormat::uint32) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16;
}

FMetalCommandContext::FMetalCommandContext(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , IRHICommandContext()
    , CommandBuffer(nil)
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
    
    id<MTLCommandQueue> CommandQueue = GetDevice()->GetMTLCommandQueue();
    CommandBuffer = [CommandQueue commandBuffer];

    ContextState.ResetStateForNewCommandBuffer();
}

void FMetalCommandContext::FinishContext()
{
    CHECK(CommandBuffer != nil);
    
    CopyContext.FinishEncoder();
    
    [CommandBuffer commit];
    [CommandBuffer release];
    
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

    if (!GraphicsEncoder)
    {
        GraphicsEncoder = [CommandBuffer renderCommandEncoderWithDescriptor:RenderPassDescriptor];
    }

    [RenderPassDescriptor release];

    [GraphicsEncoder endEncoding];
    GraphicsEncoder = nil;
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
    
    CopyContext.FinishEncoder();

    FMetalRenderTargetViewRHI* CachedRenderTargets[RHI_MAX_RENDER_TARGETS] = { };
    const uint32 NumRenderTargets = BeginRenderPassDesc.NumRenderTargets;
    for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
    {
        CachedRenderTargets[Index] = static_cast<FMetalRenderTargetViewRHI*>(BeginRenderPassDesc.RenderTargets[Index].View);
    }

    FMetalDepthStencilViewRHI* MetalDSV = static_cast<FMetalDepthStencilViewRHI*>(BeginRenderPassDesc.DepthStencilAttachment.View);
    ContextState.SetRenderTargets(CachedRenderTargets, NumRenderTargets, MetalDSV);

    FMetalTextureRHI* DSVTexture = MetalDSV ? GetMetalTexture(static_cast<FRHITexture*>(MetalDSV->GetResource())) : nullptr;
    METAL_ERROR_COND((NumRenderTargets > 0) || (DSVTexture != nullptr), "A RenderPass needs a valid RenderTargetView or DepthStencilView");
    
    MTLRenderPassDescriptor* RenderPassDescriptor = [MTLRenderPassDescriptor new];
    RenderPassDescriptor.defaultRasterSampleCount = 1;
    RenderPassDescriptor.renderTargetArrayLength  = 1;
    
    for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
    {
        const FRHIRenderPassAttachment& Attachment = BeginRenderPassDesc.RenderTargets[Index];
        FMetalRenderTargetViewRHI* MetalRTV = static_cast<FMetalRenderTargetViewRHI*>(Attachment.View);
        METAL_ERROR_COND(MetalRTV != nullptr, "RenderTargetView cannot be nullptr");

        FMetalTextureRHI* RTVTexture = GetMetalTexture(static_cast<FRHITexture*>(MetalRTV->GetResource()));
        METAL_ERROR_COND(RTVTexture != nullptr, "Texture cannot be nullptr");

        MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPassDescriptor.colorAttachments[Index];
        ColorAttachment.texture            = RTVTexture->GetMTLTexture();
        ColorAttachment.loadAction         = ConvertAttachmentLoadAction(Attachment.LoadAction);
        ColorAttachment.level              = MetalRTV->GetMipLevel();
        ColorAttachment.slice              = MetalRTV->GetArrayIndex();
        ColorAttachment.storeActionOptions = MTLStoreActionOptionNone;
        ColorAttachment.storeAction        = ConvertAttachmentStoreAction(Attachment.StoreAction);
        ColorAttachment.clearColor         = MTLClearColorMake(Attachment.ClearValue.R, Attachment.ClearValue.G, Attachment.ClearValue.B, Attachment.ClearValue.A);
    }

    if (DSVTexture)
    {
        const FRHIDepthStencilAttachment& DepthStencilAttachment = BeginRenderPassDesc.DepthStencilAttachment;

        MTLRenderPassDepthAttachmentDescriptor* DepthAttachment = RenderPassDescriptor.depthAttachment;
        DepthAttachment.texture            = DSVTexture->GetMTLTexture();
        DepthAttachment.loadAction         = ConvertAttachmentLoadAction(DepthStencilAttachment.LoadAction);
        DepthAttachment.clearDepth         = DepthStencilAttachment.ClearValue.Depth;
        DepthAttachment.level              = MetalDSV->GetMipLevel();
        DepthAttachment.slice              = MetalDSV->GetArrayIndex();
        DepthAttachment.storeActionOptions = MTLStoreActionOptionNone;
        DepthAttachment.storeAction        = ConvertAttachmentStoreAction(DepthStencilAttachment.StoreAction);
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
    // TODO: ImGui is screwing something up here; once fixed, forward a properly sized rect to ContextState.
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

void FMetalCommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SourceData)
{
}

void FMetalCommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SourceData, uint32 SrcRowPitch)
{
}

void FMetalCommandContext::UpdateTexture3D(FRHITexture* Dst, const FTextureRegion3D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch, uint32 SrcDepthPitch)
{
}

void FMetalCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
}

void FMetalCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    FMetalBufferRHI* MetalDst = GetMetalBuffer(Dst);
    FMetalBufferRHI* MetalSrc = GetMetalBuffer(Src);
    
    CHECK(CommandBuffer != nil);
    CHECK(MetalDst      != nullptr);
    CHECK(MetalSrc      != nullptr);
    
    CopyContext.StartEncoder(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder copyFromBuffer:MetalSrc->GetMTLBuffer()
                   sourceOffset:CopyDesc.SrcOffset
                       toBuffer:MetalDst->GetMTLBuffer()
              destinationOffset:CopyDesc.DstOffset
                           size:CopyDesc.Size];
    
    CopyContext.FinishEncoder();
}

void FMetalCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FMetalTextureRHI* MetalDst = GetMetalTexture(Dst);
    FMetalTextureRHI* MetalSrc = GetMetalTexture(Src);
    
    CHECK(CommandBuffer != nil);
    CHECK(MetalDst      != nullptr);
    CHECK(MetalSrc      != nullptr);
    
    CopyContext.StartEncoder(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder copyFromTexture:MetalSrc->GetMTLTexture() toTexture:MetalDst->GetMTLTexture()];
    
    CopyContext.FinishEncoder();
}

void FMetalCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc) 
{ 
} 
 
void FMetalCommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) 
{ 
} 

void FMetalCommandContext::CopyTextureSubresourceToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion3D& SrcRegion, uint32 SrcMipLevel, uint32 SrcArraySlice)
{
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

void FMetalCommandContext::SetRayTracingBindings(FRHISceneAccelerationStructure* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources)
{
}

void FMetalCommandContext::TransitionTextureState(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
}

void FMetalCommandContext::TransitionBufferState(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
}

void FMetalCommandContext::RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState)
{
}

void FMetalCommandContext::RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState)
{
}

void FMetalCommandContext::UnorderedAccessTextureBarrier(FRHITexture* Texture)
{
}

void FMetalCommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
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
        ComputeEncoder = [CommandBuffer computeCommandEncoder];
        [ComputeEncoder retain];
    }

    ContextState.PrepareComputeState();
    ContextState.BindComputeState();
}

void FMetalCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    const MTLPrimitiveType PrimitiveType = ContextState.GetPrimitiveType();
    CHECK(PrimitiveType != MTLPrimitiveType(-1));
    //[GraphicsEncoder drawPrimitives:PrimitiveType vertexStart:StartVertexLocation vertexCount:VertexCount];
}

void FMetalCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    const FMetalIndexBufferCache& IndexBufferCache = ContextState.GetIndexBufferCache();
    const MTLPrimitiveType        PrimitiveType    = ContextState.GetPrimitiveType();
    CHECK(IndexBufferCache.IndexBuffer != nil);
    CHECK(PrimitiveType                != MTLPrimitiveType(-1));

    /*[GraphicsEncoder drawIndexedPrimitives:PrimitiveType
                                indexCount:IndexCount
                                 indexType:IndexBufferCache.IndexType
                               indexBuffer:IndexBufferCache.IndexBuffer
                         indexBufferOffset:IndexBufferCache.BufferResource->GetStride() * StartIndexLocation
                             instanceCount:1
                                baseVertex:BaseVertexLocation
                              baseInstance:0];*/
}

void FMetalCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    const MTLPrimitiveType PrimitiveType = ContextState.GetPrimitiveType();
    CHECK(PrimitiveType != MTLPrimitiveType(-1));
    /*[GraphicsEncoder drawPrimitives:PrimitiveType
                        vertexStart:StartVertexLocation
                        vertexCount:VertexCountPerInstance
                      instanceCount:InstanceCount
                       baseInstance:StartInstanceLocation];*/
}

void FMetalCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    const FMetalIndexBufferCache& IndexBufferCache = ContextState.GetIndexBufferCache();
    const MTLPrimitiveType        PrimitiveType    = ContextState.GetPrimitiveType();
    CHECK(IndexBufferCache.IndexBuffer != nil);
    CHECK(PrimitiveType                != MTLPrimitiveType(-1));

    /*[GraphicsEncoder drawIndexedPrimitives:PrimitiveType
                                indexCount:IndexCountPerInstance
                                 indexType:IndexBufferCache.IndexType
                               indexBuffer:IndexBufferCache.IndexBuffer
                         indexBufferOffset:IndexBufferCache.BufferResource->GetStride() * StartIndexLocation
                             instanceCount:InstanceCount
                                baseVertex:BaseVertexLocation
                              baseInstance:StartInstanceLocation];*/
}

void FMetalCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
    PrepareForDispatch();
}

void FMetalCommandContext::DispatchRays(FRHISceneAccelerationStructure* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
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

void FMetalCommandContext::ClearState()
{
    ContextState.ResetState();
    Flush();
}

void FMetalCommandContext::Flush()
{
    if (CommandBuffer)
    {
        [CommandBuffer commit];
        [CommandBuffer waitUntilCompleted];
    }
}

void FMetalCommandContext::PushEvent(const StringView& Name)
{
    SCOPED_AUTORELEASE_POOL();
    
    id<MTLCommandEncoder> Encoder = nil;
    if (GraphicsEncoder)
    {
        Encoder = GraphicsEncoder;
    }
    else
    {
        CopyContext.StartEncoder(CommandBuffer);
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
