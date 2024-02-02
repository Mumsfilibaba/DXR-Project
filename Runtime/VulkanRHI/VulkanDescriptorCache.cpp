#include "VulkanDescriptorCache.h"

bool FVulkanDefaultResources::Initialize(FVulkanDevice& Device)
{
    // Create NullBuffer
    VkBufferCreateInfo BufferCreateInfo;
    FMemory::Memzero(&BufferCreateInfo);

    BufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                 = nullptr;
    BufferCreateInfo.flags                 = 0;
    BufferCreateInfo.pQueueFamilyIndices   = nullptr;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                  = VULKAN_DEFAULT_BUFFER_NUM_BYTES;
    BufferCreateInfo.usage                 = 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VkResult Result = vkCreateBuffer(Device.GetVkDevice(), &BufferCreateInfo, nullptr, &NullBuffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Buffer");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullBuffer", NullBuffer, VK_OBJECT_TYPE_BUFFER);
    }

    // Allocate memory based on the buffer
    FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(NullBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, false, NullBufferMemory))
    {
        VULKAN_ERROR("Failed to allocate buffer memory");
        return false;
    }


    // Create a NullImage
    constexpr VkExtent3D NullExtent = { VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 1 };

    VkImageCreateInfo ImageCreateInfo;
    FMemory::Memzero(&ImageCreateInfo);

    ImageCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType             = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ImageCreateInfo.format                = VK_FORMAT_R8G8B8A8_UNORM;
    ImageCreateInfo.extent                = NullExtent;
    ImageCreateInfo.mipLevels             = 1;
    ImageCreateInfo.pQueueFamilyIndices   = nullptr;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples               = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.arrayLayers           = 1;

    Result = vkCreateImage(Device.GetVkDevice(), &ImageCreateInfo, nullptr, &NullImage);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create image");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullImage", NullImage, VK_OBJECT_TYPE_IMAGE);
    }

    if (!MemoryManager.AllocateImageMemory(NullImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, false, NullImageMemory))
    {
        VULKAN_ERROR("Failed to allocate ImageMemory");
        return false;
    }


    // Create NullImageView
    VkImageViewCreateInfo ImageViewCreateInfo;
    FMemory::Memzero(&ImageViewCreateInfo);

    ImageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.flags                           = 0;
    ImageViewCreateInfo.format                          = ImageCreateInfo.format;
    ImageViewCreateInfo.image                           = NullImage;
    ImageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    ImageViewCreateInfo.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    Result = vkCreateImageView(Device.GetVkDevice(), &ImageViewCreateInfo, nullptr, &NullImageView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkCreateImageView failed");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullImageView", NullImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
    }


    // Create a NullSampler
    VkSamplerCreateInfo SamplerCreateInfo;
    FMemory::Memzero(&SamplerCreateInfo);

    SamplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.magFilter               = VK_FILTER_LINEAR;
    SamplerCreateInfo.minFilter               = VK_FILTER_LINEAR;
    SamplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerCreateInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.mipLodBias              = 0.0f;
    SamplerCreateInfo.anisotropyEnable        = VK_FALSE;
    SamplerCreateInfo.maxAnisotropy           = 1.0f;
    SamplerCreateInfo.compareEnable           = VK_FALSE;
    SamplerCreateInfo.compareOp               = VK_COMPARE_OP_NEVER;
    SamplerCreateInfo.minLod                  = 0.0f;
    SamplerCreateInfo.maxLod                  = VK_LOD_CLAMP_NONE;
    SamplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    SamplerCreateInfo.unnormalizedCoordinates = false;

    Result = vkCreateSampler(Device.GetVkDevice(), &SamplerCreateInfo, nullptr, &NullSampler);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkCreateSampler failed");
        return false;
    }
    else
    {
        FVulkanDebugUtilsEXT::SetObjectName(Device.GetVkDevice(), "NullSampler", NullSampler, VK_OBJECT_TYPE_SAMPLER);
    }

    return true;
}

