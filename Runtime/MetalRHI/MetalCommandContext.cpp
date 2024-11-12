#include "MetalCommandContext.h"
#include "MetalDeviceContext.h"
#include "MetalBuffer.h"
#include "MetalTexture.h"
#include "MetalViewport.h"
#include "MetalPipelineState.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FMetalCommandContext::FMetalCommandContext(FMetalDeviceContext* InDeviceContext)
    : FMetalDeviceChild(InDeviceContext)
    , IRHICommandContext()
    , CommandBuffer(nil)
    , GraphicsEncoder(nil)
{
    RHIClearState();
}

FMetalCommandContext* FMetalCommandContext::CreateMetalContext(FMetalDeviceContext* InDeviceContext)
{ 
    return new FMetalCommandContext(InDeviceContext);
}

void FMetalCommandContext::RHIStartContext() 
{
    CHECK(CommandBuffer == nil);
    
    id<MTLCommandQueue> CommandQueue = GetDeviceContext()->GetMTLCommandQueue();
    CommandBuffer = [CommandQueue commandBuffer];
}

void FMetalCommandContext::RHIFinishContext()
{
    CHECK(CommandBuffer != nil);
    
    CopyContext.FinishEncoder();
    
    [CommandBuffer commit];
    [CommandBuffer release];
    
    CommandBuffer = nil;
}

void FMetalCommandContext::RHIQueryTimestamp(FRHIQuery* Query)
{
}

void FMetalCommandContext::RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    SCOPED_AUTORELEASE_POOL();
    
    FMetalTexture* RTVTexture = GetMetalTexture(RenderTargetView.Texture);
    
    MTLRenderPassDescriptor* RenderPassDescriptor = [MTLRenderPassDescriptor new];
    MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPassDescriptor.colorAttachments[0];

    ColorAttachment.texture            = RTVTexture->GetMTLTexture();
    ColorAttachment.loadAction         = ConvertAttachmentLoadAction(RenderTargetView.LoadAction);
    ColorAttachment.clearColor         = MTLClearColorMake(RenderTargetView.ClearValue.r, RenderTargetView.ClearValue.g, RenderTargetView.ClearValue.b, RenderTargetView.ClearValue.a);
    ColorAttachment.level              = RenderTargetView.MipLevel;
    ColorAttachment.slice              = RenderTargetView.ArrayIndex;
    ColorAttachment.storeActionOptions = MTLStoreActionOptionNone;
    ColorAttachment.storeAction        = ConvertAttachmentStoreAction(RenderTargetView.StoreAction);
    
    if(!GraphicsEncoder)
    {
        GraphicsEncoder = [CommandBuffer renderCommandEncoderWithDescriptor:RenderPassDescriptor];
    }
    
    [RenderPassDescriptor release];
    
    [GraphicsEncoder endEncoding];
    GraphicsEncoder = nil;
}

void FMetalCommandContext::RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
}

void FMetalCommandContext::RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
}

