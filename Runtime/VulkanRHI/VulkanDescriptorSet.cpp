#include "VulkanDescriptorSet.h"
#include "VulkanPipelineLayout.h"
#include "VulkanCommandBuffer.h"
#include "VulkanResourceViews.h"
#include "VulkanBuffer.h"
#include "VulkanSamplerState.h"

FVulkanDescriptorState::FVulkanDescriptorState(FVulkanDevice* InDevice, FVulkanPipelineLayout* InLayout, const FVulkanDefaultResources& InDefaultResources)
    : FVulkanDeviceChild(InDevice)
    , Layout(InLayout)
    , DescriptorSetHandles()
    , DescriptorSetBuilders()
    , DefaultResources(InDefaultResources)
{
    if (!Layout)
    {
        VULKAN_ERROR("PipelineLayout cannot be nullptr");
        DEBUG_BREAK();
        return;
    }

    // Allocate all the per-descriptorset resources
    const TArray<FVulkanDescriptorRemappingInfo>& RemappingInfos = Layout->GetDescriptorRemappingInfos();
    DescriptorSetHandles.Resize(RemappingInfos.Size());
    DescriptorSetWrites.Resize(RemappingInfos.Size());
    DescriptorSetBuilders.Resize(RemappingInfos.Size());
    
    // Pre-Initialize all the DescriptorWrites that are necessary
    for (int32 DescriptorSetIndex = 0; DescriptorSetIndex < RemappingInfos.Size(); DescriptorSetIndex++)
    {
        const FVulkanDescriptorRemappingInfo& SetRemappingInfo = RemappingInfos[DescriptorSetIndex];
        
        // Allocate DescriptorWrites
        FVulkanDescriptorWrites& DSWrites = DescriptorSetWrites[DescriptorSetIndex];
        DSWrites.DescriptorWrites.Resize(SetRemappingInfo.RemappingInfo.Size());
        FMemory::Memzero(DSWrites.DescriptorWrites.Data(), DSWrites.DescriptorWrites.SizeInBytes());
        
        // Init DescriptorWrites and count the other bindings
        uint32 NumImageInfos  = 0;
        uint32 NumBufferInfos = 0;
        for (int32 Index = 0; Index < SetRemappingInfo.RemappingInfo.Size(); Index++)
        {
            const FVulkanDescriptorRemappingInfo::FRemappingInfo& Binding = SetRemappingInfo.RemappingInfo[Index];
            CHECK(Binding.BindingIndex == static_cast<uint32>(Index));
            
            VkWriteDescriptorSet& WriteDescriptorSet = DSWrites.DescriptorWrites[Index];
            WriteDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            WriteDescriptorSet.descriptorType  = GetDescriptorTypeFromBindingType(Binding.BindingType);
            WriteDescriptorSet.descriptorCount = 1;
            WriteDescriptorSet.dstBinding      = Binding.BindingIndex;
            WriteDescriptorSet.dstArrayElement = 0;
            
            switch(WriteDescriptorSet.descriptorType)
            {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                {
                    NumBufferInfos++;
                    break;
                }
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    NumImageInfos++;
                    break;
                }
                default:
                {
                    VULKAN_ERROR("Unhandled DescriptorType");
                    DEBUG_BREAK();
                    break;
                }
            };
        }
        
        // Allocate Buffer and Image Infos
        DSWrites.DescriptorImageInfos.Resize(NumImageInfos);
        FMemory::Memzero(DSWrites.DescriptorImageInfos.Data(), DSWrites.DescriptorImageInfos.SizeInBytes());
        
        DSWrites.DescriptorBufferInfos.Resize(NumBufferInfos);
        FMemory::Memzero(DSWrites.DescriptorBufferInfos.Data(), DSWrites.DescriptorBufferInfos.SizeInBytes());
        
        // Setup Buffer and ImageInfos
        uint32 CurrentImageInfo  = 0;
        uint32 CurrentBufferInfo = 0;
        for (int32 Index = 0; Index < DSWrites.DescriptorWrites.Size(); Index++)
        {
            VkWriteDescriptorSet& WriteDescriptorSet = DSWrites.DescriptorWrites[Index];
            switch(WriteDescriptorSet.descriptorType)
            {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                {
                    WriteDescriptorSet.pBufferInfo = &DSWrites.DescriptorBufferInfos[CurrentBufferInfo++];
                    
                    // Initialize buffers to use a null-buffer
                    VkDescriptorBufferInfo* BufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptorSet.pBufferInfo);
                    BufferInfo->buffer = DefaultResources.NullBuffer;
                    BufferInfo->offset = 0;
                    BufferInfo->range  = VK_WHOLE_SIZE;
                    break;
                }
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    // Initialize textures to use a null-image
                    WriteDescriptorSet.pImageInfo = &DSWrites.DescriptorImageInfos[CurrentImageInfo++];
                    
                    VkDescriptorImageInfo* ImageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptorSet.pImageInfo);
                    ImageInfo->imageView   = DefaultResources.NullImageView;
                    ImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    ImageInfo->sampler     = VK_NULL_HANDLE;
                    break;
                }
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                {
                    // Initialize samplers to use a null-sampler
                    WriteDescriptorSet.pImageInfo = &DSWrites.DescriptorImageInfos[CurrentImageInfo++];
                    
                    VkDescriptorImageInfo* ImageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptorSet.pImageInfo);
                    ImageInfo->imageView   = VK_NULL_HANDLE;
                    ImageInfo->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    ImageInfo->sampler     = DefaultResources.NullSampler;
                    break;
                }
                default:
                {
                    VULKAN_ERROR("Unhandled DescriptorType");
                    DEBUG_BREAK();
                    break;
                }
            };
        }
        
        // Setup the builders
        DescriptorSetBuilders[DescriptorSetIndex].SetupDescriptorWrites(DSWrites.DescriptorWrites.Data(), DSWrites.DescriptorWrites.Size());
    }
}