void FVulkanDefaultResources::Release(FVulkanDevice& Device)
{
    FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
    
    VkDevice VulkanDevice = Device.GetVkDevice();
    if (VULKAN_CHECK_HANDLE(NullBuffer))
    {
        vkDestroyBuffer(VulkanDevice, NullBuffer, nullptr);
        NullBuffer = VK_NULL_HANDLE;
        MemoryManager.Free(NullBufferMemory);
    }

    if (VULKAN_CHECK_HANDLE(NullImageView))
    {
        vkDestroyImageView(VulkanDevice, NullImageView, nullptr);
        NullImageView = VK_NULL_HANDLE;
    }

    if (VULKAN_CHECK_HANDLE(NullImage))
    {
        vkDestroyImage(VulkanDevice, NullImage, nullptr);
        NullImage = VK_NULL_HANDLE;
        MemoryManager.Free(NullImageMemory);
    }

    if (VULKAN_CHECK_HANDLE(NullSampler))
    {
        vkDestroySampler(VulkanDevice, NullSampler, nullptr);
        NullSampler = VK_NULL_HANDLE;
    }
}


FVulkanDescriptorSetCache::FVulkanDescriptorSetCache(FVulkanDevice* InDevice, FVulkanCommandContext& InContext)
    : FVulkanDeviceChild(InDevice)
    , Context(InContext)
    , DefaultResources()
    , DescriptorPool(VK_NULL_HANDLE)
    , PendingDescriptorPools()
    , AvailableDescriptorPools()
{
    FMemory::Memzero(DescriptorSets, sizeof(DescriptorSets));
}

FVulkanDescriptorSetCache::~FVulkanDescriptorSetCache()
{
    DefaultResources.Release(*GetDevice());
    
    if (VULKAN_CHECK_HANDLE(DescriptorPool))
    {
        vkDestroyDescriptorPool(GetDevice()->GetVkDevice(), DescriptorPool, nullptr);
        DescriptorPool = VK_NULL_HANDLE;
    }
}