void FMetalCommandContext::RHIBeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo)
{
    SCOPED_AUTORELEASE_POOL();
    
    CHECK(GraphicsEncoder == nil);
    
    CopyContext.FinishEncoder();

    FMetalTexture* DSVTexture = GetMetalTexture(BeginRenderPassInfo.DepthStencilView.Texture);
    METAL_ERROR_COND((BeginRenderPassInfo.NumRenderTargets > 0) || (DSVTexture != nullptr), "A RenderPass needs a valid RenderTargetView or DepthStencilView");
    
    MTLRenderPassDescriptor* RenderPassDescriptor = [MTLRenderPassDescriptor new];
    RenderPassDescriptor.defaultRasterSampleCount = 1;
    RenderPassDescriptor.renderTargetArrayLength  = 1;
    
    for (uint32 Index = 0; Index < BeginRenderPassInfo.NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetView& RenderTargetView = BeginRenderPassInfo.RenderTargets[Index];
        
        FMetalTexture* RTVTexture = GetMetalTexture(RenderTargetView.Texture);
        METAL_ERROR_COND(RTVTexture != nullptr, "Texture cannot be nullptr");
        
        MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPassDescriptor.colorAttachments[Index];
        ColorAttachment.texture            = RTVTexture->GetMTLTexture();
        ColorAttachment.loadAction         = ConvertAttachmentLoadAction(RenderTargetView.LoadAction);
        ColorAttachment.level              = RenderTargetView.MipLevel;
        ColorAttachment.slice              = RenderTargetView.ArrayIndex;
        ColorAttachment.storeActionOptions = MTLStoreActionOptionNone;
        ColorAttachment.storeAction        = ConvertAttachmentStoreAction(RenderTargetView.StoreAction);
        ColorAttachment.clearColor         = MTLClearColorMake(1.0f, RenderTargetView.ClearValue.g, RenderTargetView.ClearValue.g, RenderTargetView.ClearValue.a);
    }

    if (DSVTexture)
    {
        const FRHIDepthStencilView& DepthStencilView = BeginRenderPassInfo.DepthStencilView;
        
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

    CHECK(RenderPassDescriptor != nil);
    GraphicsEncoder = [CommandBuffer renderCommandEncoderWithDescriptor:RenderPassDescriptor];
    [GraphicsEncoder retain];
    
    [RenderPassDescriptor release];
}

void FMetalCommandContext::RHIEndRenderPass()
{
    CHECK(GraphicsEncoder != nil);
        
    [GraphicsEncoder endEncoding];
    [GraphicsEncoder release];
}

void FMetalCommandContext::RHISetViewport(const FViewportRegion& ViewportRegion)
{
    MTLViewport Viewport;
    Viewport.width   = ViewportRegion.Width;
    Viewport.height  = ViewportRegion.Height;
    Viewport.originX = ViewportRegion.PositionX;
    Viewport.originY = ViewportRegion.PositionY;
    Viewport.znear   = ViewportRegion.MinDepth;
    Viewport.zfar    = ViewportRegion.MaxDepth;
    
    CurrentViewport = Viewport;
}

void FMetalCommandContext::RHISetScissorRect(const FScissorRegion& ScissorRegion)
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

void FMetalCommandContext::RHISetBlendFactor(const FVector4& Color)
{
}

void FMetalCommandContext::RHISetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 BufferIndex = 0; BufferIndex < InVertexBuffers.Size(); ++BufferIndex)
    {
        const uint32 Index = BufferSlot + BufferIndex;
        
        FMetalBuffer* Buffer  = static_cast<FMetalBuffer*>(InVertexBuffers[Index]);
        CurrentVertexBuffers[Index] = Buffer ? Buffer->GetMTLBuffer() : nil;
        CurrentVertexOffsets[Index] = 0;
    }
    
    CurrentVertexBufferRange = NSMakeRange(FMath::Min<uint64>(BufferSlot, CurrentVertexBufferRange.location), FMath::Max<uint64>(InVertexBuffers.Size(), CurrentVertexBufferRange.length));
}

void FMetalCommandContext::RHISetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    CurrentIndexBuffer = MakeSharedRef<FMetalBuffer>(IndexBuffer);
}

void FMetalCommandContext::RHISetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    CurrentGraphicsPipeline = MakeSharedRef<FMetalGraphicsPipelineState>(PipelineState);
}

void FMetalCommandContext::RHISetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
}

void FMetalCommandContext::RHISet32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
}

void FMetalCommandContext::RHISetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxSRVs);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    CurrentSRVs[Visibility][ParameterIndex] = MakeSharedRef<FMetalShaderResourceView>(ShaderResourceView);
}

void FMetalCommandContext::RHISetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK((ParameterIndex + InShaderResourceViews.Size()) < kMaxSRVs);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        CurrentSRVs[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalShaderResourceView>(InShaderResourceViews[Index]);
    }
}

void FMetalCommandContext::RHISetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxUAVs);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    CurrentUAVs[Visibility][ParameterIndex] = MakeSharedRef<FMetalUnorderedAccessView>(UnorderedAccessView);
}

void FMetalCommandContext::RHISetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK((ParameterIndex + InUnorderedAccessViews.Size()) < kMaxUAVs);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        CurrentUAVs[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalUnorderedAccessView>(InUnorderedAccessViews[Index]);
    }
}

void FMetalCommandContext::RHISetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    CurrentConstantBuffers[Visibility][ParameterIndex] = MakeSharedRef<FMetalBuffer>(ConstantBuffer);
}

void FMetalCommandContext::RHISetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK((ParameterIndex + InConstantBuffers.Size()) < kMaxConstantBuffers);
        
    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        CurrentConstantBuffers[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalBuffer>(InConstantBuffers[Index]);
    }
}

void FMetalCommandContext::RHISetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    CurrentSamplerStates[Visibility][ParameterIndex] = MakeSharedRef<FMetalSamplerState>(SamplerState);
}

void FMetalCommandContext::RHISetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK((ParameterIndex + InSamplerStates.Size()) < kMaxSamplerStates);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        CurrentSamplerStates[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalSamplerState>(InSamplerStates[Index]);
    }

    /*CurrentSamplerStates[Visibility] = NSMakeRange(
        FMath::Min<uint32>(ParameterIndex, CurrentSamplerStateRange[Visibility].location),
        FMath::Max<uint32>(InSamplerStates.Size(), CurrentSamplerStateRange[Visibility].length));*/
}

