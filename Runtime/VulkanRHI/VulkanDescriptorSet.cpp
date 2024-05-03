#include "VulkanDescriptorSet.h"
#include "VulkanPipelineLayout.h"
#include "VulkanCommandBuffer.h"
#include "VulkanResourceViews.h"
#include "VulkanBuffer.h"
#include "VulkanSamplerState.h"
#include "VulkanDevice.h"
#include "Core/Misc/ConsoleManager.h"

#define VALIDATE_NO_NULL_DESCRIPTORS (0)

static TAutoConsoleVariable<int32> CVarVulkanMaxDescriptorSetsPerPool(
    "VulkanRHI.MaxDescriptorSetsPerPool",
    "The number of DescriptorSets that can be created from a DescriptorPool",
    32);

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
        return;
    }

    // Allocate all the per DescriptorSet resources
    const TArray<FVulkanDescriptorRemappingInfo>& RemappingInfos = Layout->GetDescriptorRemappingInfos();
    DescriptorSetHandles.Resize(RemappingInfos.Size());
    DescriptorSetWrites.Resize(RemappingInfos.Size());
    DescriptorSetBuilders.Resize(RemappingInfos.Size());
    DescriptorPoolInfos.Reserve(DescriptorSetHandles.Size());
    
    // Maps from a descriptor-type to the number of descriptors for this type
    TMap<VkDescriptorType, uint32> DescriptorCountMap;

    // Pre-Initialize all the DescriptorWrites that are necessary
    for (int32 DescriptorSetIndex = 0; DescriptorSetIndex < RemappingInfos.Size(); DescriptorSetIndex++)
    {
        DescriptorCountMap.Clear();

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
                    break;
                }
            };

            DescriptorCountMap[WriteDescriptorSet.descriptorType]++;
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
                    break;
                }
            };
        }
        
        // Fill in all the info we need to allocate DescriptorSets from a DescriptorPool
        FVulkanDescriptorPoolInfo PoolInfo;
        PoolInfo.DescriptorSetLayout = Layout->GetVkDescriptorSetLayout(DescriptorSetIndex);

        // Setup the builders
        DescriptorSetBuilders[DescriptorSetIndex].SetupDescriptorWrites(PoolInfo.DescriptorSetLayout, DSWrites.DescriptorWrites.Data(), DSWrites.DescriptorWrites.Size());

        for (auto TypePair : DescriptorCountMap)
        {
            PoolInfo.DescriptorSizes.Emplace(TypePair.First, TypePair.Second);
        }

        PoolInfo.GenerateHash();
        DescriptorPoolInfos.Add(Move(PoolInfo));
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
            case FVulkanResourceView::EType::ImageView:
            {
                const FVulkanResourceView::FImageView& ImageViewInfo = ShaderResourceView->GetImageViewInfo();
                DSBuilder.WriteSampledImage(BindingIndex, ImageViewInfo.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                break; 
            }
            case FVulkanResourceView::EType::StructuredBufferView:
            {
                const FVulkanResourceView::FStructuredBufferView& StructuredBufferView = ShaderResourceView->GetStructuredBufferInfo();
                DSBuilder.WriteStorageBuffer(BindingIndex, StructuredBufferView.Buffer, StructuredBufferView.Offset, StructuredBufferView.Range);
                break; 
            }
            default:
            {
                VULKAN_ERROR("Invalid ShaderResourveView, probably uninitialized resource");
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
            case FVulkanResourceView::EType::ImageView:
            {
                const FVulkanResourceView::FImageView& ImageViewInfo = UnorderedAccessView->GetImageViewInfo();
                DSBuilder.WriteStorageImage(BindingIndex, ImageViewInfo.ImageView, VK_IMAGE_LAYOUT_GENERAL);
                break;
            }
            case FVulkanResourceView::EType::StructuredBufferView:
            {
                const FVulkanResourceView::FStructuredBufferView& StructuredBufferView = UnorderedAccessView->GetStructuredBufferInfo();
                DSBuilder.WriteStorageBuffer(BindingIndex, StructuredBufferView.Buffer, StructuredBufferView.Offset, StructuredBufferView.Range);
                break;
            }
            default:
            {
                VULKAN_ERROR("Invalid ShaderResourveView, probably uninitialized resource");
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
    #if VALIDATE_NO_NULL_DESCRIPTORS
        const FVulkanDescriptorWrites& DSWrites = DescriptorSetWrites[Index];
        for (const VkWriteDescriptorSet& WriteInfo : DSWrites.DescriptorWrites)
        {
            if (WriteInfo.pBufferInfo)
            {
                CHECK(WriteInfo.pBufferInfo->buffer != DefaultResources.NullBuffer);
            }
            else if (WriteInfo.pImageInfo)
            {
                if (WriteInfo.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || WriteInfo.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                {
                    CHECK(WriteInfo.pImageInfo->imageView != DefaultResources.NullImageView);
                }
                else if (WriteInfo.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
                {
                    CHECK(WriteInfo.pImageInfo->sampler != DefaultResources.NullSampler);
                }
            }
            else
            {
                DEBUG_BREAK();
            }
        }
     #endif

        FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[Index];
        DSBuilder.UpdateHash();

        const FVulkanDescriptorPoolInfo& DescriptorPoolInfo = DescriptorPoolInfos[Index];
        FVulkanDescriptorSetCache& DescriptorSetCache = GetDevice()->GetDescriptorSetCache();
        if (!DescriptorSetCache.FindOrCreateDescriptorSet(DescriptorPoolInfo, DSBuilder, DescriptorSetHandles[Index]))
        {
            VULKAN_ERROR("Failed to find or create DescriptorSet");
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
    CHECK(DescriptorSetHandles.Size() > 0); // Cannot bind zero DescriptorSets
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
            break;
        }
    }
}

FVulkanDescriptorPool::FVulkanDescriptorPool(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , DescriptorPool(VK_NULL_HANDLE)
    , MaxDescriptorSets(0)
    , NumDescriptorSets(0)
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

bool FVulkanDescriptorPool::Initialize(const FVulkanDescriptorPoolInfo& PoolInfo)
{
    const uint32 MaxDescriptorSetsPerPool = FMath::Max<int32>(CVarVulkanMaxDescriptorSetsPerPool.GetValue(), 1);

    TArray<VkDescriptorPoolSize> PoolSizes;
    for (const FVulkanDescriptorPoolInfo::FDescriptorSize& Size : PoolInfo.DescriptorSizes)
    {
        VkDescriptorPoolSize NewPoolSize;
        NewPoolSize.type            = static_cast<VkDescriptorType>(Size.Type);
        NewPoolSize.descriptorCount = Size.NumDescriptors * MaxDescriptorSetsPerPool;
        PoolSizes.Add(NewPoolSize);
    }

    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo;
    FMemory::Memzero(&DescriptorPoolCreateInfo);

    DescriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.maxSets       = MaxDescriptorSetsPerPool;
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
        NumDescriptorSets = MaxDescriptorSets = MaxDescriptorSetsPerPool;
        return true;
    }
}

bool FVulkanDescriptorPool::AllocateDescriptorSet(const VkDescriptorSetAllocateInfo& DescriptorSetAllocateInfo, VkDescriptorSet* OutDescriptorSets)
{
    CHECK(DescriptorSetAllocateInfo.sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
    CHECK(DescriptorSetAllocateInfo.pNext == nullptr);

    VkDescriptorSetAllocateInfo AllocateInfo = DescriptorSetAllocateInfo;
    AllocateInfo.descriptorPool = DescriptorPool;

    VkResult Result = vkAllocateDescriptorSets(GetDevice()->GetVkDevice(), &AllocateInfo, OutDescriptorSets);
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
        NumDescriptorSets--;
        return true;
    }
}

void FVulkanDescriptorPool::Reset()
{
    vkResetDescriptorPool(GetDevice()->GetVkDevice(), DescriptorPool, 0);
    NumDescriptorSets = MaxDescriptorSets;
}

FVulkanDescriptorSetCache::FCachedPool::FCachedPool(FVulkanDevice* InDevice, const FVulkanDescriptorPoolInfo& InPoolInfo)
    : FVulkanDeviceChild(InDevice)
    , CurrentDescriptorPool(nullptr)
    , DescriptorPools()
    , PoolInfo(InPoolInfo)
{
}

FVulkanDescriptorSetCache::FCachedPool::~FCachedPool()
{
    SAFE_DELETE(CurrentDescriptorPool);

    // TODO: Look into putting the pools in the deferred deletion queue
    for (FVulkanDescriptorPool* DescriptorPool : DescriptorPools)
    {
        delete DescriptorPool;
    }

    DescriptorPools.Clear();
}

bool FVulkanDescriptorSetCache::FCachedPool::AllocateDescriptorSet(VkDescriptorSetLayout SetLayout, VkDescriptorSet& OutDescriptorSet)
{
    VkDescriptorSetAllocateInfo AllocateInfo;
    FMemory::Memzero(&AllocateInfo, sizeof(VkDescriptorSetAllocateInfo));

    AllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocateInfo.pSetLayouts        = &SetLayout;
    AllocateInfo.descriptorSetCount = 1;

    if (CurrentDescriptorPool)
    {
        if (CurrentDescriptorPool->CanAllocateDescriptorSet())
        {
            return CurrentDescriptorPool->AllocateDescriptorSet(AllocateInfo, &OutDescriptorSet);
        }

        DescriptorPools.Add(CurrentDescriptorPool);
    }

    CurrentDescriptorPool = new FVulkanDescriptorPool(GetDevice());
    if (!CurrentDescriptorPool->Initialize(PoolInfo))
    {
        return false;
    }

    return CurrentDescriptorPool->AllocateDescriptorSet(AllocateInfo, &OutDescriptorSet);
}

FVulkanDescriptorSetCache::FVulkanDescriptorSetCache(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Caches()
    , DescriptorSets()
    , CacheCS()
{
}

FVulkanDescriptorSetCache::~FVulkanDescriptorSetCache()
{
    Release();
}

void FVulkanDescriptorSetCache::ReleaseDescriptorSets()
{
    TScopedLock Lock(CacheCS);
    DescriptorSets.Clear();
}

void FVulkanDescriptorSetCache::Release()
{
    TScopedLock Lock(CacheCS);
    DescriptorSets.Clear();

    // Destroy all cached-pools
    for (auto CachedPools : Caches)
    {
        delete CachedPools.Second;
    }

    Caches.Clear();
}

bool FVulkanDescriptorSetCache::FindOrCreateDescriptorSet(const FVulkanDescriptorPoolInfo& PoolInfo, FVulkanDescriptorSetBuilder& DSBuilder, VkDescriptorSet& OutDescriptorSet)
{
    TScopedLock Lock(CacheCS);

    // Get or Create a DescriptorSet
    const FVulkanDescriptorSetKey& DSKey = DSBuilder.GetKey();
    if (VkDescriptorSet* DescriptorSet = DescriptorSets.Find(DSKey))
    {
        OutDescriptorSet = *DescriptorSet;
    }
    else
    {
        FCachedPool* CachedPool = nullptr;
        if (FCachedPool** ExistingPool = Caches.Find(PoolInfo))
        {
            CachedPool = *ExistingPool;
        }
        else
        {
            FCachedPool* NewPool = new FCachedPool(GetDevice(), PoolInfo);
            CachedPool = Caches.Add(Move(PoolInfo), NewPool);
        }

        if (!CachedPool)
        {
            DEBUG_BREAK();
            return false;
        }

        // Create a new DescriptorSet
        if (!CachedPool->AllocateDescriptorSet(PoolInfo.DescriptorSetLayout, OutDescriptorSet))
        {
            return false;
        }

        CHECK(OutDescriptorSet != VK_NULL_HANDLE);
        DescriptorSets.Add(DSKey, OutDescriptorSet);

        DSBuilder.SetDescriptorSet(OutDescriptorSet);
        DSBuilder.UpdateDescriptorSet(GetDevice()->GetVkDevice());
    }

    return true;
}