bool FVulkanDescriptorSetCache::Initialize()
{
    if (!DefaultResources.Initialize(*GetDevice()))
    {
        return false;
    }
    
    if (!AllocateDescriptorPool())
    {
        return false;
    }
    
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

void FVulkanDescriptorSetCache::ResetPendingDescriptorPools()
{
    for (VkDescriptorPool CurrentPool : PendingDescriptorPools)
    {
        vkResetDescriptorPool(GetDevice()->GetVkDevice(), CurrentPool, 0);
        AvailableDescriptorPools.Add(CurrentPool);
    }
    
    PendingDescriptorPools.Clear();
}

bool FVulkanDescriptorSetCache::AllocateDescriptorSets(EShaderVisibility ShaderStage, VkDescriptorSetLayout Layout)
{
    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
    FMemory::Memzero(&DescriptorSetAllocateInfo);
    
    DescriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescriptorSetAllocateInfo.descriptorPool     = DescriptorPool;
    DescriptorSetAllocateInfo.descriptorSetCount = 1;
    DescriptorSetAllocateInfo.pSetLayouts        = &Layout;
    
    VkDescriptorSet& DescriptorSet = DescriptorSets[ShaderStage];
    DescriptorSet = VK_NULL_HANDLE;
    
    VkResult Result = vkAllocateDescriptorSets(GetDevice()->GetVkDevice(), &DescriptorSetAllocateInfo, &DescriptorSet);
    if (Result == VK_ERROR_OUT_OF_POOL_MEMORY)
    {
        AllocateDescriptorPool();
        
        // After we have allocated a new pool, we try and allocate again
        DescriptorSetAllocateInfo.descriptorPool = DescriptorPool;
        Result = vkAllocateDescriptorSets(GetDevice()->GetVkDevice(), &DescriptorSetAllocateInfo, &DescriptorSet);
    }
    
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to allocate descriptorset");
        return false;
    }
    
    return true;
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
    constexpr uint32 NumBufferSRVs       = VULKAN_DEFAULT_NUM_DESCRIPTOR_BINDINGS; // Limit due to MoltenVK
    constexpr uint32 NumDescriptorWrites = VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT + NumBufferSRVs;

    // TODO: We want to store this together with the PipelineLayout
    constexpr uint32 ImageBindingsStartIndex  = 40;
    constexpr uint32 BufferBindingsStartIndex = 16;

    VkDescriptorImageInfo  ImageInfos[VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT];
    VkDescriptorBufferInfo BufferInfos[NumBufferSRVs];
    VkWriteDescriptorSet   DescriptorWrites[NumDescriptorWrites];
    
    auto& SRVCache = Cache.ResourceViews[ShaderStage];
    for (uint32 Index = 0; Index < NumSRVs; Index++)
    {
        VkWriteDescriptorSet& CurrentDescriptorWrite = DescriptorWrites[Index];
        FMemory::Memzero(&CurrentDescriptorWrite);
        
        CurrentDescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        CurrentDescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        CurrentDescriptorWrite.dstSet          = DescriptorSets[ShaderStage];
        CurrentDescriptorWrite.descriptorCount = 1;
        CurrentDescriptorWrite.dstBinding      = ImageBindingsStartIndex + Index;
        
        VkDescriptorImageInfo& DescriptorInfo = ImageInfos[Index];
        CurrentDescriptorWrite.pImageInfo     = &DescriptorInfo;
        
        FVulkanShaderResourceView* ShaderResourceView = SRVCache[Index];
        if (ShaderResourceView && ShaderResourceView->HasImageView())
        {
            DescriptorInfo.imageView   = ShaderResourceView->GetVkImageView();
            DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            DescriptorInfo.sampler     = VK_NULL_HANDLE;
        }
        else
        {
            // If we have the NullDescriptors feature enable we don't create DefaultResources, so we just use the VK_NULL_HANDLE
            if (FVulkanRobustness2EXT::SupportsNullDescriptors())
            {
                DescriptorInfo.imageView   = VK_NULL_HANDLE;
                DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                DescriptorInfo.sampler     = VK_NULL_HANDLE;
            }
            else
            {
                DescriptorInfo.imageView   = DefaultResources.NullImageView;
                DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                DescriptorInfo.sampler     = VK_NULL_HANDLE;
            }
        }
    }


    // Limit NumSRVs since we have limited the amount of buffers allowed
    NumSRVs = FMath::Min(NumSRVs, NumBufferSRVs);
    for (uint32 Index = 0; Index < NumSRVs; Index++)
    {
        VkWriteDescriptorSet& CurrentDescriptorWrite = DescriptorWrites[VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT + Index];
        FMemory::Memzero(&CurrentDescriptorWrite);
        
        CurrentDescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        CurrentDescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        CurrentDescriptorWrite.dstSet          = DescriptorSets[ShaderStage];
        CurrentDescriptorWrite.descriptorCount = 1;
        CurrentDescriptorWrite.dstBinding      = BufferBindingsStartIndex + Index;
        
        VkDescriptorBufferInfo& DescriptorInfo = BufferInfos[Index];
        CurrentDescriptorWrite.pBufferInfo     = &DescriptorInfo;
        
        FVulkanShaderResourceView* ShaderResourceView = SRVCache[Index];
        if (ShaderResourceView && !ShaderResourceView->HasImageView())
        {
            DescriptorInfo = ShaderResourceView->GetDescriptorBufferInfo();
        }
        else
        {
            // If we have the NullDescriptors feature enable we don't create DefaultResources, so we just use the VK_NULL_HANDLE
            if (FVulkanRobustness2EXT::SupportsNullDescriptors())
            {
                DescriptorInfo.buffer = VK_NULL_HANDLE;
                DescriptorInfo.offset = 0;
                DescriptorInfo.range  = VK_WHOLE_SIZE;
            }
            else
            {
                DescriptorInfo.buffer = DefaultResources.NullBuffer;
                DescriptorInfo.offset = 0;
                DescriptorInfo.range  = VK_WHOLE_SIZE;
            }
        }
    }
    
    vkUpdateDescriptorSets(GetDevice()->GetVkDevice(), NumDescriptorWrites, DescriptorWrites, 0, nullptr);
}

