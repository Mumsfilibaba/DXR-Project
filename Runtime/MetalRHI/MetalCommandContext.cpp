#include "MetalCommandContext.h"
#include "MetalDeviceContext.h"
#include "MetalTexture.h"
#include "MetalViewport.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalCommandContext

CMetalCommandContext::CMetalCommandContext(CMetalDeviceContext* InDeviceContext)
    : CMetalObject(InDeviceContext)
    , IRHICommandContext()
    , CommandBuffer(nil)
    , GraphicsEncoder(nil)
    , RenderPassDescriptor(nil)
{ }

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
    [CommandBuffer waitUntilCompleted];
    [CommandBuffer release];
    CommandBuffer = nil;
}

void CMetalCommandContext::BeginTimeStamp(CRHITimestampQuery* Profiler, uint32 Index)
{
}

void CMetalCommandContext::EndTimeStamp(CRHITimestampQuery* Profiler, uint32 Index)
{
}

void CMetalCommandContext::ClearRenderTargetView(const CRHIRenderTargetView& RenderTargetView, const TStaticArray<float, 4>& ClearColor)
{
}

void CMetalCommandContext::ClearDepthStencilView(const CRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
}

void CMetalCommandContext::ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const TStaticArray<float, 4>& ClearColor)
{
}

void CMetalCommandContext::BeginRenderPass(const CRHIRenderPassInitializer& RenderPassInitializer)
{
    Check(GraphicsEncoder      == nil);
    Check(RenderPassDescriptor == nil);
    
    CopyContext.FinishContext();

    CMetalTexture* DSVTexture = GetMetalTexture(RenderPassInitializer.DepthStencilView.Texture);
    METAL_ERROR_COND((RenderPassInitializer.NumRenderTargets > 0) || (DSVTexture != nullptr), "A RenderPass needs a valid RenderTargetView or DepthStencilView");
    
    RenderPassDescriptor = [MTLRenderPassDescriptor new];
    RenderPassDescriptor.defaultRasterSampleCount = 1;
    RenderPassDescriptor.renderTargetArrayLength  = 1;
    
    for (uint32 Index = 0; Index < RenderPassInitializer.NumRenderTargets; ++Index)
    {
        const CRHIRenderTargetView& RenderTargetView = RenderPassInitializer.RenderTargets[Index];
        
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
        const CRHIDepthStencilView& DepthStencilView = RenderPassInitializer.DepthStencilView;
        
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
}

void CMetalCommandContext::EndRenderPass()
{
    Check(GraphicsEncoder      != nil);
    Check(RenderPassDescriptor != nil);
        
    [GraphicsEncoder endEncoding];
    [GraphicsEncoder release];
    GraphicsEncoder = nil;

    [RenderPassDescriptor release];
    RenderPassDescriptor = nil;
}

void CMetalCommandContext::SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
{
    Check(GraphicsEncoder != nil);
    
    MTLViewport Viewport;
    Viewport.width   = Width;
    Viewport.height  = Height;
    Viewport.originX = x;
    Viewport.originY = y;
    Viewport.znear   = MinDepth;
    Viewport.zfar    = MaxDepth;
    
    [GraphicsEncoder setViewport:Viewport];
}

void CMetalCommandContext::SetScissorRect(float Width, float Height, float x, float y)
{
    // TODO: ImGui is screwing something up here
    /*Check(GraphicsEncoder != nil);
    
    // Ensure that the size is correct;
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

void CMetalCommandContext::SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot)
{
}

void CMetalCommandContext::SetIndexBuffer(CRHIIndexBuffer* IndexBuffer)
{
}

void CMetalCommandContext::SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
{
}

void CMetalCommandContext::SetGraphicsPipelineState(CRHIGraphicsPipelineState* PipelineState)
{
}

void CMetalCommandContext::SetComputePipelineState(CRHIComputePipelineState* PipelineState)
{
}

void CMetalCommandContext::Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
}

void CMetalCommandContext::SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
}

void CMetalCommandContext::SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex)
{
}

void CMetalCommandContext::SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
}

void CMetalCommandContext::SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
{
}

void CMetalCommandContext::SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
{
}

void CMetalCommandContext::SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
{
}

void CMetalCommandContext::SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex)
{
}

void CMetalCommandContext::SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
{
}

void CMetalCommandContext::UpdateBuffer(CRHIBuffer* Dst, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)
{
}

void CMetalCommandContext::UpdateTexture2D(CRHITexture2D* Dst, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
{
}

void CMetalCommandContext::ResolveTexture(CRHITexture* Dst, CRHITexture* Src)
{
}

void CMetalCommandContext::CopyBuffer(CRHIBuffer* Dst, CRHIBuffer* Src, const SRHICopyBufferInfo& CopyInfo)
{
}

void CMetalCommandContext::CopyTexture(CRHITexture* Dst, CRHITexture* Src)
{
}

void CMetalCommandContext::CopyTextureRegion(CRHITexture* Dst, CRHITexture* Src, const SRHICopyTextureInfo& CopyTextureInfo)
{
}

void CMetalCommandContext::DestroyResource(class IRHIResource* Resource)
{
}

void CMetalCommandContext::DiscardContents(class CRHITexture* Texture)
{
}

void CMetalCommandContext::BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool bUpdate)
{
}

void CMetalCommandContext::BuildRayTracingScene(CRHIRayTracingScene* RayTracingScene, const TArrayView<const CRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
{
}

void CMetalCommandContext::SetRayTracingBindings( CRHIRayTracingScene* RayTracingScene
                                                , CRHIRayTracingPipelineState* PipelineState
                                                , const SRayTracingShaderResources* GlobalResource
                                                , const SRayTracingShaderResources* RayGenLocalResources
                                                , const SRayTracingShaderResources* MissLocalResources
                                                , const SRayTracingShaderResources* HitGroupResources
                                                , uint32 NumHitGroupResources)
{
}

void CMetalCommandContext::GenerateMips(CRHITexture* Texture)
{
    CMetalTexture* MetalTexture = GetMetalTexture(Texture);
    Check(MetalTexture != nullptr);
    
    CopyContext.StartContext(CommandBuffer);
    
    id<MTLBlitCommandEncoder> CopyEncoder = CopyContext.GetMTLCopyEncoder();
    [CopyEncoder generateMipmapsForTexture:MetalTexture->GetMTLTexture()];
}

void CMetalCommandContext::TransitionTexture(CRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
{
}

void CMetalCommandContext::TransitionBuffer(CRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)
{
}

void CMetalCommandContext::UnorderedAccessTextureBarrier(CRHITexture* Texture)
{
}

void CMetalCommandContext::UnorderedAccessBufferBarrier(CRHIBuffer* Buffer)
{
}

void CMetalCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
}

void CMetalCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
}

void CMetalCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
}

void CMetalCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
}

void CMetalCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
}

void CMetalCommandContext::DispatchRays(CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
}

void CMetalCommandContext::ClearState()
{
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
