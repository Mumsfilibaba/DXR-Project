#include "Core/Misc/ConsoleManager.h"
#include "Core/Math/Math.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanPipelineLayout.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanSamplerState.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanRHI.h"

#define VALIDATE_NO_NULL_DESCRIPTORS (0)
#define BREAK_ON_NULL_DESCRIPTORS (0)

static TAutoConsoleVariable<int32> CVarVulkanMaxDescriptorSetsPerPool(
    "VulkanRHI.MaxDescriptorSetsPerPool",
    "The number of DescriptorSets that can be created from a DescriptorPool",
    32);

static TAutoConsoleVariable<bool> CVarVulkanUseDescriptorCache(
    "VulkanRHI.UseDescriptorCache",
    "Enable descriptor set caching (false = transient per-frame allocation, true = cached)",
    false);

static TAutoConsoleVariable<int32> CVarVulkanTransientDescriptorSetsPerPool(
    "VulkanRHI.TransientDescriptorSetsPerPool",
    "The number of DescriptorSets per pool when using transient (non-cached) descriptor allocation",
    256);

FVulkanDescriptorState::FVulkanDescriptorState(FVulkanDevice* InDevice, FVulkanPipelineLayout* InLayout, const FVulkanDefaultResources& InDefaultResources)
    : FVulkanDeviceChild(InDevice)
    , Layout(InLayout)
    , DescriptorSetHandles()
    , DescriptorSetBuilders()
    , DefaultResources(InDefaultResources)
    , DescriptorSetVersion(0)
    , bDynamicOffsetsDirty(false)
{
    if (!Layout)
    {
        VULKAN_ERROR_CRITICAL("PipelineLayout cannot be nullptr");
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
        uint32 NumImageInfos       = 0;
        uint32 NumBufferInfos      = 0;
        uint32 NumTexelBufferViews = 0;

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
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
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
                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                {
                    NumTexelBufferViews++;
                    break;
                }
                default:
                {
                    VULKAN_ERROR_CRITICAL("Unhandled DescriptorType");
                    break;
                }
            };

            DescriptorCountMap[WriteDescriptorSet.descriptorType]++;
        }

        // Allocate Buffer, Image, and TexelBufferView Infos
        DSWrites.DescriptorImageInfos.Resize(NumImageInfos);
        FMemory::Memzero(DSWrites.DescriptorImageInfos.Data(), DSWrites.DescriptorImageInfos.SizeInBytes());

        DSWrites.DescriptorBufferInfos.Resize(NumBufferInfos);
        FMemory::Memzero(DSWrites.DescriptorBufferInfos.Data(), DSWrites.DescriptorBufferInfos.SizeInBytes());

        DSWrites.DescriptorTexelBufferViews.Resize(NumTexelBufferViews);
        FMemory::Memzero(DSWrites.DescriptorTexelBufferViews.Data(), DSWrites.DescriptorTexelBufferViews.SizeInBytes());

        // Setup Buffer, Image, and TexelBufferView Infos
        uint32 CurrentImageInfo       = 0;
        uint32 CurrentBufferInfo      = 0;
        uint32 CurrentTexelBufferView = 0;

        for (int32 Index = 0; Index < DSWrites.DescriptorWrites.Size(); Index++)
        {
            VkWriteDescriptorSet& WriteDescriptorSet = DSWrites.DescriptorWrites[Index];
            switch(WriteDescriptorSet.descriptorType)
            {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                {
                    WriteDescriptorSet.pBufferInfo = &DSWrites.DescriptorBufferInfos[CurrentBufferInfo++];
                    
                    VkDescriptorBufferInfo* BufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptorSet.pBufferInfo);
                    BufferInfo->buffer = DefaultResources.NullBuffer;
                    BufferInfo->offset = 0;
                    BufferInfo->range  = VK_WHOLE_SIZE;
                    break;
                }
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    WriteDescriptorSet.pImageInfo = &DSWrites.DescriptorImageInfos[CurrentImageInfo++];
                    
                    VkDescriptorImageInfo* ImageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptorSet.pImageInfo);
                    ImageInfo->imageView   = DefaultResources.NullImageView;
                    ImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    ImageInfo->sampler     = VK_NULL_HANDLE;
                    break;
                }
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                {
                    WriteDescriptorSet.pImageInfo = &DSWrites.DescriptorImageInfos[CurrentImageInfo++];
                    
                    VkDescriptorImageInfo* ImageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptorSet.pImageInfo);
                    ImageInfo->imageView   = VK_NULL_HANDLE;
                    ImageInfo->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    ImageInfo->sampler     = DefaultResources.NullSampler;
                    break;
                }
                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                {
                    WriteDescriptorSet.pTexelBufferView = &DSWrites.DescriptorTexelBufferViews[CurrentTexelBufferView++];
                    break;
                }
                default:
                {
                    VULKAN_ERROR_CRITICAL("Unhandled DescriptorType");
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

    // Initialize dynamic offset tracking as a single flat array
    const uint32 TotalDynamic = Layout->GetTotalDynamicOffsetCount();
    DynamicOffsets.Resize(TotalDynamic);
    FMemory::Memzero(DynamicOffsets.Data(), DynamicOffsets.SizeInBytes());

    DynamicOffsetBasePerSet.Resize(RemappingInfos.Size());
    BindingToDynamicIndex.Resize(RemappingInfos.Size());

    uint32 FlatBase = 0;
    for (int32 SetIndex = 0; SetIndex < RemappingInfos.Size(); SetIndex++)
    {
        const uint32 NumDynamic  = Layout->GetDynamicOffsetCount(SetIndex);
        const int32  NumBindings = RemappingInfos[SetIndex].RemappingInfo.Size();

        DynamicOffsetBasePerSet[SetIndex] = FlatBase;
        BindingToDynamicIndex[SetIndex].Resize(NumBindings);

        int32 DynamicSlot = 0;
        for (int32 BindingIndex = 0; BindingIndex < NumBindings; BindingIndex++)
        {
            const VkDescriptorType Type = DescriptorSetWrites[SetIndex].DescriptorWrites[BindingIndex].descriptorType;
            if (Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || Type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
            {
                BindingToDynamicIndex[SetIndex][BindingIndex] = DynamicSlot++;
            }
            else
            {
                BindingToDynamicIndex[SetIndex][BindingIndex] = -1;
            }
        }

        CHECK(static_cast<uint32>(DynamicSlot) == NumDynamic);
        FlatBase += NumDynamic;
    }

    CHECK(FlatBase == TotalDynamic);
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
            case FVulkanResourceView::EType::TypedBufferView:
            {
                const FVulkanResourceView::FTypedBufferView& TypedBufferView = ShaderResourceView->GetTypedBufferInfo();
                DSBuilder.WriteUniformTexelBuffer(BindingIndex, TypedBufferView.BufferView);
                break;
            }
            default:
            {
                VULKAN_ERROR_CRITICAL("Invalid ShaderResourveView, probably uninitialized resource");
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
            case FVulkanResourceView::EType::TypedBufferView:
            {
                const FVulkanResourceView::FTypedBufferView& TypedBufferView = UnorderedAccessView->GetTypedBufferInfo();
                DSBuilder.WriteStorageTexelBuffer(BindingIndex, TypedBufferView.BufferView);
                break;
            }
            default:
            {
                VULKAN_ERROR_CRITICAL("Invalid ShaderResourveView, probably uninitialized resource");
                break;
            }
        };
    }
    else
    {
        ResetDescriptorBinding(DescriptorSetIndex, BindingIndex);
    }
}