void FVulkanDescriptorSetCache::SetUAVs(FVulkanUnorderedAccessViewCache& Cache, EShaderVisibility ShaderStage, uint32 NumUAVs)
{
    // TODO: We want to store this together with the PipelineLayout
    constexpr uint32 ImageBindingsStartIndex  = 32;
    constexpr uint32 BufferBindingsStartIndex = 8;
    constexpr uint32 NumDescriptorWrites      = VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT + VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT;

    VkDescriptorImageInfo  ImageInfos[VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT];
    VkDescriptorBufferInfo BufferInfos[VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT];
    VkWriteDescriptorSet   DescriptorWrites[NumDescriptorWrites];
    
    auto& UAVCache = Cache.ResourceViews[ShaderStage];
    for (uint32 Index = 0; Index < NumUAVs; Index++)
    {
        VkWriteDescriptorSet& CurrentDescriptorWrite = DescriptorWrites[Index];
        FMemory::Memzero(&CurrentDescriptorWrite);
        
        CurrentDescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        CurrentDescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        CurrentDescriptorWrite.dstSet          = DescriptorSets[ShaderStage];
        CurrentDescriptorWrite.descriptorCount = 1;
        CurrentDescriptorWrite.dstBinding      = ImageBindingsStartIndex + Index;
        
        VkDescriptorImageInfo& DescriptorInfo = ImageInfos[Index];
        CurrentDescriptorWrite.pImageInfo     = &DescriptorInfo;
        
        FVulkanUnorderedAccessView* UnorderedAccessView = UAVCache[Index];
        if (UnorderedAccessView && UnorderedAccessView->HasImageView())
        {
            DescriptorInfo.imageView   = UnorderedAccessView->GetVkImageView();
            DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            DescriptorInfo.sampler     = VK_NULL_HANDLE;
        }
        else
        {
            // If we have the NullDescriptors feature enable we don't create DefaultResources, so we just use the VK_NULL_HANDLE
            if (FVulkanRobustness2EXT::SupportsNullDescriptors())
            {
                DescriptorInfo.imageView   = VK_NULL_HANDLE;
                DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                DescriptorInfo.sampler     = VK_NULL_HANDLE;
            }
            else
            {
                DescriptorInfo.imageView   = DefaultResources.NullImageView;
                DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                DescriptorInfo.sampler     = VK_NULL_HANDLE;
            }
        }
    }
    
    
    for (uint32 Index = 0; Index < NumUAVs; Index++)
    {
        VkWriteDescriptorSet& CurrentDescriptorWrite = DescriptorWrites[VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT + Index];
        FMemory::Memzero(&CurrentDescriptorWrite);
        
        CurrentDescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        CurrentDescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        CurrentDescriptorWrite.dstSet          = DescriptorSets[ShaderStage];
        CurrentDescriptorWrite.descriptorCount = 1;
        CurrentDescriptorWrite.dstBinding      = BufferBindingsStartIndex + Index;
        
        VkDescriptorBufferInfo& DescriptorInfo = BufferInfos[Index];
        CurrentDescriptorWrite.pBufferInfo     = &DescriptorInfo;
        
        FVulkanUnorderedAccessView* UnorderedAccessView = UAVCache[Index];
        if (UnorderedAccessView && !UnorderedAccessView->HasImageView())
        {
            DescriptorInfo = UnorderedAccessView->GetDescriptorBufferInfo();
        }
        else
        {
            // If we have the NullDescriptors feature enable we don't create DefaultResources, so we just use the VK_NULL_HANDLE
            if (FVulkanRobustness2EXT::SupportsNullDescriptors())
            {
                DescriptorInfo.buffer = VK_NULL_HANDLE;
                DescriptorInfo.offset = 0;
                DescriptorInfo.range  = VK_WHOLE_SIZE;
            }
            else
            {
                DescriptorInfo.buffer = DefaultResources.NullBuffer;
                DescriptorInfo.offset = 0;
                DescriptorInfo.range  = VK_WHOLE_SIZE;
            }
        }
    }
    
    vkUpdateDescriptorSets(GetDevice()->GetVkDevice(), NumDescriptorWrites, DescriptorWrites, 0, nullptr);
}

