#include "VulkanCommandContext.h"
#include "VulkanResourceView.h"
#include "VulkanTexture.h"
#include "VulkanViewport.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FVulkanCommandContext::FVulkanCommandContext(FVulkanDevice* InDevice, FVulkanQueue* InCommandQueue)
    : FVulkanDeviceObject(InDevice)
    , CommandQueue(MakeSharedRef<FVulkanQueue>(InCommandQueue))
    , CommandBuffer(InDevice, InCommandQueue->GetType())
{
}

FVulkanCommandContext::~FVulkanCommandContext()
{
}

bool FVulkanCommandContext::Initialize()
{
    if (!CommandBuffer.Initialize(VK_COMMAND_BUFFER_LEVEL_PRIMARY))
    {
        return false;
    }

    return true;
}

void FVulkanCommandContext::FlushCommands()
{
    if (CommandBuffer.IsRecording())
    {
        VULKAN_ERROR_COND(CommandBuffer.End(), "Failed to End CommandBuffer");

        FVulkanCommandBuffer* SubmitCommandBuffer = GetCommandBuffer();
        CommandQueue->ExecuteCommandBuffer(&SubmitCommandBuffer, 1, CommandBuffer.GetFence());
    }
}

void FVulkanCommandContext::RHIStartContext()
{
    VULKAN_ERROR_COND(CommandBuffer.Begin(), "Failed to Begin CommandBuffer");
}

void FVulkanCommandContext::RHIFinishContext()
{
    FlushCommands();
}

void FVulkanCommandContext::RHIBeginTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)
{
}

void FVulkanCommandContext::RHIEndTimeStamp(FRHITimestampQuery* TimestampQuery, uint32 Index)  
{
}

void FVulkanCommandContext::RHIClearRenderTargetView(const FRHIRenderTargetView& RenderTargetView, const FVector4& ClearColor)
{
    /*FVulkanRenderTargetView* VulkanRTV = reinterpret_cast<FVulkanRenderTargetView*>(RenderTargetView);
    VULKAN_ERROR_COND(VulkanRTV, "Trying to clear an RenderTargetView that is nullptr");
    
    VkClearColorValue VulkanClearColor;
    FMemory::Memcpy(VulkanClearColor.float32, ClearColor.Elements, sizeof(ClearColor.Elements));
    
    FVulkanImageView* ImageView = VulkanRTV->GetImageView();
    if (ImageView)
    {
        // VULKAN_ERROR_COND(ImageView, "Trying to clear an image view that is nullptr");

        VkImageSubresourceRange SubresourceRange = ImageView->GetSubresourceRange();
        CommandBuffer.ClearColorImage(ImageView->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &VulkanClearColor, 1, &SubresourceRange);
    }*/
}

void FVulkanCommandContext::RHIClearDepthStencilView(const FRHIDepthStencilView& DepthStencilView, const float Depth, uint8 Stencil)
{
    //CommandBuffer.ClearDepthImage();
}

void FVulkanCommandContext::RHIClearUnorderedAccessViewFloat(FRHIUnorderedAccessView* UnorderedAccessView, const FVector4& ClearColor)
{
}

void FVulkanCommandContext::RHIBeginRenderPass(const FRHIRenderPassDesc& RenderPassInitializer)
{
    //CommandBuffer.BeginRenderPass();
}

void FVulkanCommandContext::RHIEndRenderPass()  
{
    //CommandBuffer.EndRenderPass();
}

void FVulkanCommandContext::RHISetViewport(const FRHIViewportRegion& ViewportRegion)
{
}

void FVulkanCommandContext::RHISetScissorRect(const FRHIScissorRegion& ScissorRegion)
{
}

void FVulkanCommandContext::RHISetBlendFactor(const FVector4& Color)
{
}

void FVulkanCommandContext::RHISetVertexBuffers(const TArrayView<FRHIBuffer* const> InVertexBuffers, uint32 BufferSlot)
{
}

void FVulkanCommandContext::RHISetIndexBuffer(FRHIBuffer* IndexBuffer, EIndexFormat IndexFormat)
{
}

void FVulkanCommandContext::RHISetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
{
}

void FVulkanCommandContext::RHISetGraphicsPipelineState(class FRHIGraphicsPipelineState* PipelineState)
{
}

void FVulkanCommandContext::RHISetComputePipelineState(class FRHIComputePipelineState* PipelineState)  
{
}

void FVulkanCommandContext::RHISet32BitShaderConstants(FRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
}