void FVulkanDescriptorState::SetSRV(FVulkanShaderResourceView* ShaderResourceView, uint32 DescriptorSetIndex, uint32 BindingIndex)
{
    CHECK(DescriptorSetIndex < static_cast<uint32>(DescriptorSetBuilders.Size()));

    FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[DescriptorSetIndex];
    if (ShaderResourceView)
    {
        switch(ShaderResourceView->GetType())
        {
            case FVulkanShaderResourceView::EType::Texture:
            {
                DSBuilder.WriteSampledImage(BindingIndex, ShaderResourceView->GetVkImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                break; 
            }
            case FVulkanShaderResourceView::EType::Buffer:
            {
                const VkDescriptorBufferInfo& BufferInfo = ShaderResourceView->GetDescriptorBufferInfo();
                DSBuilder.WriteStorageBuffer(BindingIndex, BufferInfo.buffer, BufferInfo.offset, BufferInfo.range);
                break; 
            }
            default:
            {
                VULKAN_ERROR("Invalid ShaderResourveView, probably uninitialized resource");
                DEBUG_BREAK();
                break;
            }
        };
    }
    else
    {
        ResetDescriptorBinding(DescriptorSetIndex, BindingIndex);
    }
}

void FVulkanDescriptorState::SetUAV(FVulkanUnorderedAccessView* UnorderedAccessView, uint32 DescriptorSetIndex, uint32 BindingIndex)
{
    CHECK(DescriptorSetIndex < static_cast<uint32>(DescriptorSetBuilders.Size()));
    
    FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[DescriptorSetIndex];
    if (UnorderedAccessView)
    {
        switch(UnorderedAccessView->GetType())
        {
            case FVulkanUnorderedAccessView::EType::Texture:
            {
                DSBuilder.WriteStorageImage(BindingIndex, UnorderedAccessView->GetVkImageView(), VK_IMAGE_LAYOUT_GENERAL);
                break; 
            }
            case FVulkanUnorderedAccessView::EType::Buffer:
            {
                const VkDescriptorBufferInfo& BufferInfo = UnorderedAccessView->GetDescriptorBufferInfo();
                DSBuilder.WriteStorageBuffer(BindingIndex, BufferInfo.buffer, BufferInfo.offset, BufferInfo.range);
                break; 
            }
            default:
            {
                VULKAN_ERROR("Invalid ShaderResourveView, probably uninitialized resource");
                DEBUG_BREAK();
                break;
            }
        };
    }
    else
    {
        ResetDescriptorBinding(DescriptorSetIndex, BindingIndex);
    }
}

void FVulkanDescriptorState::SetUniform(FVulkanBuffer* UniformBuffer, uint32 DescriptorSetIndex, uint32 BindingIndex)
{
    CHECK(DescriptorSetIndex < static_cast<uint32>(DescriptorSetBuilders.Size()));

    if (UniformBuffer)
    {
        const VkDeviceSize Range = FMath::AlignUp<VkDeviceSize>(UniformBuffer->GetSize(), UniformBuffer->GetRequiredAlignment());
        FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[DescriptorSetIndex];
        DSBuilder.WriteUniformBuffer(BindingIndex, UniformBuffer->GetVkBuffer(), 0, Range);
    }
    else
    {
        ResetDescriptorBinding(DescriptorSetIndex, BindingIndex);
    }
}

void FVulkanDescriptorState::SetSampler(FVulkanSamplerState* SamplerState, uint32 DescriptorSetIndex, uint32 BindingIndex)
{
    CHECK(DescriptorSetIndex < static_cast<uint32>(DescriptorSetBuilders.Size()));

    if (SamplerState)
    {
        FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[DescriptorSetIndex];
        DSBuilder.WriteSampler(BindingIndex, SamplerState->GetVkSampler());
    }
    else
    {
        ResetDescriptorBinding(DescriptorSetIndex, BindingIndex);
    }
}

void FVulkanDescriptorState::UpdateDescriptorSets()
{
    for (int32 Index = 0; Index < DescriptorSetHandles.Size(); Index++)
    {
        const FVulkanDescriptorWrites& DSWrites = DescriptorSetWrites[Index];
        for (const VkWriteDescriptorSet& WriteInfo : DSWrites.DescriptorWrites)
        {
            if (WriteInfo.pBufferInfo)
            {
                CHECK(WriteInfo.pBufferInfo->buffer != DefaultResources.NullBuffer);
            }
            else if (WriteInfo.pImageInfo)
            {
                CHECK(WriteInfo.pImageInfo->sampler != DefaultResources.NullSampler);
                CHECK(WriteInfo.pImageInfo->imageView != DefaultResources.NullImageView);
            }
            else
            {
                DEBUG_BREAK();
            }
        }
        
        FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[Index];
        DSBuilder.UpdateHash();

        VkDescriptorSetLayout SetLayout = Layout->GetVkDescriptorSetLayout(Index);
        if (!GetDevice()->GetDescriptorSetCache().FindOrCreateDescriptorSet(SetLayout, DSBuilder, DescriptorSetHandles[Index]))
        {
            VULKAN_ERROR("Failed to find or create DescriptorSet");
            DEBUG_BREAK();
            return;
        }
    }
}

void FVulkanDescriptorState::Reset()
{
    for (int32 DescriptorSetIndex = 0; DescriptorSetIndex < DescriptorSetWrites.Size(); DescriptorSetIndex++)
    {
        FVulkanDescriptorWrites& DSWrites = DescriptorSetWrites[DescriptorSetIndex];
        for (int32 BindingIndex = 0; BindingIndex < DSWrites.DescriptorWrites.Size(); BindingIndex++)
        {
            ResetDescriptorBinding(DescriptorSetIndex, BindingIndex);
        }
    }
}

void FVulkanDescriptorState::BindDescriptorSets(class FVulkanCommandBuffer& CommandBuffer, VkPipelineBindPoint BindPoint)
{
    CHECK(DescriptorSetHandles.Size() > 0); // Cannot bind zero descriptorsets
    CommandBuffer->BindDescriptorSets(BindPoint, Layout->GetVkPipelineLayout(), 0, DescriptorSetHandles.Size(), DescriptorSetHandles.Data(), 0, nullptr);
}

void FVulkanDescriptorState::ResetDescriptorBinding(uint32 DescriptorSetIndex, uint32 BindingIndex)
{
    FVulkanDescriptorWrites&     DSWrites  = DescriptorSetWrites[DescriptorSetIndex];
    FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[DescriptorSetIndex];
    
    const VkDescriptorType Type = DSWrites.DescriptorWrites[BindingIndex].descriptorType;
    switch(Type)
    {
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            DSBuilder.WriteStorageBuffer(BindingIndex, DefaultResources.NullBuffer, 0, VK_WHOLE_SIZE);
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
            DSBuilder.WriteUniformBuffer(BindingIndex, DefaultResources.NullBuffer, 0, VK_WHOLE_SIZE);
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            DSBuilder.WriteStorageImage(BindingIndex, DefaultResources.NullImageView, VK_IMAGE_LAYOUT_GENERAL);
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            DSBuilder.WriteSampledImage(BindingIndex, DefaultResources.NullImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        {
            DSBuilder.WriteSampler(BindingIndex, DefaultResources.NullSampler);
            break;
        }
        default:
        {
            VULKAN_ERROR("Unhandled DescriptorType");
            DEBUG_BREAK();
            break;
        }
    }
}


FVulkanDescriptorPool::FVulkanDescriptorPool(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , DescriptorPool(VK_NULL_HANDLE)
{
}

FVulkanDescriptorPool::~FVulkanDescriptorPool()
{
    if (VULKAN_CHECK_HANDLE(DescriptorPool))
    {
        VkDevice VulkanDevice = GetDevice()->GetVkDevice();
        vkResetDescriptorPool(VulkanDevice, DescriptorPool, 0);
        vkDestroyDescriptorPool(VulkanDevice, DescriptorPool, nullptr);
        DescriptorPool = VK_NULL_HANDLE;
    }
}

bool FVulkanDescriptorPool::Initialize()
{
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
    DescriptorPoolCreateInfo.flags         = 0;
    DescriptorPoolCreateInfo.poolSizeCount = PoolSizes.Size();
    DescriptorPoolCreateInfo.pPoolSizes    = PoolSizes.Data();
    
    VkResult Result = vkCreateDescriptorPool(GetDevice()->GetVkDevice(), &DescriptorPoolCreateInfo, nullptr, &DescriptorPool);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create DescriptorPool");
        return false;
    }
    else
    {
        return true;
    }
}

bool FVulkanDescriptorPool::AllocateDescriptorSet(VkDescriptorSetLayout DescriptorSetLayout, VkDescriptorSet& OutDescriptorSet)
{
    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
    FMemory::Memzero(&DescriptorSetAllocateInfo);
    
    DescriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescriptorSetAllocateInfo.descriptorPool     = DescriptorPool;
    DescriptorSetAllocateInfo.descriptorSetCount = 1;
    DescriptorSetAllocateInfo.pSetLayouts        = &DescriptorSetLayout;
    
    // Initalize the DescriptorSet to Null
    OutDescriptorSet = VK_NULL_HANDLE;
    
    VkResult Result = vkAllocateDescriptorSets(GetDevice()->GetVkDevice(), &DescriptorSetAllocateInfo, &OutDescriptorSet);
    if (Result == VK_ERROR_OUT_OF_POOL_MEMORY)
    {
        return false;
    }
    
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to allocate descriptorset");
        return false;
    }
    else
    {
        return true;
    }
}

void FVulkanDescriptorPool::Reset()
{
    vkResetDescriptorPool(GetDevice()->GetVkDevice(), DescriptorPool, 0);
}


FVulkanDescriptorPoolManager::FVulkanDescriptorPoolManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , DescriptorPools()
    , DescriptorPoolsCS()
{
}

FVulkanDescriptorPoolManager::~FVulkanDescriptorPoolManager()
{
    ReleaseAll();
}

FVulkanDescriptorPool* FVulkanDescriptorPoolManager::ObtainPool()
{
    {
        SCOPED_LOCK(DescriptorPoolsCS);
        
        if (!DescriptorPools.IsEmpty())
        {
            FVulkanDescriptorPool* DescriptorPool = DescriptorPools.LastElement();
            DescriptorPools.Pop();

            // Reset the memory of the DescriptorPool
            DescriptorPool->Reset();
            return DescriptorPool;
        }
    }
    
    FVulkanDescriptorPool* DescriptorPool = new FVulkanDescriptorPool(GetDevice());
    if (!DescriptorPool->Initialize())
    {
        DEBUG_BREAK();
        return nullptr;
    }
    else
    {
        return DescriptorPool;
    }
}

void FVulkanDescriptorPoolManager::RecyclePool(FVulkanDescriptorPool* InDescriptorPool)
{
    if (InDescriptorPool)
    {
        SCOPED_LOCK(DescriptorPoolsCS);
        DescriptorPools.Add(InDescriptorPool);
    }
    else
    {
        LOG_WARNING("Trying to Recycle an invalid Fence");
    }
}

void FVulkanDescriptorPoolManager::ReleaseAll()
{
    SCOPED_LOCK(DescriptorPoolsCS);
    
    for (FVulkanDescriptorPool* DescriptorPool : DescriptorPools)
    {
        delete DescriptorPool;
    }
    
    DescriptorPools.Clear();
}