void FVulkanDescriptorSetCache::SetConstantBuffers(FVulkanConstantBufferCache& Cache, EShaderVisibility ShaderStage, uint32 NumBuffers)
{
    // TODO: We want to store this together with the PipelineLayout
    constexpr uint32 BindingsStartIndex = 0;
    
    VkDescriptorBufferInfo BufferInfos[VULKAN_DEFAULT_CONSTANT_BUFFER_COUNT];
    VkWriteDescriptorSet   DescriptorWrites[VULKAN_DEFAULT_CONSTANT_BUFFER_COUNT];
    
    auto& ConstantBufferCache = Cache.ConstantBuffers[ShaderStage];
    for (uint32 Index = 0; Index < NumBuffers; Index++)
    {
        VkWriteDescriptorSet& CurrentDescriptorWrite = DescriptorWrites[Index];
        FMemory::Memzero(&CurrentDescriptorWrite);
        
        CurrentDescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        CurrentDescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        CurrentDescriptorWrite.dstSet          = DescriptorSets[ShaderStage];
        CurrentDescriptorWrite.descriptorCount = 1;
        CurrentDescriptorWrite.dstBinding      = BindingsStartIndex + Index;
        
        VkDescriptorBufferInfo& DescriptorInfo = BufferInfos[Index];
        CurrentDescriptorWrite.pBufferInfo     = &DescriptorInfo;
        
        if (FVulkanBuffer* ConstantBuffer = ConstantBufferCache[Index])
        {
            DescriptorInfo.buffer = ConstantBuffer->GetVkBuffer();
            DescriptorInfo.offset = 0;
            DescriptorInfo.range  = FMath::AlignUp<VkDeviceSize>(ConstantBuffer->GetSize(), ConstantBuffer->GetRequiredAlignment());
        }
        else
        {
            // If we have the NullDescriptors feature enable we don't create DefaultResources, so we just use the VK_NULL_HANDLE
            if (FVulkanRobustness2EXT::SupportsNullDescriptors())
            {
                DescriptorInfo.buffer = VK_NULL_HANDLE;
                DescriptorInfo.offset = 0;
                DescriptorInfo.range  = VK_WHOLE_SIZE;
            }
            else
            {
                DescriptorInfo.buffer = DefaultResources.NullBuffer;
                DescriptorInfo.offset = 0;
                DescriptorInfo.range  = VK_WHOLE_SIZE;
            }
        }
    }
    
    vkUpdateDescriptorSets(GetDevice()->GetVkDevice(), NumBuffers, DescriptorWrites, 0, nullptr);
}