void FVulkanDescriptorState::SetUniformBuffer(FVulkanBuffer* UniformBuffer, uint32 DescriptorSetIndex, uint32 BindingIndex)
{
    CHECK(DescriptorSetIndex < static_cast<uint32>(DescriptorSetBuilders.Size()));

    if (UniformBuffer)
    {
        const VkBuffer     Buffer = UniformBuffer->GetBindVkBuffer();
        const VkDeviceSize Offset = UniformBuffer->GetBindOffset();
        const VkDeviceSize Range  = UniformBuffer->GetBindRange();

        FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[DescriptorSetIndex];
        const int32 DynamicIndex = BindingToDynamicIndex[DescriptorSetIndex][BindingIndex];
        if (DynamicIndex >= 0)
        {
            DSBuilder.WriteDynamicUniformBuffer(BindingIndex, Buffer, Range);

            const uint32 FlatIndex     = DynamicOffsetBasePerSet[DescriptorSetIndex] + DynamicIndex;
            const uint32 DynamicOffset = static_cast<uint32>(Offset);
            if (DynamicOffsets[FlatIndex] != DynamicOffset)
            {
                DynamicOffsets[FlatIndex] = DynamicOffset;
                bDynamicOffsetsDirty = true;
            }
        }
        else
        {
            DSBuilder.WriteUniformBuffer(BindingIndex, Buffer, Offset, Range);
        }
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

#if VALIDATE_NO_NULL_DESCRIPTORS
static const CHAR* GetDescriptorTypeName(VkDescriptorType Type)
{
    switch (Type)
    {
        case VK_DESCRIPTOR_TYPE_SAMPLER:                return "SAMPLER";
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:          return "SAMPLED_IMAGE";
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:          return "STORAGE_IMAGE";
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:         return "UNIFORM_BUFFER";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:         return "STORAGE_BUFFER";
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return "UNIFORM_BUFFER_DYNAMIC";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return "STORAGE_BUFFER_DYNAMIC";
        default:                                        return "UNKNOWN";
    }
}
#endif

void FVulkanDescriptorState::UpdateDescriptorSets(FVulkanTransientDescriptorAllocator* TransientAllocator)
{
    for (int32 Index = 0; Index < DescriptorSetHandles.Size(); Index++)
    {
    #if VALIDATE_NO_NULL_DESCRIPTORS
        const FVulkanDescriptorWrites& DSWrites = DescriptorSetWrites[Index];
        const FVulkanDescriptorRemappingInfo& RemappingInfo = Layout->GetDescriptorRemappingInfo(Index);
        for (int32 BindIdx = 0; BindIdx < DSWrites.DescriptorWrites.Size(); BindIdx++)
        {
            const VkWriteDescriptorSet& WriteInfo = DSWrites.DescriptorWrites[BindIdx];
            const uint16 OriginalBinding = (BindIdx < RemappingInfo.RemappingInfo.Size()) ? RemappingInfo.RemappingInfo[BindIdx].OriginalBindingIndex : 0;

        #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
            const CHAR* BindingName = Layout->GetBindingDebugName(Index, BindIdx);
        #else
            const CHAR* BindingName = "";
        #endif

            if (WriteInfo.pBufferInfo)
            {
                if (WriteInfo.pBufferInfo->buffer == DefaultResources.NullBuffer)
                {
                    VULKAN_WARNING("Null buffer descriptor '%s' (register b%u) at set=%d binding=%u (%s)",
                        BindingName, OriginalBinding, Index, WriteInfo.dstBinding, GetDescriptorTypeName(WriteInfo.descriptorType));
                #if BREAK_ON_NULL_DESCRIPTORS
                    DEBUG_BREAK();
                #endif
                }
            }
            else if (WriteInfo.pImageInfo)
            {
                if (WriteInfo.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || WriteInfo.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                {
                    if (WriteInfo.pImageInfo->imageView == DefaultResources.NullImageView)
                    {
                        VULKAN_WARNING("Null image view descriptor '%s' (register t%u) at set=%d binding=%u (%s)",
                            BindingName, OriginalBinding, Index, WriteInfo.dstBinding, GetDescriptorTypeName(WriteInfo.descriptorType));
                    #if BREAK_ON_NULL_DESCRIPTORS
                        DEBUG_BREAK();
                    #endif
                    }
                }
                else if (WriteInfo.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
                {
                    if (WriteInfo.pImageInfo->sampler == DefaultResources.NullSampler)
                    {
                        VULKAN_WARNING("Null sampler descriptor '%s' (register s%u) at set=%d binding=%u",
                            BindingName, OriginalBinding, Index, WriteInfo.dstBinding);
                    #if BREAK_ON_NULL_DESCRIPTORS
                        DEBUG_BREAK();
                    #endif
                    }
                }
            }
        }
     #endif

        FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[Index];

        if (GVulkanUseDescriptorCache)
        {
            DSBuilder.UpdateHash();

            // Must run every draw even when not dirty. The cache hit updates LastUsedFrame,
            // preventing the eviction logic from reclaiming the entry and recycling its pool.
            
            FVulkanDescriptorSetCache& DescriptorSetCache = GetDevice()->GetDescriptorSetCache();
            if (!DescriptorSetCache.FindOrCreateDescriptorSet(DescriptorPoolInfos[Index], DSBuilder, DescriptorSetHandles[Index]))
            {
                VULKAN_ERROR_CRITICAL("Failed to find or create DescriptorSet");
                return;
            }
        }
        else
        {
            CHECK(TransientAllocator != nullptr);

            if (TransientAllocator->GetDescriptorSetVersion() != DescriptorSetVersion)
            {
                DescriptorSetVersion = TransientAllocator->GetDescriptorSetVersion();
                FMemory::Memzero(DescriptorSetHandles.Data(), DescriptorSetHandles.SizeInBytes());
            }

            if (!DSBuilder.IsKeyDirty() && DescriptorSetHandles[Index] != VK_NULL_HANDLE)
            {
                continue;
            }

            if (!TransientAllocator->AllocateDescriptorSet(DescriptorPoolInfos[Index], DSBuilder, DescriptorSetHandles[Index]))
            {
                VULKAN_ERROR_CRITICAL("Failed to allocate transient DescriptorSet");
                return;
            }
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
    CHECK(DescriptorSetHandles.Size() > 0);
    CommandBuffer->BindDescriptorSets(BindPoint, Layout->GetVkPipelineLayout(), 0, DescriptorSetHandles.Size(), DescriptorSetHandles.Data(), DynamicOffsets.Size(), DynamicOffsets.Data());
    bDynamicOffsetsDirty = false;
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
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        {
            DSBuilder.WriteDynamicUniformBuffer(BindingIndex, DefaultResources.NullBuffer, VK_WHOLE_SIZE);

            const int32 DynamicIndex = BindingToDynamicIndex[DescriptorSetIndex][BindingIndex];
            if (DynamicIndex >= 0)
            {
                const uint32 FlatIndex = DynamicOffsetBasePerSet[DescriptorSetIndex] + DynamicIndex;
                if (DynamicOffsets[FlatIndex] != 0)
                {
                    DynamicOffsets[FlatIndex] = 0;
                    bDynamicOffsetsDirty = true;
                }
            }
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
            VULKAN_ERROR_CRITICAL("Unhandled DescriptorType");
            break;
        }
    }
}

FVulkanDescriptorPool::FVulkanDescriptorPool(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , PoolInfo()
    , DescriptorPool(VK_NULL_HANDLE)
    , MaxDescriptorSets(0)
    , NumDescriptorSets(0)
    , LiveDescriptorSets(0)
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

bool FVulkanDescriptorPool::Initialize(const FVulkanDescriptorPoolInfo& InPoolInfo, int32 MaxSetsPerPool)
{
    PoolInfo = InPoolInfo;

    TArray<VkDescriptorPoolSize> PoolSizes;
    for (const FVulkanDescriptorPoolInfo::FDescriptorSize& Size : InPoolInfo.DescriptorSizes)
    {
        VkDescriptorPoolSize NewPoolSize;
        NewPoolSize.type            = static_cast<VkDescriptorType>(Size.Type);
        NewPoolSize.descriptorCount = Size.NumDescriptors * MaxSetsPerPool;
        PoolSizes.Add(NewPoolSize);
    }

    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
    DescriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.maxSets       = MaxSetsPerPool;
    DescriptorPoolCreateInfo.flags         = 0;
    DescriptorPoolCreateInfo.poolSizeCount = PoolSizes.Size();
    DescriptorPoolCreateInfo.pPoolSizes    = PoolSizes.Data();

    VkResult Result = vkCreateDescriptorPool(GetDevice()->GetVkDevice(), &DescriptorPoolCreateInfo, nullptr, &DescriptorPool);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create DescriptorPool");
        return false;
    }
    else
    {
        NumDescriptorSets = MaxDescriptorSets = MaxSetsPerPool;
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
        VULKAN_ERROR_CRITICAL("Failed to allocate descriptorset");
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
    CHECK(LiveDescriptorSets == 0);
    vkResetDescriptorPool(GetDevice()->GetVkDevice(), DescriptorPool, 0);
    NumDescriptorSets = MaxDescriptorSets;
}

FVulkanDescriptorPoolManager::FVulkanDescriptorPoolManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , FreePools()
    , PoolCS()
    , CurrentFrame(0)
{
}

FVulkanDescriptorPoolManager::~FVulkanDescriptorPoolManager()
{
    TScopedLock Lock(PoolCS);
    for (auto Entry : FreePools)
    {
        for (FFreePool& FreePool : Entry.Second)
        {
            delete FreePool.Pool;
        }
    }

    FreePools.Clear();
}

FVulkanDescriptorPool* FVulkanDescriptorPoolManager::AcquirePool(const FVulkanDescriptorPoolInfo& PoolInfo)
{
    TScopedLock Lock(PoolCS);

    if (TArray<FFreePool>* FreeList = FreePools.Find(PoolInfo))
    {
        if (!FreeList->IsEmpty())
        {
            FFreePool Entry = FreeList->LastElement();
            FreeList->RemoveAt(FreeList->Size() - 1);
            return Entry.Pool;
        }
    }

    FVulkanDescriptorPool* NewPool = new FVulkanDescriptorPool(GetDevice());
    if (!NewPool->Initialize(PoolInfo, Math::Max<int32>(CVarVulkanTransientDescriptorSetsPerPool.GetValue(), 1)))
    {
        delete NewPool;
        return nullptr;
    }

    return NewPool;
}

void FVulkanDescriptorPoolManager::ReleasePool(FVulkanDescriptorPool* Pool)
{
    CHECK(Pool != nullptr);

    TScopedLock Lock(PoolCS);

    Pool->Reset();

    const FVulkanDescriptorPoolInfo& PoolInfo = Pool->GetPoolInfo();

    FFreePool Entry;
    Entry.Pool          = Pool;
    Entry.ReturnedFrame = CurrentFrame;

    if (TArray<FFreePool>* FreeList = FreePools.Find(PoolInfo))
    {
        FreeList->Add(Entry);
    }
    else
    {
        TArray<FFreePool> NewList;
        NewList.Add(Entry);
        FreePools.Add(PoolInfo, Move(NewList));
    }
}

void FVulkanDescriptorPoolManager::EvictUnusedPools()
{
    TScopedLock Lock(PoolCS);
    CurrentFrame++;

    for (auto Entry : FreePools)
    {
        TArray<FFreePool>& FreeList = Entry.Second;
        for (int32 Index = FreeList.Size() - 1; Index >= 0; --Index)
        {
            if ((CurrentFrame - FreeList[Index].ReturnedFrame) > MinUnusedFrames)
            {
                delete FreeList[Index].Pool;
                FreeList.RemoveAtSwap(Index);
            }
        }
    }
}

FVulkanTransientDescriptorAllocator::FVulkanTransientDescriptorAllocator(FVulkanDevice* InDevice, FVulkanDescriptorPoolManager& InPoolManager)
    : FVulkanDeviceChild(InDevice)
    , PoolManager(InPoolManager)
    , PoolSets()
    , DescriptorSetVersion(0)
{
}

FVulkanTransientDescriptorAllocator::~FVulkanTransientDescriptorAllocator()
{
    for (auto Entry : PoolSets)
    {
        FPoolSet& Set = Entry.Second;
        if (Set.ActivePool)
        {
            PoolManager.ReleasePool(Set.ActivePool);
        }

        for (FVulkanDescriptorPool* Pool : Set.UsedPools)
        {
            PoolManager.ReleasePool(Pool);
        }
    }

    PoolSets.Clear();
}

bool FVulkanTransientDescriptorAllocator::AllocateDescriptorSet(const FVulkanDescriptorPoolInfo& PoolInfo, FVulkanDescriptorSetBuilder& DSBuilder, VkDescriptorSet& OutDescriptorSet)
{
    FPoolSet* Set = PoolSets.Find(PoolInfo);
    if (!Set)
    {
        FPoolSet NewSet;
        Set = &PoolSets.Add(PoolInfo, Move(NewSet));
    }

    VkDescriptorSetAllocateInfo AllocateInfo = {};
    AllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocateInfo.pSetLayouts        = &PoolInfo.DescriptorSetLayout;
    AllocateInfo.descriptorSetCount = 1;

    bool bAllocated = false;
    if (Set->ActivePool && Set->ActivePool->CanAllocateDescriptorSet())
    {
        bAllocated = Set->ActivePool->AllocateDescriptorSet(AllocateInfo, &OutDescriptorSet);
    }
    else
    {
        if (Set->ActivePool)
        {
            Set->UsedPools.Add(Set->ActivePool);
            Set->ActivePool = nullptr;
        }

        Set->ActivePool = PoolManager.AcquirePool(PoolInfo);
        if (!Set->ActivePool)
        {
            return false;
        }

        bAllocated = Set->ActivePool->AllocateDescriptorSet(AllocateInfo, &OutDescriptorSet);
    }

    if (bAllocated)
    {
        DSBuilder.SetDescriptorSet(OutDescriptorSet);
        DSBuilder.UpdateDescriptorSet(GetDevice()->GetVkDevice());
        DSBuilder.ClearDirtyKey();
    }

    return bAllocated;
}

void FVulkanTransientDescriptorAllocator::FlushPools()
{
    for (auto Entry : PoolSets)
    {
        FPoolSet& Set = Entry.Second;
        if (Set.ActivePool)
        {
            FVulkanRHI::DeferDeletion(&PoolManager, Set.ActivePool);
            Set.ActivePool = nullptr;
        }

        for (FVulkanDescriptorPool* Pool : Set.UsedPools)
        {
            FVulkanRHI::DeferDeletion(&PoolManager, Pool);
        }

        Set.UsedPools.Clear();
    }

    DescriptorSetVersion++;
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

bool FVulkanDescriptorSetCache::FCachedPool::AllocateDescriptorSet(VkDescriptorSetLayout SetLayout, VkDescriptorSet& OutDescriptorSet, FVulkanDescriptorPool** OutPool)
{
    VkDescriptorSetAllocateInfo AllocateInfo = {};
    AllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    AllocateInfo.pSetLayouts        = &SetLayout;
    AllocateInfo.descriptorSetCount = 1;

    if (CurrentDescriptorPool)
    {
        if (CurrentDescriptorPool->CanAllocateDescriptorSet())
        {
            *OutPool = CurrentDescriptorPool;
            return CurrentDescriptorPool->AllocateDescriptorSet(AllocateInfo, &OutDescriptorSet);
        }

        DescriptorPools.Add(CurrentDescriptorPool);
        CurrentDescriptorPool = nullptr;
    }

    // Try to recycle a pool that has no live descriptor sets
    for (int32 Index = 0; Index < DescriptorPools.Size(); ++Index)
    {
        if (DescriptorPools[Index]->CanRecycle())
        {
            CurrentDescriptorPool = DescriptorPools[Index];
            DescriptorPools.RemoveAtSwap(Index);
            CurrentDescriptorPool->Reset();
            break;
        }
    }

    if (!CurrentDescriptorPool)
    {
        CurrentDescriptorPool = new FVulkanDescriptorPool(GetDevice());
        if (!CurrentDescriptorPool->Initialize(PoolInfo, Math::Max<int32>(CVarVulkanMaxDescriptorSetsPerPool.GetValue(), 1)))
        {
            return false;
        }
    }

    *OutPool = CurrentDescriptorPool;
    return CurrentDescriptorPool->AllocateDescriptorSet(AllocateInfo, &OutDescriptorSet);
}

FVulkanDescriptorSetCache::FVulkanDescriptorSetCache(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Caches()
    , DescriptorSets()
    , CacheCS()
    , CurrentFrame(0)
{
}

FVulkanDescriptorSetCache::~FVulkanDescriptorSetCache()
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

void FVulkanDescriptorSetCache::EvictStaleDescriptorSets(uint64 InFramesInFlight)
{
    TScopedLock Lock(CacheCS);

    CurrentFrame++;

    const uint64 SafeFrames = Math::Max(MinUnusedFrames, InFramesInFlight + 1);

    TArray<FVulkanDescriptorSetKey> StaleKeys;
    DescriptorSets.Foreach([&](const FVulkanDescriptorSetKey& Key, const FCachedDescriptorSet& Entry)
    {
        if ((CurrentFrame - Entry.LastUsedFrame) > SafeFrames)
        {
            StaleKeys.Add(Key);
        }
    });

    for (const FVulkanDescriptorSetKey& Key : StaleKeys)
    {
        FCachedDescriptorSet Evicted;
        if (DescriptorSets.RemoveKey(Key, &Evicted))
        {
            if (Evicted.OwnerPool)
            {
                Evicted.OwnerPool->DecrementLive();
            }
        }
    }
}

bool FVulkanDescriptorSetCache::FindOrCreateDescriptorSet(const FVulkanDescriptorPoolInfo& PoolInfo, FVulkanDescriptorSetBuilder& DSBuilder, VkDescriptorSet& OutDescriptorSet)
{
    TScopedLock Lock(CacheCS);

    // Get or Create a DescriptorSet
    const FVulkanDescriptorSetKey& DSKey = DSBuilder.GetKey();
    if (FCachedDescriptorSet* Cached = DescriptorSets.Find(DSKey))
    {
        Cached->LastUsedFrame = CurrentFrame;
        OutDescriptorSet = Cached->DescriptorSet;
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

        FVulkanDescriptorPool* AllocatingPool = nullptr;
        if (!CachedPool->AllocateDescriptorSet(PoolInfo.DescriptorSetLayout, OutDescriptorSet, &AllocatingPool))
        {
            return false;
        }

        CHECK(OutDescriptorSet != VK_NULL_HANDLE);
        CHECK(AllocatingPool != nullptr);

        AllocatingPool->IncrementLive();

        FCachedDescriptorSet NewEntry;
        NewEntry.DescriptorSet = OutDescriptorSet;
        NewEntry.LastUsedFrame = CurrentFrame;
        NewEntry.OwnerPool     = AllocatingPool;
        DescriptorSets.Add(DSKey, NewEntry);

        DSBuilder.SetDescriptorSet(OutDescriptorSet);
        DSBuilder.UpdateDescriptorSet(GetDevice()->GetVkDevice());
    }

    return true;
}