void FMetalCommandContext::RHIUpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SourceData)
{
}

void FMetalCommandContext::RHIUpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SourceData, uint32 SrcRowPitch)
{
}

void FMetalCommandContext::RHIResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
}

void FMetalCommandContext::RHICopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc)
{
    FMetalBuffer* MetalDst = GetMetalBuffer(Dst);
    FMetalBuffer* MetalSrc = GetMetalBuffer(Src);
    
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

void FMetalCommandContext::RHICopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FMetalTexture* MetalDst = GetMetalTexture(Dst);
    FMetalTexture* MetalSrc = GetMetalTexture(Src);
    
    CHECK(CommandBuffer != nil);
    CHECK(MetalDst      != nullptr);
    CHECK(MetalSrc      != nullptr);
    
    CopyContext.StartEncoder(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder copyFromTexture:MetalSrc->GetMTLTexture() toTexture:MetalDst->GetMTLTexture()];
    
    CopyContext.FinishEncoder();
}

void FMetalCommandContext::RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc)
{
}

void FMetalCommandContext::RHIDiscardContents(class FRHITexture* Texture)
{
}

void FMetalCommandContext::RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo)
{
}

void FMetalCommandContext::RHIBuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo)
{
}

void FMetalCommandContext::RHISetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources)
{
}

void FMetalCommandContext::RHIGenerateMips(FRHITexture* Texture)
{
    FMetalTexture* MetalTexture = GetMetalTexture(Texture);
    CHECK(MetalTexture != nullptr);
    
    // Cannot call generatemips inside of a RenderPass
    CHECK(GraphicsEncoder == nil);
    
    CopyContext.StartEncoder(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder generateMipmapsForTexture:MetalTexture->GetMTLTexture()];
}

void FMetalCommandContext::RHITransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
{
}

void FMetalCommandContext::RHITransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
}

void FMetalCommandContext::RHIUnorderedAccessTextureBarrier(FRHITexture* Texture)
{
}

void FMetalCommandContext::RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
}

void FMetalCommandContext::PrepareForDraw()
{
    CHECK(GraphicsEncoder != nil);
    
    [GraphicsEncoder setViewport:CurrentViewport];
    
    // Necessary to retrieve all states and the resource bindings
    if (CurrentGraphicsPipeline)
    {
        [GraphicsEncoder setVertexBuffers:CurrentVertexBuffers.Data()
                                  offsets:CurrentVertexOffsets.Data()
                                withRange:CurrentVertexBufferRange];

        FMetalDepthStencilState* DepthStencilState = CurrentGraphicsPipeline->GetMetalDepthStencilState();
        CHECK(DepthStencilState != nullptr);
        
        [GraphicsEncoder setDepthStencilState:DepthStencilState->GetMTLDepthStencilState()];
        
        FMetalRasterizerState* RasterizerState = CurrentGraphicsPipeline->GetMetalRasterizerState();
        CHECK(RasterizerState != nullptr);
        
        [GraphicsEncoder setFrontFacingWinding:RasterizerState->FrontFaceWinding];
        [GraphicsEncoder setTriangleFillMode:RasterizerState->FillMode];
        
        // [GraphicsEncoder setRenderPipelineState:CurrentGraphicsPipeline->GetMTLPipelineState()];
        
        // Vertex-Buffers stage
        [GraphicsEncoder setVertexBuffers:CurrentVertexBuffers.Data()
                                  offsets:CurrentVertexOffsets.Data()
                                withRange:CurrentVertexBufferRange];
        
        /*
        // Set resources for each shaderstage
        for (EShaderVisibility ShaderStage = ShaderVisibility_Compute; ShaderStage < ShaderVisibility_Count; ShaderStage = EShaderVisibility(ShaderStage + 1))
        {
            id<MTLSamplerState>* SamplerStates = CurrentSamplerStates[ShaderVisibility_Vertex].Data();
            [GraphicsEncoder setVertexSamplerStates:SamplerStates withRange:CurrentSamplerStateRange[ShaderVisibility_Vertex]];

            const uint32 NumConstantBuffer = CurrentGraphicsPipeline->GetNumBuffers(ShaderStage);
            for (uint32 Index = 0; Index < kMaxConstantBuffers; ++Index)
            {
                //const uint8 BindingIndex = CurrentGraphicsPipeline->GetConstantBufferBinding(Index);
                //CurrentBuffers[ShaderStage][BindingIndex] = CurrentConstantBuffers[ShaderStage] ? CurrentConstantBuffers[ShaderStage]->GetMTLBuffer() : nil;
            }

            [GraphicsEncoder setVertexBuffers:CurrentBuffers[ShaderStage].Data()
                                      offsets:nil
                                    withRange:NSMakeRange(0, 0)];
        }
         */
    }
}

