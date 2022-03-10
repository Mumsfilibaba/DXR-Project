#include "VulkanCommandContext.h"
#include "VulkanResourceView.h"
#include "VulkanTexture.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanCommandContext

CVulkanCommandContextRef CVulkanCommandContext::CreateCommandContext(CVulkanDevice* InDevice, CVulkanQueue* InCommandQueue)
{
    CVulkanCommandContextRef NewCommandContext = dbg_new CVulkanCommandContext(InDevice, InCommandQueue);
    if (NewCommandContext && NewCommandContext->Initialize())
    {
        return NewCommandContext;
    }

    return nullptr;
}

CVulkanCommandContext::CVulkanCommandContext(CVulkanDevice* InDevice, CVulkanQueue* InCommandQueue)
    : CVulkanDeviceObject(InDevice)
    , CommandQueue(::AddRef(InCommandQueue))
    , CommandBuffer(InDevice, InCommandQueue->GetType())
{
}

CVulkanCommandContext::~CVulkanCommandContext()
{
}

bool CVulkanCommandContext::Initialize()
{
    if (!CommandBuffer.Initialize(VK_COMMAND_BUFFER_LEVEL_PRIMARY))
    {
        return false;
    }

    return true;
}

void CVulkanCommandContext::Begin()
{
    VULKAN_ERROR(CommandBuffer.Begin(), "Failed to Begin CommandBuffer");
}

void CVulkanCommandContext::End()  
{
    VULKAN_ERROR(CommandBuffer.End(), "Failed to End CommandBuffer");

    CVulkanCommandBuffer* SubmitCommandBuffer = &CommandBuffer;
    CommandQueue->ExecuteCommandBuffer(&SubmitCommandBuffer, 1, CommandBuffer.GetFence());
}

void CVulkanCommandContext::BeginTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index)
{
}

void CVulkanCommandContext::EndTimeStamp(CRHITimestampQuery* TimestampQuery, uint32 Index)  
{
}

void CVulkanCommandContext::ClearRenderTargetView(CRHIRenderTargetView* RenderTargetView, const SColorF& ClearColor)
{
    CVulkanRenderTargetView* VulkanRTV = reinterpret_cast<CVulkanRenderTargetView*>(RenderTargetView);
    Assert(VulkanRTV != nullptr);
    
    VkClearColorValue VulkanClearColor;
    CMemory::Memcpy(VulkanClearColor.float32, ClearColor.Elements, sizeof(ClearColor.Elements));
    
    VkImageSubresourceRange SubresourceRange;
    SubresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    SubresourceRange.baseMipLevel   = 0;
    SubresourceRange.levelCount     = 1;
    SubresourceRange.baseArrayLayer = 0;
    SubresourceRange.layerCount     = 1;
    
    CVulkanImageView* ImageView = VulkanRTV->GetImageView();
    if (ImageView)
    {
        CommandBuffer.ClearColorImage(ImageView->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &VulkanClearColor, 1,  &SubresourceRange);
    }
}

void CVulkanCommandContext::ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const SDepthStencil& ClearValue)
{
    //CommandBuffer.ClearDepthImage();
}

void CVulkanCommandContext::ClearUnorderedAccessViewFloat(CRHIUnorderedAccessView* UnorderedAccessView, const SColorF& ClearColor)
{
}

void CVulkanCommandContext::SetShadingRate(ERHIShadingRate ShadingRate)     
{
}

void CVulkanCommandContext::SetShadingRateImage(CRHITexture2D* ShadingImage)
{
}

void CVulkanCommandContext::BeginRenderPass()
{
    //CommandBuffer.BeginRenderPass();
}

void CVulkanCommandContext::EndRenderPass()  
{
    //CommandBuffer.EndRenderPass();
}

void CVulkanCommandContext::SetViewport(float Width, float Height, float MinDepth, float MaxDepth, float x, float y)
{
}

void CVulkanCommandContext::SetScissorRect(float Width, float Height, float x, float y)
{
}

void CVulkanCommandContext::SetBlendFactor(const SColorF& Color)
{
}

void CVulkanCommandContext::SetRenderTargets(CRHIRenderTargetView* const* RenderTargetViews, uint32 RenderTargetCount, CRHIDepthStencilView* DepthStencilView)
{
}

void CVulkanCommandContext::SetVertexBuffers(CRHIBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot)
{
}

void CVulkanCommandContext::SetIndexBuffer(CRHIBuffer* IndexBuffer)
{
}

void CVulkanCommandContext::SetPrimitiveTopology(ERHIPrimitiveTopology PrimitveTopologyType)
{
}

void CVulkanCommandContext::SetGraphicsPipelineState(class CRHIGraphicsPipelineState* PipelineState)
{
}

void CVulkanCommandContext::SetComputePipelineState(class CRHIComputePipelineState* PipelineState)  
{
}

void CVulkanCommandContext::Set32BitShaderConstants(CRHIShader* Shader, const void* Shader32BitConstants, uint32 Num32BitConstants)
{
}

void CVulkanCommandContext::SetShaderResourceView(CRHIShader* Shader, CRHIShaderResourceView* ShaderResourceView, uint32 ParameterIndex)
{
}