void FVulkanCommandContext::RHISetShaderResourceView(FRHIShader* Shader, FRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetShaderResourceViews(FRHIShader* Shader, const TArrayView<FRHIShaderResourceView* const> ShaderResourceViews, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetUnorderedAccessView(FRHIShader* Shader, FRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetUnorderedAccessViews(FRHIShader* Shader, const TArrayView<FRHIUnorderedAccessView* const> UnorderedAccessViews, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetConstantBuffer(FRHIShader* Shader, FRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetConstantBuffers(FRHIShader* Shader, const TArrayView<FRHIBuffer* const> ConstantBuffers, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetSamplerState(FRHIShader* Shader, FRHISamplerState* SamplerState, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHISetSamplerStates(FRHIShader* Shader, const TArrayView<FRHISamplerState* const> SamplerStates, uint32 ParameterIndex)
{
}

void FVulkanCommandContext::RHIUpdateBuffer(FRHIBuffer* Dst, const FBufferRegion& BufferRegion, const void* SrcData)     
{
}

void FVulkanCommandContext::RHIUpdateTexture2D(FRHITexture* Dst, const FTextureRegion2D& TextureRegion, uint32 MipLevel, const void* SrcData, uint32 SrcRowPitch) 
{
}

void FVulkanCommandContext::RHIResolveTexture(FRHITexture* Dst, FRHITexture* Src)
{
}

void FVulkanCommandContext::RHICopyBuffer(FRHIBuffer* Dst, FRHIBuffer* Src, const FRHIBufferCopyDesc& CopyDesc)
{
}

void FVulkanCommandContext::RHICopyTexture(FRHITexture* Destination, FRHITexture* Source)
{
}

void FVulkanCommandContext::RHICopyTextureRegion(FRHITexture* Dst, FRHITexture* Src, const FRHITextureCopyDesc& CopyDesc)
{
}

void FVulkanCommandContext::RHIDestroyResource(class IRefCounted* Resource)  
{
}

void FVulkanCommandContext::RHIDiscardContents(class FRHITexture* Resource)
{
}

void FVulkanCommandContext::RHIBuildRayTracingGeometry(
    FRHIRayTracingGeometry* RayTracingGeometry,
    FRHIBuffer*             VertexBuffer,
    uint32                  NumVertices,
    FRHIBuffer*             IndexBuffer,
    uint32                  NumIndices,
    EIndexFormat            IndexFormat,
    bool                    bUpdate)
{
}

void FVulkanCommandContext::RHIBuildRayTracingScene(FRHIRayTracingScene* RayTracingScene, const TArrayView<const FRHIRayTracingGeometryInstance>& Instances, bool bUpdate)
{
}

void FVulkanCommandContext::RHISetRayTracingBindings(
    FRHIRayTracingScene*              RayTracingScene,
    FRHIRayTracingPipelineState*      PipelineState,
    const FRayTracingShaderResources* GlobalResource,
    const FRayTracingShaderResources* RayGenLocalResources,
    const FRayTracingShaderResources* MissLocalResources,
    const FRayTracingShaderResources* HitGroupResources,
    uint32                            NumHitGroupResources)
{
}

void FVulkanCommandContext::RHIGenerateMips(FRHITexture* Texture)
{
}

void FVulkanCommandContext::RHITransitionTexture(FRHITexture* Texture, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(Texture);
    if (VulkanTexture->GetVkImage() == VK_NULL_HANDLE)
    {
        return;
    }
    
    FVulkanImageTransitionBarrier TransitionBarrier;
    TransitionBarrier.Image                           = VulkanTexture->GetVkImage();
    TransitionBarrier.PreviousLayout                  = ConvertResourceStateToImageLayout(BeforeState);
    TransitionBarrier.NewLayout                       = ConvertResourceStateToImageLayout(AfterState);
    TransitionBarrier.DependencyFlags                 = 0;
    TransitionBarrier.SrcAccessMask                   = VK_ACCESS_NONE;
    TransitionBarrier.DstAccessMask                   = VK_ACCESS_NONE;
    TransitionBarrier.SrcStageMask                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    TransitionBarrier.DstStageMask                    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    TransitionBarrier.SubresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
    TransitionBarrier.SubresourceRange.baseMipLevel   = 0;
    TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    TransitionBarrier.SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
    
    CommandBuffer.ImageLayoutTransitionBarrier(TransitionBarrier);
}

void FVulkanCommandContext::RHITransitionBuffer(FRHIBuffer* Buffer, EResourceAccess BeforeState, EResourceAccess AfterState)   
{
}

void FVulkanCommandContext::RHIUnorderedAccessTextureBarrier(FRHITexture* Texture)
{
}

void FVulkanCommandContext::RHIUnorderedAccessBufferBarrier(FRHIBuffer* Buffer)   
{
}

void FVulkanCommandContext::RHIDraw(uint32 VertexCount, uint32 StartVertexLocation)
{
}

void FVulkanCommandContext::RHIDrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
}

void FVulkanCommandContext::RHIDrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
}

void FVulkanCommandContext::RHIDrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
}

void FVulkanCommandContext::RHIDispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
}

void FVulkanCommandContext::RHIDispatchRays(FRHIRayTracingScene* InScene, FRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
}

void FVulkanCommandContext::RHIPresentViewport(FRHIViewport* Viewport, bool bVerticalSync)
{
    FlushCommands();

    FVulkanViewport* VulkanViewport = static_cast<FVulkanViewport*>(Viewport);
    VulkanViewport->Present(bVerticalSync);
}

void FVulkanCommandContext::RHIClearState()
{
    RHIFlush();
}

void FVulkanCommandContext::RHIFlush()
{
    FlushCommands();

    CommandQueue->WaitForCompletion();
}

void FVulkanCommandContext::RHIInsertMarker(const FStringView& Message)
{
}

void FVulkanCommandContext::RHIBeginExternalCapture()
{
    // TODO: Investigate the probability of this
}

void FVulkanCommandContext::RHIEndExternalCapture()  
{
    // TODO: Investigate the probability of this
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
