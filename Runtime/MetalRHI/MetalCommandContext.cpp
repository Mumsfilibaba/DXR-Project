#include "MetalCommandContext.h"
#include "MetalDeviceContext.h"
#include "MetalBuffer.h"
#include "MetalTexture.h"
#include "MetalViewport.h"
#include "MetalPipelineState.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalCommandContext

FMetalCommandContext::FMetalCommandContext(FMetalDeviceContext* InDeviceContext)
    : FMetalObject(InDeviceContext)
    , IRHICommandContext()
    , CommandBuffer(nil)
    , GraphicsEncoder(nil)
{
    ClearState();
}

FMetalCommandContext* FMetalCommandContext::CreateMetalContext(FMetalDeviceContext* InDeviceContext)
{ 
    return dbg_new FMetalCommandContext(InDeviceContext);
}

void FMetalCommandContext::StartContext() 
{
    Check(CommandBuffer == nil);
    
    id<MTLCommandQueue> CommandQueue = GetDeviceContext()->GetMTLCommandQueue();
    CommandBuffer = [CommandQueue commandBuffer];
}

void FMetalCommandContext::FinishContext()
{
    Check(CommandBuffer != nil);
    
    CopyContext.FinishContext();
    
    [CommandBuffer commit];
    [CommandBuffer release];
    
    CommandBuffer = nil;
}

void FMetalCommandContext::BeginTimeStamp(FRHITimestampQuery* Profiler, uint32 Index)
{
}

void FMetalCommandContext::EndTimeStamp(FRHITimestampQuery* Profiler, uint32 Index)
{
}

void FMetalCommandContext::ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    SCOPED_AUTORELEASE_POOL();
    
    FMetalTexture*  RTVTexture = GetMetalTexture(RenderTargetView.Texture);
    FMetalViewport* Viewport   = RTVTexture ? RTVTexture->GetViewport() : nullptr;
    
    MTLRenderPassDescriptor* RenderPassDescriptor = [MTLRenderPassDescriptor new];
    
    MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPassDescriptor.colorAttachments[0];
    ColorAttachment.texture            = Viewport ? Viewport->GetDrawableTexture() : RTVTexture->GetMTLTexture();
    ColorAttachment.loadAction         = ConvertAttachmentLoadAction(RenderTargetView.LoadAction);
    ColorAttachment.clearColor         = MTLClearColorMake(RenderTargetView.ClearValue.R, RenderTargetView.ClearValue.G, RenderTargetView.ClearValue.B, RenderTargetView.ClearValue.A);
    ColorAttachment.level              = RenderTargetView.MipLevel;
    ColorAttachment.slice              = RenderTargetView.ArrayIndex;
    ColorAttachment.storeActionOptions = MTLStoreActionOptionNone;
    ColorAttachment.storeAction        = ConvertAttachmentStoreAction(RenderTargetView.StoreAction);
    
    if(!GraphicsEncoder)
    {
        GraphicsEncoder = [CommandBuffer renderCommandEncoderWithDescriptor:RenderPassDescriptor];
    }
    
    NSRelease(RenderPassDescriptor);
    
    [GraphicsEncoder endEncoding];
    GraphicsEncoder = nil;
}

void FMetalCommandContext::ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
}

void FMetalCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
}