void CVulkanCommandContext::SetShaderResourceViews(CRHIShader* Shader, CRHIShaderResourceView* const* ShaderResourceView, uint32 NumShaderResourceViews, uint32 ParameterIndex)
{
}

void CVulkanCommandContext::SetUnorderedAccessView(CRHIShader* Shader, CRHIUnorderedAccessView* UnorderedAccessView, uint32 ParameterIndex)
{
}

void CVulkanCommandContext::SetUnorderedAccessViews(CRHIShader* Shader, CRHIUnorderedAccessView* const* UnorderedAccessViews, uint32 NumUnorderedAccessViews, uint32 ParameterIndex)
{
}

void CVulkanCommandContext::SetConstantBuffer(CRHIShader* Shader, CRHIBuffer* ConstantBuffer, uint32 ParameterIndex)
{
}

void CVulkanCommandContext::SetConstantBuffers(CRHIShader* Shader, CRHIBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
{
}

void CVulkanCommandContext::SetSamplerState(CRHIShader* Shader, CRHISamplerState* SamplerState, uint32 ParameterIndex)
{
}

void CVulkanCommandContext::SetSamplerStates(CRHIShader* Shader, CRHISamplerState* const* SamplerStates, uint32 NumSamplerStates, uint32 ParameterIndex)
{
}

void CVulkanCommandContext::UpdateBuffer(CRHIBuffer* Destination, uint64 OffsetInBytes, uint64 SizeInBytes, const void* SourceData)          
{
}

void CVulkanCommandContext::UpdateTexture2D(CRHITexture2D* Destination, uint32 Width, uint32 Height, uint32 MipLevel, const void* SourceData)
{
}

void CVulkanCommandContext::ResolveTexture(CRHITexture* Destination, CRHITexture* Source)
{
}

void CVulkanCommandContext::CopyBuffer(CRHIBuffer* Destination, CRHIBuffer* Source, const SRHICopyBufferInfo& CopyInfo)
{
}

void CVulkanCommandContext::CopyTexture(CRHITexture* Destination, CRHITexture* Source)
{
}

void CVulkanCommandContext::CopyTextureRegion(CRHITexture* Destination, CRHITexture* Source, const SRHICopyTextureInfo& CopyTextureInfo)
{
}

void CVulkanCommandContext::DestroyResource(class CRHIObject* Resource)  
{
}

void CVulkanCommandContext::DiscardContents(class CRHIResource* Resource)
{
}

void CVulkanCommandContext::BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIBuffer* VertexBuffer, CRHIBuffer* IndexBuffer, bool bUpdate)      
{
}

void CVulkanCommandContext::BuildRayTracingScene(CRHIRayTracingScene* RayTracingScene, const SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate)
{
}

void CVulkanCommandContext::SetRayTracingBindings(
        CRHIRayTracingScene* RayTracingScene,
        CRHIRayTracingPipelineState* PipelineState,
        const SRayTracingShaderResources* GlobalResource,
        const SRayTracingShaderResources* RayGenLocalResources,
        const SRayTracingShaderResources* MissLocalResources,
        const SRayTracingShaderResources* HitGroupResources,
        uint32 NumHitGroupResources)
{
}

void CVulkanCommandContext::GenerateMips(CRHITexture* Texture)
{
}

void CVulkanCommandContext::TransitionTexture(CRHITexture* Texture, ERHIResourceState BeforeState, ERHIResourceState AfterState)
{
    CVulkanTexture* VulkanTexture = CastTexture(Texture);
    if (VulkanTexture->GetVkImage() == VK_NULL_HANDLE)
    {
        return;
    }
    
    SVulkanImageTransitionBarrier TransitionBarrier;
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

void CVulkanCommandContext::TransitionBuffer(CRHIBuffer* Buffer, ERHIResourceState BeforeState, ERHIResourceState AfterState)   
{
}

void CVulkanCommandContext::UnorderedAccessTextureBarrier(CRHITexture* Texture)
{
}

void CVulkanCommandContext::UnorderedAccessBufferBarrier(CRHIBuffer* Buffer)   
{
}

void CVulkanCommandContext::Draw(uint32 VertexCount, uint32 StartVertexLocation)
{
}

void CVulkanCommandContext::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, uint32 BaseVertexLocation)
{
}

void CVulkanCommandContext::DrawInstanced(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation)
{
}

void CVulkanCommandContext::DrawIndexedInstanced(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
}

void CVulkanCommandContext::Dispatch(uint32 WorkGroupsX, uint32 WorkGroupsY, uint32 WorkGroupsZ)
{
}

void CVulkanCommandContext::DispatchRays(CRHIRayTracingScene* InScene, CRHIRayTracingPipelineState* InPipelineState, uint32 InWidth, uint32 InHeight, uint32 InDepth)
{
}

void CVulkanCommandContext::ClearState()
{
}

void CVulkanCommandContext::Flush()
{
}

void CVulkanCommandContext::InsertMarker(const String& Message)
{
}

void CVulkanCommandContext::BeginExternalCapture()
{
}

void CVulkanCommandContext::EndExternalCapture()  
{
}