void FMetalCommandContext::RHIDraw(uint32 VertexCount, uint32 StartVertexLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    CHECK(CurrentPrimitiveType != MTLPrimitiveType(-1));
    //[GraphicsEncoder drawPrimitives:CurrentPrimitiveType vertexStart:StartVertexLocation vertexCount:VertexCount];
}

void FMetalCommandContext::RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    CHECK(CurrentIndexBuffer   != nullptr);
    CHECK(CurrentPrimitiveType != MTLPrimitiveType(-1));
    
    /*[GraphicsEncoder drawIndexedPrimitives:CurrentPrimitiveType
                                indexCount:IndexCount
                                 indexType:(CurrentIndexBuffer->GetFormat() == EIndexFormat::uint32) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16
                               indexBuffer:CurrentIndexBuffer->GetMTLBuffer()
                         indexBufferOffset:CurrentIndexBuffer->GetStride() * StartIndexLocation
                             instanceCount:1
                                baseVertex:BaseVertexLocation
                              baseInstance:0];*/
}

void FMetalCommandContext::RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    CHECK(CurrentPrimitiveType != MTLPrimitiveType(-1));
    /*[GraphicsEncoder drawPrimitives:CurrentPrimitiveType
                        vertexStart:StartVertexLocation
                        vertexCount:VertexCountPerInstance
                      instanceCount:InstanceCount
                       baseInstance:StartInstanceLocation];*/
}

void FMetalCommandContext::RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    CHECK(CurrentIndexBuffer   != nullptr);
    CHECK(CurrentPrimitiveType != MTLPrimitiveType(-1));
    
    /*[GraphicsEncoder drawIndexedPrimitives:CurrentPrimitiveType
                                indexCount:IndexCountPerInstance
                                 indexType:(CurrentIndexBuffer->GetFormat() == EIndexFormat::uint32) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16
                               indexBuffer:CurrentIndexBuffer->GetMTLBuffer()
                         indexBufferOffset:CurrentIndexBuffer->GetStride() * StartIndexLocation
                             instanceCount:InstanceCount
                                baseVertex:BaseVertexLocation
                              baseInstance:StartInstanceLocation];*/
}

void FMetalCommandContext::RHIDispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
}

void FMetalCommandContext::RHIDispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
}

void FMetalCommandContext::RHIPresentViewport(FRHIViewport* Viewport, bool bVerticalSync)
{
    FMetalViewport* MetalViewport = static_cast<FMetalViewport*>(Viewport);
    MetalViewport->Present(bVerticalSync);
}

void FMetalCommandContext::RHIResizeViewport(FRHIViewport* Viewport, uint32 Width, uint32 Height)
{
    FMetalViewport* MetalViewport = static_cast<FMetalViewport*>(Viewport);
    MetalViewport->Resize(Width, Height);
}

void FMetalCommandContext::RHIClearState()
{
    FMemory::Memzero(&CurrentViewport);
    
    CurrentIndexBuffer      = nullptr;
    CurrentGraphicsPipeline = nullptr;
    
    CurrentVertexBuffers.Fill(nil);
    CurrentVertexOffsets.Memzero();
    CurrentVertexBufferRange = NSMakeRange(0, 0);

    CurrentIndexBuffer = nullptr;
    
    // Pipeline
    CurrentPrimitiveType    = MTLPrimitiveType(-1);
    CurrentGraphicsPipeline = nullptr;
    
    // Resources
    for (uint32 ShaderStage = 0; ShaderStage < ShaderVisibility_Count; ++ShaderStage)
    {
        CurrentSamplerStates[ShaderStage].Fill(nil);
        // CurrentSamplerStateRange[ShaderStage] = NSMakeRange(0, 0);
        
        CurrentSRVs[ShaderStage].Fill(nullptr);
        CurrentUAVs[ShaderStage].Fill(nullptr);
        CurrentConstantBuffers[ShaderStage].Fill(nullptr);
        
        CurrentBuffers[ShaderStage].Fill(nil);
        CurrentTextures[ShaderStage].Fill(nil);
    }
    
    RHIFlush();
}

void FMetalCommandContext::RHIFlush()
{
    if (CommandBuffer)
    {
        [CommandBuffer commit];
        [CommandBuffer waitUntilCompleted];
    }
}

void FMetalCommandContext::RHIInsertMarker(const FStringView& Message)
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

    [Encoder insertDebugSignpost:FString(Message).GetNSString()];
}

void FMetalCommandContext::RHIBeginExternalCapture()
{
    // Empty for now
}

void FMetalCommandContext::RHIEndExternalCapture()
{
    // Empty for now
}
