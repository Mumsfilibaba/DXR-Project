#include "VulkanCommandContext.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanCommandContext

TSharedRef<CVulkanCommandContext> CVulkanCommandContext::CreateCommandContext(CVulkanDevice* InDevice, CVulkanCommandQueue* InCommandQueue)
{
    TSharedRef<CVulkanCommandContext> NewCommandContext = dbg_new CVulkanCommandContext(InDevice, InCommandQueue);
    if (NewCommandContext && NewCommandContext->Initialize())
    {
        return NewCommandContext;
    }

    return nullptr;
}

CVulkanCommandContext::CVulkanCommandContext(CVulkanDevice* InDevice, CVulkanCommandQueue* InCommandQueue)
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
}

void CVulkanCommandContext::ClearDepthStencilView(CRHIDepthStencilView* DepthStencilView, const SDepthStencil& ClearValue)        
{
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
}

void CVulkanCommandContext::EndRenderPass()  
{
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

void CVulkanCommandContext::SetVertexBuffers(CRHIVertexBuffer* const* VertexBuffers, uint32 BufferCount, uint32 BufferSlot)
{
}

void CVulkanCommandContext::SetIndexBuffer(CRHIIndexBuffer* IndexBuffer)
{
}

void CVulkanCommandContext::SetPrimitiveTopology(EPrimitiveTopology PrimitveTopologyType)
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

void CVulkanCommandContext::SetConstantBuffer(CRHIShader* Shader, CRHIConstantBuffer* ConstantBuffer, uint32 ParameterIndex)
{
}

void CVulkanCommandContext::SetConstantBuffers(CRHIShader* Shader, CRHIConstantBuffer* const* ConstantBuffers, uint32 NumConstantBuffers, uint32 ParameterIndex)
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

void CVulkanCommandContext::BuildRayTracingGeometry(CRHIRayTracingGeometry* Geometry, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer, bool bUpdate)      
{
}

void CVulkanCommandContext::BuildRayTracingScene(CRHIRayTracingScene* RayTracingScene, const SRayTracingGeometryInstance* Instances, uint32 NumInstances, bool bUpdate)
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
