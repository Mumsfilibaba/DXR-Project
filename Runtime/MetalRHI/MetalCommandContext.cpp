#include "MetalCommandContext.h"
#include "MetalDeviceContext.h"
#include "MetalBuffer.h"
#include "MetalTexture.h"
#include "MetalViewport.h"
#include "MetalPipelineState.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

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
    return new FMetalCommandContext(InDeviceContext);
}

void FMetalCommandContext::StartContext() 
{
    CHECK(CommandBuffer == nil);
    
    id<MTLCommandQueue> CommandQueue = GetDeviceContext()->GetMTLCommandQueue();
    CommandBuffer = [CommandQueue commandBuffer];
}

void FMetalCommandContext::FinishContext()
{
    CHECK(CommandBuffer != nil);
    
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
    
    FMetalTexture* RTVTexture = GetMetalTexture(RenderTargetView.Texture);
    
    MTLRenderPassDescriptor* RenderPassDescriptor = [MTLRenderPassDescriptor new];
    MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPassDescriptor.colorAttachments[0];

    ColorAttachment.texture            = RTVTexture->GetMTLTexture();
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

void FMetalCommandContext::BeginRenderPass(const FRHIRenderPassDesc& RenderPassDesc)
{
    SCOPED_AUTORELEASE_POOL();
    
    CHECK(GraphicsEncoder == nil);
    
    CopyContext.FinishContext();

    FMetalTexture* DSVTexture = GetMetalTexture(RenderPassDesc.DepthStencilView.Texture);
    METAL_ERROR_COND((RenderPassDesc.NumRenderTargets > 0) || (DSVTexture != nullptr), "A RenderPass needs a valid RenderTargetView or DepthStencilView");
    
    MTLRenderPassDescriptor* RenderPassDescriptor = [MTLRenderPassDescriptor new];
    RenderPassDescriptor.defaultRasterSampleCount = 1;
    RenderPassDescriptor.renderTargetArrayLength  = 1;
    
    for (uint32 Index = 0; Index < RenderPassDesc.NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetView& RenderTargetView = RenderPassDesc.RenderTargets[Index];
        
        FMetalTexture* RTVTexture = GetMetalTexture(RenderTargetView.Texture);
        METAL_ERROR_COND(RTVTexture != nullptr, "Texture cannot be nullptr");
        
        MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPassDescriptor.colorAttachments[Index];
        ColorAttachment.texture            = RTVTexture->GetMTLTexture();
        ColorAttachment.loadAction         = ConvertAttachmentLoadAction(RenderTargetView.LoadAction);
        ColorAttachment.level              = RenderTargetView.MipLevel;
        ColorAttachment.slice              = RenderTargetView.ArrayIndex;
        ColorAttachment.storeActionOptions = MTLStoreActionOptionNone;
        ColorAttachment.storeAction        = ConvertAttachmentStoreAction(RenderTargetView.StoreAction);
        ColorAttachment.clearColor         = MTLClearColorMake(1.0f, RenderTargetView.ClearValue.G, RenderTargetView.ClearValue.B, RenderTargetView.ClearValue.A);
    }

    if (DSVTexture)
    {
        const FRHIDepthStencilView& DepthStencilView = RenderPassDesc.DepthStencilView;
        
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
    
    NSRelease(RenderPassDescriptor);
}

void FMetalCommandContext::EndRenderPass()
{
    CHECK(GraphicsEncoder != nil);
        
    [GraphicsEncoder endEncoding];
    NSRelease(GraphicsEncoder);
}

void FMetalCommandContext::SetViewport(const FRHIViewportRegion& ViewportRegion)
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

void FMetalCommandContext::SetScissorRect(const FRHIScissorRegion& ScissorRegion)
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

void FMetalCommandContext::SetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
    for (int32 BufferIndex = 0; BufferIndex < InVertexBuffers.Size(); ++BufferIndex)
    {
        const uint32 Index = BufferSlot + BufferIndex;
        
        FMetalBuffer* Buffer  = static_cast<FMetalBuffer*>(InVertexBuffers[Index]);
        CurrentVertexBuffers[Index] = Buffer ? Buffer->GetMTLBuffer() : nil;
        CurrentVertexOffsets[Index] = 0;
    }
    
    CurrentVertexBufferRange = NSMakeRange(
        FMath::Min<uint32>(BufferSlot, CurrentVertexBufferRange.location), 
        FMath::Max<uint32>(InVertexBuffers.Size(), CurrentVertexBufferRange.length));
}

void FMetalCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    CurrentIndexBuffer = MakeSharedRef<FMetalBuffer>(IndexBuffer);
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
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxSRVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentSRVs[Visibility][ParameterIndex] = MakeSharedRef<FMetalShaderResourceView>(ShaderResourceView);
}

void FMetalCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK((ParameterIndex + InShaderResourceViews.Size()) < kMaxSRVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (int32 Index = 0; Index < InShaderResourceViews.Size(); ++Index)
    {
        CurrentSRVs[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalShaderResourceView>(InShaderResourceViews[Index]);
    }
}

void FMetalCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxUAVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentUAVs[Visibility][ParameterIndex] = MakeSharedRef<FMetalUnorderedAccessView>(UnorderedAccessView);
}

void FMetalCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK((ParameterIndex + InUnorderedAccessViews.Size()) < kMaxUAVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (int32 Index = 0; Index < InUnorderedAccessViews.Size(); ++Index)
    {
        CurrentUAVs[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalUnorderedAccessView>(InUnorderedAccessViews[Index]);
    }
}

void FMetalCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentConstantBuffers[Visibility][ParameterIndex] = MakeSharedRef<FMetalBuffer>(ConstantBuffer);
}

void FMetalCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK((ParameterIndex + InConstantBuffers.Size()) < kMaxConstantBuffers);
        
    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (int32 Index = 0; Index < InConstantBuffers.Size(); ++Index)
    {
        CurrentConstantBuffers[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalBuffer>(InConstantBuffers[Index]);
    }
}

void FMetalCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentSamplerStates[Visibility][ParameterIndex] = MakeSharedRef<FMetalSamplerState>(SamplerState);
}

void FMetalCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK((ParameterIndex + InSamplerStates.Size()) < kMaxSamplerStates);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (int32 Index = 0; Index < InSamplerStates.Size(); ++Index)
    {
        CurrentSamplerStates[Visibility][ParameterIndex + Index] = MakeSharedRef<FMetalSamplerState>(InSamplerStates[Index]);
    }

    /*CurrentSamplerStates[Visibility] = NSMakeRange(
        FMath::Min<uint32>(ParameterIndex, CurrentSamplerStateRange[Visibility].location),
        FMath::Max<uint32>(InSamplerStates.Size(), CurrentSamplerStateRange[Visibility].length));*/
}

void FMetalCommandContext::UpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SourceData)
{
}

void FMetalCommandContext::UpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SourceData, uint32 SrcRowPitch)
{
}

void FMetalCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
}

void FMetalCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
    FMetalBuffer* MetalDst = GetMetalBuffer(Dst);
    FMetalBuffer* MetalSrc = GetMetalBuffer(Src);
    
    CHECK(CommandBuffer != nil);
    CHECK(MetalDst      != nullptr);
    CHECK(MetalSrc      != nullptr);
    
    CopyContext.StartContext(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder copyFromBuffer:MetalSrc->GetMTLBuffer()
                   sourceOffset:CopyDesc.SrcOffset
                       toBuffer:MetalDst->GetMTLBuffer()
              destinationOffset:CopyDesc.DstOffset
                           size:CopyDesc.Size];
    
    CopyContext.FinishContext();
}

void FMetalCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    FMetalTexture* MetalDst = GetMetalTexture(Dst);
    FMetalTexture* MetalSrc = GetMetalTexture(Src);
    
    CHECK(CommandBuffer != nil);
    CHECK(MetalDst      != nullptr);
    CHECK(MetalSrc      != nullptr);
    
    CopyContext.StartContext(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder copyFromTexture:MetalSrc->GetMTLTexture() toTexture:MetalDst->GetMTLTexture()];
    
    CopyContext.FinishContext();
}

void FMetalCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc)
{
}

void FMetalCommandContext::DestroyResource(class IRefCounted* Resource)
{
}

void FMetalCommandContext::DiscardContents(class FRHITexture* Texture)
{
}

void FMetalCommandContext::BuildRayTracingGeometry(
    FRHIRayTracingGeometry* RayTracingGeometry,
    FRHIBuffer*             VertexBuffer,
    uint32                  NumVertices,
    FRHIBuffer*             IndexBuffer,
    uint32                  NumIndices,
    EIndexFormat            IndexFormat,
    bool                    bUpdate)
{
}

void FMetalCommandContext::BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
{
}

void FMetalCommandContext::SetRayTracingBindings(
    FRHIRayTracingScene*              RayTracingScene,
    FRHIRayTracingPipelineState*      PipelineState,
    const FRayTracingShaderResources* GlobalResource,
    const FRayTracingShaderResources* RayGenLocalResources,
    const FRayTracingShaderResources* MissLocalResources,
    const FRayTracingShaderResources* HitGroupResources,
    uint32                            NumHitGroupResources)
{
}

void FMetalCommandContext::GenerateMips(FRHITexture* Texture)
{
    FMetalTexture* MetalTexture = GetMetalTexture(Texture);
    CHECK(MetalTexture != nullptr);
    
    // Cannot call generatemips inside of a RenderPass
    CHECK(GraphicsEncoder == nil);
    
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
        
        [GraphicsEncoder setFrontFacingWinding:RasterizerState->GetMTLFrontFaceWinding()];
        [GraphicsEncoder setTriangleFillMode:RasterizerState->GetMTLFillMode()];
        
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

void FMetalCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    CHECK(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    CHECK(CurrentPrimitiveType != MTLPrimitiveType(-1));
    //[GraphicsEncoder drawPrimitives:CurrentPrimitiveType vertexStart:StartVertexLocation vertexCount:VertexCount];
}

void FMetalCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
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

void FMetalCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
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

void FMetalCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
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

void FMetalCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
}

void FMetalCommandContext::DispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
}

void FMetalCommandContext::PresentViewport(FRHIViewport* Viewport, bool bVerticalSync)
{
    FMetalViewport* MetalViewport = static_cast<FMetalViewport*>(Viewport);
    MetalViewport->Present(bVerticalSync);
}

void FMetalCommandContext::ClearState()
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

void FMetalCommandContext::InsertMarker(const FStringView& Message)
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

    [Encoder insertDebugSignpost:FString(Message).GetNSString()];
}

void FMetalCommandContext::BeginExternalCapture()
{
    // Empty for now
}

void FMetalCommandContext::EndExternalCapture() 
{
    // Empty for now
}
