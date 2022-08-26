#include "MetalCommandContext.h"
#include "MetalDeviceContext.h"
#include "MetalBuffer.h"
#include "MetalTexture.h"
#include "MetalViewport.h"
#include "MetalPipelineState.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalCommandContext

CMetalCommandContext::CMetalCommandContext(CMetalDeviceContext* InDeviceContext)
    : CMetalObject(InDeviceContext)
    , IRHICommandContext()
    , CommandBuffer(nil)
    , GraphicsEncoder(nil)
{
    ClearState();
}

CMetalCommandContext* CMetalCommandContext::CreateMetalContext(CMetalDeviceContext* InDeviceContext)
{ 
    return dbg_new CMetalCommandContext(InDeviceContext);
}

void CMetalCommandContext::StartContext() 
{
    Check(CommandBuffer == nil);
    
    id<MTLCommandQueue> CommandQueue = GetDeviceContext()->GetMTLCommandQueue();
    CommandBuffer = [CommandQueue commandBuffer];
}

void CMetalCommandContext::FinishContext()
{
    Check(CommandBuffer != nil);
    
    CopyContext.FinishContext();
    
    [CommandBuffer commit];
    [CommandBuffer release];
    
    CommandBuffer = nil;
}

void CMetalCommandContext::BeginTimeStamp(FRHITimestampQuery* Profiler, uint32 Index)
{
}

void CMetalCommandContext::EndTimeStamp(FRHITimestampQuery* Profiler, uint32 Index)
{
}

void CMetalCommandContext::ClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const TStaticArray<float, 4>& ClearColor)
{
    SCOPED_AUTORELEASE_POOL();
    
    CMetalTexture*  RTVTexture = GetMetalTexture(RenderTargetView.Texture);
    CMetalViewport* Viewport   = RTVTexture ? RTVTexture->GetViewport() : nullptr;
    
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

void CMetalCommandContext::ClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
}

void CMetalCommandContext::ClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor)
{
}

