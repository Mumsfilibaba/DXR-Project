#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalDeviceContext.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalSwapChain.h"
#include "MetalRHI/MetalPipelineState.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FMetalCommandContext::FMetalCommandContext(FMetalDeviceContext* InDeviceContext)
    : FMetalDeviceChild(InDeviceContext)
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
    
    CopyContext.FinishEncoder();
    
    [CommandBuffer commit];
    [CommandBuffer release];
    
    CommandBuffer = nil;
}

void FMetalCommandContext::QueryTimestamp(FRHIQuery* Query)
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
    
    [RenderPassDescriptor release];
    
    [GraphicsEncoder endEncoding];
    GraphicsEncoder = nil;
}

void FMetalCommandContext::ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
}

void FMetalCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
}

void FMetalCommandContext::BeginRenderPass(const FRHIBeginRenderPassInfo& BeginRenderPassInfo)
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
        ColorAttachment.clearColor         = MTLClearColorMake(RenderTargetView.ClearValue.R, RenderTargetView.ClearValue.G, RenderTargetView.ClearValue.B, RenderTargetView.ClearValue.A);
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

void FMetalCommandContext::EndRenderPass()
{
    CHECK(GraphicsEncoder != nil);
        
    [GraphicsEncoder endEncoding];
    [GraphicsEncoder release];
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
    
    CurrentViewport = Viewport;
}

void FMetalCommandContext::SetScissorRect(const FScissorRegion& ScissorRegion)
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
    
    CurrentVertexBufferRange = NSMakeRange(Math::Min<uint64>(BufferSlot, CurrentVertexBufferRange.location), Math::Max<uint64>(InVertexBuffers.Size(), CurrentVertexBufferRange.length));
}

void FMetalCommandContext::SetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
    CurrentIndexBuffer = MakeSharedRef<FMetalBuffer>(IndexBuffer);
}

void FMetalCommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    CurrentGraphicsPipeline = MakeSharedRef<FMetalGraphicsPipelineState>(PipelineState);
}

void FMetalCommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
}

void FMetalCommandContext::SetShaderConstants(FRHIShader* Shader, const void* ShaderConstants, uint32 NumShaderConstants)
{
}

void FMetalCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxSRVs);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    CurrentSRVs[Visibility][ParameterIndex] = MakeSharedRef<FMetalShaderResourceView>(ShaderResourceView);
}

void FMetalCommandContext::SetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> InShaderResourceViews, uint32 ParameterIndex)
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

void FMetalCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxUAVs);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    CurrentUAVs[Visibility][ParameterIndex] = MakeSharedRef<FMetalUnorderedAccessView>(UnorderedAccessView);
}

void FMetalCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> InUnorderedAccessViews, uint32 ParameterIndex)
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

void FMetalCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    CurrentConstantBuffers[Visibility][ParameterIndex] = MakeSharedRef<FMetalBuffer>(ConstantBuffer);
}

void FMetalCommandContext::SetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> InConstantBuffers, uint32 ParameterIndex)
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

void FMetalCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    FMetalShader* MetalShader = GetMetalShader(Shader);
    CHECK(MetalShader != nullptr);
    CHECK(ParameterIndex < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisibility();
    CurrentSamplerStates[Visibility][ParameterIndex] = MakeSharedRef<FMetalSamplerState>(SamplerState);
}

void FMetalCommandContext::SetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> InSamplerStates, uint32 ParameterIndex)
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
        Math::Min<uint32>(ParameterIndex, CurrentSamplerStateRange[Visibility].location),
        Math::Max<uint32>(InSamplerStates.Size(), CurrentSamplerStateRange[Visibility].length));*/
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

void FMetalCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FBufferCopyInfo& CopyDesc)
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

void FMetalCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
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

void FMetalCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FTextureCopyInfo& CopyDesc) 
{ 
} 
 
void FMetalCommandContext::CopyTextureRegionToBuffer(FRHIBuffer* Dst, uint64 DstOffset, FRHITexture* Src, const FTextureRegion2D& SrcRegion, uint32 SrcMipLevel) 
{ 
} 
 
void FMetalCommandContext::WriteFence(FRHIGpuFence* Fence) 
{ 
} 

void FMetalCommandContext::DiscardContents(class FRHITexture* Texture)
{
}

void FMetalCommandContext::BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const FRayTracingSceneBuildInfo& BuildInfo)
{
}

void FMetalCommandContext::BuildRayTracingGeometry(FRHIRayTracingGeometry* RayTracingGeometry, const FRayTracingGeometryBuildInfo& BuildInfo)
{
}

void FMetalCommandContext::SetRayTracingBindings(FRHIRayTracingScene* RayTracingScene, FRHIRayTracingPipelineState* PipelineState, const FRayTracingShaderResources* GlobalResource, const FRayTracingShaderResources* RayGenLocalResources, const FRayTracingShaderResources* MissLocalResources, const FRayTracingShaderResources* HitGroupResources, uint32 NumHitGroupResources)
{
}

void FMetalCommandContext::TransitionTexture(FRHITexture* Texture, const FRHITextureTransition& TextureTransition)
{
}

void FMetalCommandContext::TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
}

void FMetalCommandContext::RequireTextureState(FRHITexture* Texture, const FRHIRequiredTextureState& RequiredState)
{
}

void FMetalCommandContext::RequireBufferState(FRHIBuffer* Buffer, EResourceAccess RequiredState)
{
}

void FMetalCommandContext::EnableResourceStateTracking(FRHITexture* Texture, EResourceAccess InitialState)
{
}

void FMetalCommandContext::DisableResourceStateTracking(FRHITexture* Texture, EResourceAccess TargetState)
{
}

void FMetalCommandContext::EnableResourceStateTracking(FRHIBuffer* Buffer, EResourceAccess InitialState)
{
}

void FMetalCommandContext::DisableResourceStateTracking(FRHIBuffer* Buffer, EResourceAccess TargetState)
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

void FMetalCommandContext::PresentSwapChain(FRHISwapChain* SwapChain, bool bVerticalSync)
{
    FMetalSwapChain* MetalSwapChain = static_cast<FMetalSwapChain*>(SwapChain);
    MetalSwapChain->Present(bVerticalSync);
}

void FMetalCommandContext::ResizeSwapChain(FRHISwapChain* SwapChain, uint32 Width, uint32 Height)
{
    FMetalSwapChain* MetalSwapChain = static_cast<FMetalSwapChain*>(SwapChain);
    MetalSwapChain->Resize(Width, Height);
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
        CopyContext.StartEncoder(CommandBuffer);
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
