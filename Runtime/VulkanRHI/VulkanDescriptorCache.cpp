#include "VulkanDescriptorCache.h"

FVulkanDescriptorSetCache::FVulkanDescriptorSetCache(FVulkanDevice* InDevice, FVulkanCommandContext& InContext)
    : FVulkanDeviceObject(InDevice)
    , Context(InContext)
{
}

bool FVulkanDescriptorSetCache::Initialize()
{
    return true;
}

void FVulkanDescriptorSetCache::DirtyState()
{
}

void FVulkanDescriptorSetCache::DirtyStateSamplers()
{
}

void FVulkanDescriptorSetCache::DirtyStateResources()
{
}

void FVulkanDescriptorSetCache::SetVertexBuffers(FVulkanVertexBufferCache& VertexBuffersCache)
{
    Context.GetCommandBuffer().BindVertexBuffers(0, VertexBuffersCache.NumVertexBuffers, VertexBuffersCache.VertexBuffers, VertexBuffersCache.VertexBufferOffsets);
}

void FVulkanDescriptorSetCache::SetIndexBuffer(FVulkanIndexBufferCache& IndexBufferCache)
{
    Context.GetCommandBuffer().BindIndexBuffer(IndexBufferCache.IndexBuffer, IndexBufferCache.Offset, IndexBufferCache.IndexType);
}

void FVulkanDescriptorSetCache::SetSRVs(FVulkanShaderResourceViewCache& Cache, EShaderVisibility ShaderStage, uint32 NumSRVs)
{
}

void FVulkanDescriptorSetCache::SetUAVs(FVulkanUnorderedAccessViewCache& Cache, EShaderVisibility ShaderStage, uint32 NumUAVs)
{
}

void FVulkanDescriptorSetCache::SetCBVs(FVulkanConstantBufferCache& Cache, EShaderVisibility ShaderStage, uint32 NumCBVs)
{
}

void FVulkanDescriptorSetCache::SetSamplers(FVulkanSamplerStateCache& Cache, EShaderVisibility ShaderStage, uint32 NumSamplers)
{
}