void FMetalCommandContext::BeginRenderPass(const FRHIRenderPassInitializer& RenderPassInitializer)
{
    SCOPED_AUTORELEASE_POOL();
    
    Check(GraphicsEncoder == nil);
    
    CopyContext.FinishContext();

    FMetalTexture* DSVTexture = GetMetalTexture(RenderPassInitializer.DepthStencilView.Texture);
    METAL_ERROR_COND((RenderPassInitializer.NumRenderTargets > 0) || (DSVTexture != nullptr), "A RenderPass needs a valid RenderTargetView or DepthStencilView");
    
    MTLRenderPassDescriptor* RenderPassDescriptor = [MTLRenderPassDescriptor new];
    RenderPassDescriptor.defaultRasterSampleCount = 1;
    RenderPassDescriptor.renderTargetArrayLength  = 1;
    
    for (uint32 Index = 0; Index < RenderPassInitializer.NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetView& RenderTargetView = RenderPassInitializer.RenderTargets[Index];
        
        FMetalTexture*  RTVTexture = GetMetalTexture(RenderTargetView.Texture);
        FMetalViewport* Viewport   = RTVTexture ? RTVTexture->GetViewport() : nullptr;
        
        MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPassDescriptor.colorAttachments[Index];
        ColorAttachment.texture            = Viewport ? Viewport->GetDrawableTexture() : RTVTexture->GetMTLTexture();
        ColorAttachment.loadAction         = ConvertAttachmentLoadAction(RenderTargetView.LoadAction);
        ColorAttachment.level              = RenderTargetView.MipLevel;
        ColorAttachment.slice              = RenderTargetView.ArrayIndex;
        ColorAttachment.storeActionOptions = MTLStoreActionOptionNone;
        ColorAttachment.storeAction        = ConvertAttachmentStoreAction(RenderTargetView.StoreAction);
        ColorAttachment.clearColor         = MTLClearColorMake(
            RenderTargetView.ClearValue.R,
            RenderTargetView.ClearValue.G,
            RenderTargetView.ClearValue.B,
            RenderTargetView.ClearValue.A);
    }

    if (DSVTexture)
    {
        const FRHIDepthStencilView& DepthStencilView = RenderPassInitializer.DepthStencilView;
        
        MTLRenderPassDepthAttachmentDescriptor* DepthAttachment = RenderPassDescriptor.depthAttachment;
        DepthAttachment.texture            = DSVTexture->GetMTLTexture();
        DepthAttachment.loadAction         = ConvertAttachmentLoadAction(DepthStencilView.LoadAction);
        DepthAttachment.clearDepth         = DepthStencilView.ClearValue.Depth;
        DepthAttachment.level              = DepthStencilView.MipLevel;
        DepthAttachment.slice              = DepthStencilView.ArrayIndex;
        DepthAttachment.storeActionOptions = MTLStoreActionOptionNone;
        DepthAttachment.storeAction        = ConvertAttachmentStoreAction(DepthStencilView.StoreAction);
    }
    
    // TODO: Stencil Attachment

    Check(RenderPassDescriptor != nil);
    GraphicsEncoder = [CommandBuffer renderCommandEncoderWithDescriptor:RenderPassDescriptor];
    [GraphicsEncoder retain];
    
    NSRelease(RenderPassDescriptor);
}

void FMetalCommandContext::EndRenderPass()
{
    Check(GraphicsEncoder != nil);
        
    [GraphicsEncoder endEncoding];
    NSRelease(GraphicsEncoder);
}

void FMetalCommandContext::SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
{
    MTLViewport Viewport;
    Viewport.width   = Width;
    Viewport.height  = Height;
    Viewport.originX = x;
    Viewport.originY = y;
    Viewport.znear   = MinDepth;
    Viewport.zfar    = MaxDepth;
    
    CurrentViewport = Viewport;
}

void FMetalCommandContext::SetScissorRect(float Width, float Height, float x, float y)
{
    // TODO: ImGui is screwing something up here
    /*// Ensure that the size is correct;
    Width  = Width - x;
    Height = Height - y;
    
    MTLScissorRect ScissorRect;
    ScissorRect.width  = Width;
    ScissorRect.height = Height;
    ScissorRect.x      = x;
    ScissorRect.y      = y;
    
    [GraphicsEncoder setScissorRect:ScissorRect];*/
}

void FMetalCommandContext::SetBlendFactor(const FVector4& Color)
{
}

void FMetalCommandContext::SetVertexBuffers(const TArrayView<FRHIVertexBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (uint32 BufferIndex = 0; BufferIndex < InVertexBuffers.GetSize(); ++BufferIndex)
    {
        const uint32 Index = BufferSlot + BufferIndex;
        
        FMetalVertexBuffer* Buffer  = static_cast<FMetalVertexBuffer*>(VertexBuffers[Index]);
        CurrentVertexBuffers[Index] = Buffer ? Buffer->GetMTLBuffer() : nil;
        CurrentVertexOffsets[Index] = 0;
    }
    
    CurrentVertexBufferRange = NSMakeRange(
        NMath::Min<uint32>(BufferSlot, CurrentVertexBufferRange.location), 
        NMath::Max<uint32>(InVertexBuffers.GetSize(), CurrentVertexBufferRange.length));
}

void FMetalCommandContext::SetIndexBuffer(FRHIIndexBuffer* IndexBuffer)
{
    CurrentIndexBuffer = MakeSharedRef<FMetalIndexBuffer>(IndexBuffer);
}

void FMetalCommandContext::SetPrimitiveTopology(EPrimitiveTopology PrimitveTopology)
{
    CurrentPrimitiveType = ConvertPrimitiveTopology(PrimitveTopology);
}

void FMetalCommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    CurrentGraphicsPipeline = MakeSharedRef<FMetalGraphicsPipelineState>(PipelineState);
}

void FMetalCommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
}

void FMetalCommandContext::Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
}

void FMetalCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    Check(Shader != nullptr);
    FMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check(ParameterIndex < kMaxSRVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentSRVs[Visibility][ParameterIndex] = MakeSharedRef<FMetalShaderResourceView>(ShaderResourceView);
}

void FMetalCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
{
    Check(Shader              != nullptr);
    Check(ShaderResourceViews != nullptr);
    
    FMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check((ParameterIndex + InShaderResourceViews.GetSize()) < kMaxSRVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (uint32 Index = 0; Index < InShaderResourceViews.GetSize(); ++Index)
    {
        CurrentSRVs[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalShaderResourceView>(ShaderResourceViews[Index]);
    }
}

void FMetalCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    Check(Shader != nullptr);
    FMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check(ParameterIndex < kMaxUAVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentUAVs[Visibility][ParameterIndex] = MakeSharedRef<FMetalUnorderedAccessView>(UnorderedAccessView);
}

void FMetalCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
{
    Check(Shader               != nullptr);
    Check(UnorderedAccessViews != nullptr);
    
    FMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check((ParameterIndex + InUnorderedAccessViews.GetSize()) < kMaxUAVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (uint32 Index = 0; Index < InUnorderedAccessViews.GetSize(); ++Index)
    {
        CurrentUAVs[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalUnorderedAccessView>(UnorderedAccessViews[Index]);
    }
}

void FMetalCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    Check(Shader != nullptr);
    FMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check(ParameterIndex < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentConstantBuffers[Visibility][ParameterIndex] = MakeSharedRef<FMetalConstantBuffer>(ConstantBuffer);
}

void FMetalCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIConstantBuffer* const> InConstantBuffers, uint32 ParameterIndex)
{
    Check(Shader          != nullptr);
    Check(ConstantBuffers != nullptr);
    
    FMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check((ParameterIndex + InConstantBuffers.GetSize()) < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (uint32 Index = 0; Index < InConstantBuffers.GetSize(); ++Index)
    {
        CurrentConstantBuffers[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalConstantBuffer>(ConstantBuffers[Index]);
    }
}

void FMetalCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    Check(Shader != nullptr);
    FMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check(ParameterIndex < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentSamplerStates[Visibility][ParameterIndex] = MakeSharedRef<FMetalSamplerState>(SamplerState);
}

void FMetalCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
{
    Check(Shader        != nullptr);
    Check(SamplerStates != nullptr);
    
    FMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check((ParameterIndex + InSamplerStates.GetSize()) < kMaxSamplerStates);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (uint32 Index = 0; Index < InSamplerStates.GetSize(); ++Index)
    {
        CurrentSamplerStates[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalSamplerState>(SamplerStates[Index]);
    }
}

void FMetalCommandContext::UpdateBuffer(FRHIBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
}

void FMetalCommandContext::UpdateTexture2D(FRHITexture2D* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
{
}

void FMetalCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
}

void FMetalCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHICopyBufferInfo& CopyInfo)
{
    FMetalBuffer* MetalDst = GetMetalBuffer(Dst);
    FMetalBuffer* MetalSrc = GetMetalBuffer(Src);
    
    Check(CommandBuffer != nil);
    Check(MetalDst      != nullptr);
    Check(MetalSrc      != nullptr);
    
    CopyContext.StartContext(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder copyFromBuffer:MetalSrc->GetMTLBuffer()
                   sourceOffset:CopyInfo.SourceOffset
                       toBuffer:MetalDst->GetMTLBuffer()
              destinationOffset:CopyInfo.DestinationOffset
                           size:CopyInfo.SizeInBytes];
    
    CopyContext.FinishContext();
}

void FMetalCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FMetalTexture* MetalDst = GetMetalTexture(Dst);
    FMetalTexture* MetalSrc = GetMetalTexture(Src);
    
    Check(CommandBuffer != nil);
    Check(MetalDst      != nullptr);
    Check(MetalSrc      != nullptr);
    
    CopyContext.StartContext(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder copyFromTexture:MetalSrc->GetMTLTexture() toTexture:MetalDst->GetMTLTexture()];
    
    CopyContext.FinishContext();
}

void FMetalCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHICopyTextureInfo& CopyTextureInfo)
{
}

void FMetalCommandContext::DestroyResource(class IRHIResource* Resource)
{
}

void FMetalCommandContext::DiscardContents(class FRHITexture* Texture)
{
}

void FMetalCommandContext::BuildRayTracingGeometry(FRHIRayTracingGeometry* Geometry, FRHIVertexBuffer* VertexBuffer, FRHIIndexBuffer* IndexBuffer, bool bUpdate)
{
}

void FMetalCommandContext::BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
{
}

void FMetalCommandContext::SetRayTracingBindings(
    FRHIRayTracingScene* RayTracingScene,
    FRHIRayTracingPipelineState* PipelineState,
    const FRayTracingShaderResources* GlobalResource,
    const FRayTracingShaderResources* RayGenLocalResources,
    const FRayTracingShaderResources* MissLocalResources,
    const FRayTracingShaderResources* HitGroupResources,
    uint32 NumHitGroupResources)
{
}

void FMetalCommandContext::GenerateMips(FRHITexture* Texture)
{
    FMetalTexture* MetalTexture = GetMetalTexture(Texture);
    Check(MetalTexture != nullptr);
    
    // Cannot call generatemips inside of a RenderPass
    Check(GraphicsEncoder == nil);
    
    CopyContext.StartContext(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder generateMipmapsForTexture:MetalTexture->GetMTLTexture()];
}

void FMetalCommandContext::TransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
{
}

void FMetalCommandContext::TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
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
    Check(GraphicsEncoder != nil);
    
    [GraphicsEncoder setViewport:CurrentViewport];
    
    // Necessary to retrieve all states and the resource bindings
    if (CurrentGraphicsPipeline)
    {
        [GraphicsEncoder setVertexBuffers:CurrentVertexBuffers.GetData()
                                  offsets:CurrentVertexOffsets.GetData()
                                withRange:CurrentVertexBufferRange];

        FMetalDepthStencilState* DepthStencilState = CurrentGraphicsPipeline->GetMetalDepthStencilState();
        Check(DepthStencilState != nullptr);
        
        [GraphicsEncoder setDepthStencilState:DepthStencilState->GetMTLDepthStencilState()];
        
        FMetalRasterizerState* RasterizerState = CurrentGraphicsPipeline->GetMetalRasterizerState();
        Check(RasterizerState != nullptr);
        
        [GraphicsEncoder setFrontFacingWinding:RasterizerState->GetMTLFrontFaceWinding()];
        [GraphicsEncoder setTriangleFillMode:RasterizerState->GetMTLFillMode()];
        
        [GraphicsEncoder setRenderPipelineState:CurrentGraphicsPipeline->GetMTLPipelineState()];
        
        // Vertex-Shader stage
        // [GraphicsEncoder set]

    }
}

void FMetalCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    Check(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    Check(CurrentPrimitiveType != MTLPrimitiveType(-1));
    [GraphicsEncoder drawPrimitives:CurrentPrimitiveType vertexStart:StartVertexLocation vertexCount:VertexCount];
}

void FMetalCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    Check(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    Check(CurrentIndexBuffer   != nullptr);
    Check(CurrentPrimitiveType != MTLPrimitiveType(-1));
    
    [GraphicsEncoder drawIndexedPrimitives:CurrentPrimitiveType
                                indexCount:IndexCount
                                 indexType:(CurrentIndexBuffer->GetFormat() == EIndexFormat::uint32) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16
                               indexBuffer:CurrentIndexBuffer->GetMTLBuffer()
                         indexBufferOffset:CurrentIndexBuffer->GetStride() * StartIndexLocation
                             instanceCount:1
                                baseVertex:BaseVertexLocation
                              baseInstance:0];
}

void FMetalCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    Check(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    Check(CurrentPrimitiveType != MTLPrimitiveType(-1));
    [GraphicsEncoder drawPrimitives:CurrentPrimitiveType
                        vertexStart:StartVertexLocation
                        vertexCount:VertexCountPerInstance
                      instanceCount:InstanceCount
                       baseInstance:StartInstanceLocation];
}

void FMetalCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    Check(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    Check(CurrentIndexBuffer   != nullptr);
    Check(CurrentPrimitiveType != MTLPrimitiveType(-1));
    
    [GraphicsEncoder drawIndexedPrimitives:CurrentPrimitiveType
                                indexCount:IndexCountPerInstance
                                 indexType:(CurrentIndexBuffer->GetFormat() == EIndexFormat::uint32) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16
                               indexBuffer:CurrentIndexBuffer->GetMTLBuffer()
                         indexBufferOffset:CurrentIndexBuffer->GetStride() * StartIndexLocation
                             instanceCount:InstanceCount
                                baseVertex:BaseVertexLocation
                              baseInstance:StartInstanceLocation];
}

void FMetalCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
}

void FMetalCommandContext::DispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
}

void FMetalCommandContext::ClearState()
{
    CMemory::Memzero(&CurrentViewport);
    
    CurrentIndexBuffer      = nullptr;
    CurrentGraphicsPipeline = nullptr;
    
    CurrentVertexBuffers.Fill(nil);
    CurrentVertexOffsets.Memzero();
    CurrentVertexBufferRange = NSMakeRange(0, 0);
    
    CurrentPrimitiveType = MTLPrimitiveType(-1);
    
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

void FMetalCommandContext::InsertMarker(const FString& Message)
{
    SCOPED_AUTORELEASE_POOL();
    
    id<MTLCommandEncoder> Encoder = nil;
    if (GraphicsEncoder)
    {
        Encoder = GraphicsEncoder;
    }
    else
    {
        CopyContext.StartContext(CommandBuffer);
        Encoder = CopyContext.GetMTLCopyEncoder();
    }

    [Encoder insertDebugSignpost:Message.GetNSString()];
}