void FVulkanDescriptorSetCache::SetSamplers(FVulkanSamplerStateCache& Cache, EShaderVisibility ShaderStage, uint32 NumSamplers)
{
    // TODO: We want to store this together with the PipelineLayout
    constexpr uint32 BindingsStartIndex = 24;
    
    VkDescriptorImageInfo ImageInfos[VULKAN_DEFAULT_SAMPLER_STATE_COUNT];
    VkWriteDescriptorSet  DescriptorWrites[VULKAN_DEFAULT_SAMPLER_STATE_COUNT];
    
    auto& SamplerCache = Cache.SamplerStates[ShaderStage];
    for (uint32 Index = 0; Index < NumSamplers; Index++)
    {
        VkWriteDescriptorSet& CurrentDescriptorWrite = DescriptorWrites[Index];
        FMemory::Memzero(&CurrentDescriptorWrite);
        
        CurrentDescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        CurrentDescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
        CurrentDescriptorWrite.dstSet          = DescriptorSets[ShaderStage];
        CurrentDescriptorWrite.descriptorCount = 1;
        CurrentDescriptorWrite.dstBinding      = BindingsStartIndex + Index;
        
        VkDescriptorImageInfo& DescriptorInfo = ImageInfos[Index];
        CurrentDescriptorWrite.pImageInfo     = &DescriptorInfo;
        
        if (FVulkanSamplerState* SamplerState = SamplerCache[Index])
        {
            DescriptorInfo.imageView   = VK_NULL_HANDLE;
            DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            DescriptorInfo.sampler     = SamplerState->GetVkSampler();
        }
        else
        {
            // It does not seem like samplers support NullDescriptors
            DescriptorInfo.imageView   = VK_NULL_HANDLE;
            DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            DescriptorInfo.sampler     = DefaultResources.NullSampler;
        }
    }
    
    vkUpdateDescriptorSets(GetDevice()->GetVkDevice(), NumSamplers, DescriptorWrites, 0, nullptr);
}

void FVulkanDescriptorSetCache::SetDescriptorSet(VkPipelineLayout PipelineLayout, EShaderVisibility ShaderStage)
{
    VkPipelineBindPoint BindPoint;
    uint32              DescriptorSetBindPoint;

    if (ShaderStage == ShaderVisibility_Compute)
    {
        BindPoint              = VK_PIPELINE_BIND_POINT_COMPUTE;
        DescriptorSetBindPoint = 0;
    }
    else
    {
        BindPoint              = VK_PIPELINE_BIND_POINT_GRAPHICS;
        DescriptorSetBindPoint = ShaderStage;
    }
    
    VkDescriptorSet& DescriptorSet = DescriptorSets[ShaderStage];
    CHECK(DescriptorSet != VK_NULL_HANDLE);
    Context.GetCommandBuffer().BindDescriptorSets(BindPoint, PipelineLayout, DescriptorSetBindPoint, 1, &DescriptorSet, 0, nullptr);
}

bool FVulkanDescriptorSetCache::AllocateDescriptorPool()
{
    if (VULKAN_CHECK_HANDLE(DescriptorPool))
    {
        PendingDescriptorPools.Add(DescriptorPool);
        DescriptorPool = VK_NULL_HANDLE;
    }
    
    if (!AvailableDescriptorPools.IsEmpty())
    {
        DescriptorPool = AvailableDescriptorPools.LastElement();
        AvailableDescriptorPools.Pop();
        
        if (DescriptorPool != VK_NULL_HANDLE)
        {
            return true;
        }
    }
    
    const VkDescriptorType DescriptorTypes[] =
    {
        // ConstantBuffers
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        // SRV + UAV Buffers
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        // Samplers
        VK_DESCRIPTOR_TYPE_SAMPLER,
        // UAV Textures
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        // Textures
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    };
    
    TArray<VkDescriptorPoolSize> PoolSizes;
    for (VkDescriptorType DescriptorType : DescriptorTypes)
    {
        VkDescriptorPoolSize NewPoolSize;
        NewPoolSize.type            = DescriptorType;
        NewPoolSize.descriptorCount = 1024;
        PoolSizes.Add(NewPoolSize);
    }
    
    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo;
    FMemory::Memzero(&DescriptorPoolCreateInfo);
    
    DescriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.maxSets       = 1024;
    DescriptorPoolCreateInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    DescriptorPoolCreateInfo.poolSizeCount = PoolSizes.Size();
    DescriptorPoolCreateInfo.pPoolSizes    = PoolSizes.Data();
    
    VkResult Result = vkCreateDescriptorPool(GetDevice()->GetVkDevice(), &DescriptorPoolCreateInfo, nullptr, &DescriptorPool);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create DescriptorPool");
        return false;
    }
    
    return true;
}