void CMetalCommandContext::BeginRenderPass(const FRHIRenderPassInitializer& RenderPassInitializer)
{
    SCOPED_AUTORELEASE_POOL();
    
    Check(GraphicsEncoder == nil);
    
    CopyContext.FinishContext();

    CMetalTexture* DSVTexture = GetMetalTexture(RenderPassInitializer.DepthStencilView.Texture);
    METAL_ERROR_COND((RenderPassInitializer.NumRenderTargets > 0) || (DSVTexture != nullptr), "A RenderPass needs a valid RenderTargetView or DepthStencilView");
    
    MTLRenderPassDescriptor* RenderPassDescriptor = [MTLRenderPassDescriptor new];
    RenderPassDescriptor.defaultRasterSampleCount = 1;
    RenderPassDescriptor.renderTargetArrayLength  = 1;
    
    for (uint32 Index = 0; Index < RenderPassInitializer.NumRenderTargets; ++Index)
    {
        const FRHIRenderTargetView& RenderTargetView = RenderPassInitializer.RenderTargets[Index];
        
        CMetalTexture*  RTVTexture = GetMetalTexture(RenderTargetView.Texture);
        CMetalViewport* Viewport   = RTVTexture ? RTVTexture->GetViewport() : nullptr;
        
        MTLRenderPassColorAttachmentDescriptor* ColorAttachment = RenderPassDescriptor.colorAttachments[Index];
        ColorAttachment.texture            = Viewport ? Viewport->GetDrawableTexture() : RTVTexture->GetMTLTexture();
        ColorAttachment.loadAction         = ConvertAttachmentLoadAction(RenderTargetView.LoadAction);
        ColorAttachment.clearColor         = MTLClearColorMake(RenderTargetView.ClearValue.R, RenderTargetView.ClearValue.G, RenderTargetView.ClearValue.B, RenderTargetView.ClearValue.A);
        ColorAttachment.level              = RenderTargetView.MipLevel;
        ColorAttachment.slice              = RenderTargetView.ArrayIndex;
        ColorAttachment.storeActionOptions = MTLStoreActionOptionNone;
        ColorAttachment.storeAction        = ConvertAttachmentStoreAction(RenderTargetView.StoreAction);
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

void CMetalCommandContext::EndRenderPass()
{
    Check(GraphicsEncoder != nil);
        
    [GraphicsEncoder endEncoding];
    NSRelease(GraphicsEncoder);
}

void CMetalCommandContext::SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
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

void CMetalCommandContext::SetScissorRect(float Width, float Height, float x, float y)
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

void CMetalCommandContext::SetBlendFactor(const TStaticArray<float, 4>& Color)
{
}

void CMetalCommandContext::SetVertexBuffers(FRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot)
{
    for (uint32 BufferIndex = 0; BufferIndex < BufferCount; ++BufferIndex)
    {
        const uint32 Index = BufferSlot + BufferIndex;
        
        CMetalVertexBuffer* CurrentBuffer = static_cast<CMetalVertexBuffer*>(VertexBuffers[Index]);
        CurrentVertexBuffers[Index] = CurrentBuffer ? CurrentBuffer->GetMTLBuffer() : nil;
        CurrentVertexOffsets[Index] = 0;
    }
    
    CurrentVertexBufferRange = NSMakeRange(NMath::Min<uint32>(BufferSlot, CurrentVertexBufferRange.location), NMath::Max<uint32>(BufferCount, CurrentVertexBufferRange.length));
}

void CMetalCommandContext::SetIndexBuffer(FRHIIndexBuffer* IndexBuffer)
{
    CurrentIndexBuffer = MakeSharedRef<CMetalIndexBuffer>(IndexBuffer);
}

void CMetalCommandContext::SetPrimitiveTopology(EPrimitiveTopology PrimitveTopology)
{
    CurrentPrimitiveType = ConvertPrimitiveTopology(PrimitveTopology);
}

void CMetalCommandContext::SetGraphicsPipelineState(FRHIGraphicsPipelineState* PipelineState)
{
    CurrentGraphicsPipeline = MakeSharedRef<CMetalGraphicsPipelineState>(PipelineState);
}

void CMetalCommandContext::SetComputePipelineState(FRHIComputePipelineState* PipelineState)
{
}

void CMetalCommandContext::Set32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
}

void CMetalCommandContext::SetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
    Check(Shader != nullptr);
    CMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check(ParameterIndex < kMaxSRVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentSRVs[Visibility][ParameterIndex] = MakeSharedRef<CMetalShaderResourceView>(ShaderResourceView);
}

void CMetalCommandContext::SetShaderResourceViews(FRHIShader* Shader, FRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex)
{
    Check(Shader              != nullptr);
    Check(ShaderResourceViews != nullptr);
    
    CMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check((ParameterIndex + NumShaderResourceViews) < kMaxSRVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (uint32 Index = 0; Index < NumShaderResourceViews; ++Index)
    {
        CurrentSRVs[Visibility][ParameterIndex + Index] = MakeSharedRef<CMetalShaderResourceView>(ShaderResourceViews[Index]);
    }
}

void CMetalCommandContext::SetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
    Check(Shader != nullptr);
    CMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check(ParameterIndex < kMaxUAVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentUAVs[Visibility][ParameterIndex] = MakeSharedRef<CMetalUnorderedAccessView>(UnorderedAccessView);
}

void CMetalCommandContext::SetUnorderedAccessViews(FRHIShader* Shader, FRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
{
    Check(Shader               != nullptr);
    Check(UnorderedAccessViews != nullptr);
    
    CMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check((ParameterIndex + NumUnorderedAccessViews) < kMaxUAVs);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (uint32 Index = 0; Index < NumUnorderedAccessViews; ++Index)
    {
        CurrentUAVs[Visibility][ParameterIndex + Index] = MakeSharedRef<CMetalUnorderedAccessView>(UnorderedAccessViews[Index]);
    }
}

void CMetalCommandContext::SetConstantBuffer(FRHIShader* Shader, FRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
{
    Check(Shader != nullptr);
    CMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check(ParameterIndex < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    CurrentConstantBuffers[Visibility][ParameterIndex] = MakeSharedRef<CMetalConstantBuffer>(ConstantBuffer);
}

void CMetalCommandContext::SetConstantBuffers(FRHIShader* Shader, FRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
{
    Check(Shader          != nullptr);
    Check(ConstantBuffers != nullptr);
    
    CMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check((ParameterIndex + NumConstantBuffers) < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (uint32 Index = 0; Index < NumConstantBuffers; ++Index)
    {
        CurrentConstantBuffers[Visibility][ParameterIndex + Index] = MakeSharedRef<CMetalConstantBuffer>(ConstantBuffers[Index]);
    }
}

void CMetalCommandContext::SetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
    Check(Shader != nullptr);
    CMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check(ParameterIndex < kMaxConstantBuffers);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    
    CMetalSamplerState* MetalSamplerState = static_cast<CMetalSamplerState*>(SamplerState);
    CurrentSamplerStates[Visibility][ParameterIndex] = MetalSamplerState ? MetalSamplerState->GetMTLSamplerState() : nil;
}

void CMetalCommandContext::SetSamplerStates(FRHIShader* Shader, FRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
{
    Check(Shader        != nullptr);
    Check(SamplerStates != nullptr);
    
    CMetalShader* MetalShader = GetMetalShader(Shader);
        
    Check((ParameterIndex + NumSamplerStates) < kMaxSamplerStates);

    const EShaderVisibility Visibility = MetalShader->GetVisbility();
    for (uint32 Index = 0; Index < NumSamplerStates; ++Index)
    {
        CMetalSamplerState* MetalSamplerState = static_cast<CMetalSamplerState*>(SamplerStates[Index]);
        CurrentSamplerStates[Visibility][ParameterIndex + Index] = MetalSamplerState ? MetalSamplerState->GetMTLSamplerState() : nil;
    }

    CurrentSamplerStateRange[Visibility] = NSMakeRange( NMath::Min<uint32>(ParameterIndex, CurrentSamplerStateRange[Visibility].location)
                                                      , NMath::Max<uint32>(NumSamplerStates, CurrentSamplerStateRange[Visibility].length));
}

void CMetalCommandContext::UpdateBuffer(FRHIBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
}

void CMetalCommandContext::UpdateTexture2D(FRHITexture2D* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
{
}

void CMetalCommandContext::ResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
}

void CMetalCommandContext::CopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHICopyBufferInfo& CopyInfo)
{
    CMetalBuffer* MetalDst = GetMetalBuffer(Dst);
    CMetalBuffer* MetalSrc = GetMetalBuffer(Src);
    
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

void CMetalCommandContext::CopyTexture(FRHITexture* Dst, FRHITexture* Src)
{
    CMetalTexture* MetalDst = GetMetalTexture(Dst);
    CMetalTexture* MetalSrc = GetMetalTexture(Src);
    
    Check(CommandBuffer != nil);
    Check(MetalDst      != nullptr);
    Check(MetalSrc      != nullptr);
    
    CopyContext.StartContext(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder copyFromTexture:MetalSrc->GetMTLTexture() toTexture:MetalDst->GetMTLTexture()];
    
    CopyContext.FinishContext();
}

void CMetalCommandContext::CopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHICopyTextureInfo& CopyTextureInfo)
{
}

void CMetalCommandContext::DestroyResource(class IRHIResource* Resource)
{
}

void CMetalCommandContext::DiscardContents(class FRHITexture* Texture)
{
}

void CMetalCommandContext::BuildRayTracingGeometry(FRHIRayTracingGeometry* Geometry, FRHIVertexBuffer* VertexBuffer, FRHIIndexBuffer* IndexBuffer, bool bUpdate)
{
}

void CMetalCommandContext::BuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
{
}

void CMetalCommandContext::SetRayTracingBindings( FRHIRayTracingScene* RayTracingScene
                                                , FRHIRayTracingPipelineState* PipelineState
                                                , const FRayTracingShaderResources* GlobalResource
                                                , const FRayTracingShaderResources* RayGenLocalResources
                                                , const FRayTracingShaderResources* MissLocalResources
                                                , const FRayTracingShaderResources* HitGroupResources
                                                , uint32 NumHitGroupResources)
{
}

void CMetalCommandContext::GenerateMips(FRHITexture* Texture)
{
    CMetalTexture* MetalTexture = GetMetalTexture(Texture);
    Check(MetalTexture != nullptr);
    
    // Cannot call generatemips inside of a RenderPass
    Check(GraphicsEncoder == nil);
    
    CopyContext.StartContext(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder generateMipmapsForTexture:MetalTexture->GetMTLTexture()];
}

void CMetalCommandContext::TransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
{
}

void CMetalCommandContext::TransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
}

void CMetalCommandContext::UnorderedAccessTextureBarrier(FRHITexture* Texture)
{
}

void CMetalCommandContext::UnorderedAccessBufferBarrier(FRHIBuffer* Buffer)
{
}

void CMetalCommandContext::PrepareForDraw()
{
    Check(GraphicsEncoder != nil);
    
    [GraphicsEncoder setViewport:CurrentViewport];
    
    // Necessary to retrieve all states and the resource bindings
    if (CurrentGraphicsPipeline)
    {
        CMetalDepthStencilState* DepthStencilState = CurrentGraphicsPipeline->GetMetalDepthStencilState();
        Check(DepthStencilState != nullptr);
        
        [GraphicsEncoder setDepthStencilState:DepthStencilState->GetMTLDepthStencilState()];
        
        CMetalRasterizerState* RasterizerState = CurrentGraphicsPipeline->GetMetalRasterizerState();
        Check(RasterizerState != nullptr);
        
        [GraphicsEncoder setFrontFacingWinding:RasterizerState->GetMTLFrontFaceWinding()];
        [GraphicsEncoder setTriangleFillMode:RasterizerState->GetMTLFillMode()];
        
        //[GraphicsEncoder setRenderPipelineState:CurrentGraphicsPipeline->GetMTLPipelineState()];
        
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

void CMetalCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
    Check(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    Check(CurrentPrimitiveType != MTLPrimitiveType(-1));
    //[GraphicsEncoder drawPrimitives:CurrentPrimitiveType vertexStart:StartVertexLocation vertexCount:VertexCount];
}

void CMetalCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
    Check(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    Check(CurrentIndexBuffer   != nullptr);
    Check(CurrentPrimitiveType != MTLPrimitiveType(-1));
    
    /*
    [GraphicsEncoder drawIndexedPrimitives:CurrentPrimitiveType
                                indexCount:IndexCount
                                 indexType:(CurrentIndexBuffer->GetFormat() == EIndexFormat::uint32) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16
                               indexBuffer:CurrentIndexBuffer->GetMTLBuffer()
                         indexBufferOffset:CurrentIndexBuffer->GetStride() * StartIndexLocation
                             instanceCount:1
                                baseVertex:BaseVertexLocation
                              baseInstance:0];*/
}

void CMetalCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
    Check(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    Check(CurrentPrimitiveType != MTLPrimitiveType(-1));
    /*[GraphicsEncoder drawPrimitives:CurrentPrimitiveType
                        vertexStart:StartVertexLocation
                        vertexCount:VertexCountPerInstance
                      instanceCount:InstanceCount
                       baseInstance:StartInstanceLocation];*/
}

void CMetalCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
    Check(GraphicsEncoder != nil);
    
    PrepareForDraw();
    
    Check(CurrentIndexBuffer   != nullptr);
    Check(CurrentPrimitiveType != MTLPrimitiveType(-1));
    
    /*[GraphicsEncoder drawIndexedPrimitives:CurrentPrimitiveType
                                indexCount:IndexCountPerInstance
                                 indexType:(CurrentIndexBuffer->GetFormat() == EIndexFormat::uint32) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16
                               indexBuffer:CurrentIndexBuffer->GetMTLBuffer()
                         indexBufferOffset:CurrentIndexBuffer->GetStride() * StartIndexLocation
                             instanceCount:InstanceCount
                                baseVertex:BaseVertexLocation
                              baseInstance:StartInstanceLocation];*/
}

void CMetalCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
}

void CMetalCommandContext::DispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
}

void CMetalCommandContext::ClearState()
{
    // Viewport
    CMemory::Memzero(&CurrentViewport);
        
    // VertexBuffers
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
        CurrentSamplerStateRange[ShaderStage] = NSMakeRange(0, 0);
        
        CurrentSRVs[ShaderStage].Fill(nullptr);
        CurrentUAVs[ShaderStage].Fill(nullptr);
        CurrentConstantBuffers[ShaderStage].Fill(nullptr);
        
        CurrentBuffers[ShaderStage].Fill(nil);
        CurrentTextures[ShaderStage].Fill(nil);
    }
    
    Flush();
}

void CMetalCommandContext::Flush()
{
    if (CommandBuffer)
    {
        [CommandBuffer commit];
        [CommandBuffer waitUntilCompleted];
    }
}

void CMetalCommandContext::InsertMarker(const String& Message)
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
