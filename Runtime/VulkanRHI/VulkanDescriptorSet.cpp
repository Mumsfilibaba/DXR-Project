#include "Core/Misc/ConsoleManager.h"
#include "Core/Math/Math.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanPipelineLayout.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanSamplerState.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanConstants.h"

static TAutoConsoleVariable<int32> CVarVulkanMaxDescriptorSetsPerPool(
    "VulkanRHI.MaxDescriptorSetsPerPool",
    "The number of DescriptorSets that can be created from a DescriptorPool",
    32);

static TAutoConsoleVariable<int32> CVarVulkanTransientDescriptorSetsPerPool(
    "VulkanRHI.TransientDescriptorSetsPerPool",
    "The number of DescriptorSets per pool when using transient (non-cached) descriptor allocation",
    256);

static TAutoConsoleVariable<bool> CVarVulkanEnableBindless(
    "VulkanRHI.EnableBindless",
    "When enabled, allocates a global mutable_descriptor_type-backed bindless descriptor set "
    "(plus a separate sampler set) which is appended to PSOs whose shaders reference the heaps. "
    "Has no effect when GVulkanSupportsBindless is false (requires VK_EXT_mutable_descriptor_type "
    "and Vulkan 1.2 descriptor indexing sub-features).",
    true);

static TAutoConsoleVariable<int32> CVarVulkanNumBindlessResourceDescriptors(
    "VulkanRHI.NumBindlessResourceDescriptors",
    "Number of slots reserved in the global bindless resource descriptor set "
    "(sampled images / storage images / uniform buffers / storage buffers, all aliased via "
    "VK_EXT_mutable_descriptor_type). Default mirrors D3D12 to keep the cross-RHI budget aligned.",
    100000);

static TAutoConsoleVariable<int32> CVarVulkanNumBindlessSamplerDescriptors(
    "VulkanRHI.NumBindlessSamplerDescriptors",
    "Number of slots reserved in the global bindless sampler descriptor set. Default mirrors "
    "D3D12 to keep the cross-RHI budget aligned.",
    2048);

#if VULKAN_VALIDATE_NO_NULL_DESCRIPTORS
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

FVulkanDescriptorState::FVulkanDescriptorState(FVulkanDevice* InDevice, FVulkanPipelineLayout* InLayout, const FVulkanDefaultResources& InDefaultResources)
    : FVulkanDeviceChild(InDevice)
    , Layout(InLayout)
    , DefaultResources(InDefaultResources)
    , DescriptorSetHandles()
    , DescriptorSetBuilders()
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
    BoundResourceViews.Resize(RemappingInfos.Size());
    BoundBuffers.Resize(RemappingInfos.Size());
    
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
        Memory::Memzero(DSWrites.DescriptorWrites.Data(), DSWrites.DescriptorWrites.SizeInBytes());

        DSWrites.NullViewTypes.Resize(SetRemappingInfo.RemappingInfo.Size());

        BoundResourceViews[DescriptorSetIndex].Resize(SetRemappingInfo.RemappingInfo.Size());
        Memory::Memzero(BoundResourceViews[DescriptorSetIndex].Data(), BoundResourceViews[DescriptorSetIndex].SizeInBytes());

        BoundBuffers[DescriptorSetIndex].Resize(SetRemappingInfo.RemappingInfo.Size());
        
        // Init DescriptorWrites and count the other bindings
        uint32 NumImageInfos               = 0;
        uint32 NumBufferInfos              = 0;
        uint32 NumTexelBufferViews         = 0;
        uint32 NumAccelerationStructInfos  = 0;

        for (int32 Index = 0; Index < SetRemappingInfo.RemappingInfo.Size(); Index++)
        {
            const FVulkanDescriptorRemappingInfo::FRemappingInfo& Binding = SetRemappingInfo.RemappingInfo[Index];
            CHECK(Binding.BindingIndex == static_cast<uint32>(Index));
            
            DSWrites.NullViewTypes[Index] = Binding.NullViewType;

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

                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                {
                    NumAccelerationStructInfos++;
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

        // Allocate Buffer, Image, TexelBufferView, and AccelerationStructure Infos
        DSWrites.DescriptorImageInfos.Resize(NumImageInfos);
        Memory::Memzero(DSWrites.DescriptorImageInfos.Data(), DSWrites.DescriptorImageInfos.SizeInBytes());

        DSWrites.DescriptorBufferInfos.Resize(NumBufferInfos);
        Memory::Memzero(DSWrites.DescriptorBufferInfos.Data(), DSWrites.DescriptorBufferInfos.SizeInBytes());

        DSWrites.DescriptorTexelBufferViews.Resize(NumTexelBufferViews);
        Memory::Memzero(DSWrites.DescriptorTexelBufferViews.Data(), DSWrites.DescriptorTexelBufferViews.SizeInBytes());

        DSWrites.DescriptorAccelerationStructureInfos.Resize(NumAccelerationStructInfos);
        Memory::Memzero(DSWrites.DescriptorAccelerationStructureInfos.Data(), DSWrites.DescriptorAccelerationStructureInfos.SizeInBytes());

        DSWrites.DescriptorAccelerationStructures.Resize(NumAccelerationStructInfos);
        Memory::Memzero(DSWrites.DescriptorAccelerationStructures.Data(), DSWrites.DescriptorAccelerationStructures.SizeInBytes());

        // Setup Buffer, Image, TexelBufferView, and AccelerationStructure Infos
        uint32 CurrentImageInfo                  = 0;
        uint32 CurrentBufferInfo                 = 0;
        uint32 CurrentTexelBufferView            = 0;
        uint32 CurrentAccelerationStructureInfo  = 0;

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
                    
                    VkDescriptorBufferInfo* BufferDesc = const_cast<VkDescriptorBufferInfo*>(WriteDescriptorSet.pBufferInfo);
                    BufferDesc->buffer = DefaultResources.NullBuffer;
                    BufferDesc->offset = 0;
                    BufferDesc->range  = VK_WHOLE_SIZE;
                    break;
                }

                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    WriteDescriptorSet.pImageInfo = &DSWrites.DescriptorImageInfos[CurrentImageInfo++];
                    
                    VkDescriptorImageInfo* ImageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptorSet.pImageInfo);
                    ImageInfo->imageView   = DefaultResources.GetNullImageView(DSWrites.NullViewTypes[Index]);
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

                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                {
                    const uint32 AsSlot = CurrentAccelerationStructureInfo++;

                    VkWriteDescriptorSetAccelerationStructureKHR& AsInfo = DSWrites.DescriptorAccelerationStructureInfos[AsSlot];
                    AsInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                    AsInfo.pNext                      = nullptr;
                    AsInfo.accelerationStructureCount = 1;
                    AsInfo.pAccelerationStructures    = &DSWrites.DescriptorAccelerationStructures[AsSlot];

                    WriteDescriptorSet.pNext = &AsInfo;
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
        DescriptorSetBuilders[DescriptorSetIndex].SetupDescriptorWrites(
            PoolInfo.DescriptorSetLayout, DSWrites.DescriptorWrites.Data(), DSWrites.DescriptorWrites.Size());

        for (int32 BindingIndex = 0; BindingIndex < SetRemappingInfo.RemappingInfo.Size(); BindingIndex++)
        {
            if (SetRemappingInfo.RemappingInfo[BindingIndex].BindingType == EVulkanBindingType::ImmutableSampler)
            {
                DescriptorSetBuilders[DescriptorSetIndex].MarkBindingAsImmutable(BindingIndex);
            }
        }

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
    Memory::Memzero(DynamicOffsets.Data(), DynamicOffsets.SizeInBytes());

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

void FVulkanDescriptorState::SetSRV(FVulkanShaderResourceViewRHI* ShaderResourceView, uint32 DescriptorSetIndex, uint32 BindingIndex)
{
    CHECK(DescriptorSetIndex < static_cast<uint32>(DescriptorSetBuilders.Size()));

    BoundResourceViews[DescriptorSetIndex][BindingIndex] = nullptr;
    BoundBuffers[DescriptorSetIndex][BindingIndex]       = FBoundBuffer();

    FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[DescriptorSetIndex];
    if (ShaderResourceView)
    {
        switch(ShaderResourceView->GetType())
        {
            case FVulkanResourceView::EType::ImageView:
            {
                const FVulkanResourceView::FImageView& ImageViewInfo = ShaderResourceView->GetImageViewInfo();
                DSBuilder.WriteSampledImage(BindingIndex, ImageViewInfo.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                BoundResourceViews[DescriptorSetIndex][BindingIndex] = ShaderResourceView;
                break; 
            }

            case FVulkanResourceView::EType::StructuredBufferView:
            {
                const FVulkanResourceView::FStructuredBufferView& StructuredBufferView = ShaderResourceView->GetStructuredBufferInfo();
                DSBuilder.WriteStorageBuffer(BindingIndex, StructuredBufferView.Buffer, StructuredBufferView.Offset, StructuredBufferView.Range);
                SetBoundBuffer(ShaderResourceView->GetResource(), EResourceAccess::ShaderResource, DescriptorSetIndex, BindingIndex);
                break; 
            }

            case FVulkanResourceView::EType::TypedBufferView:
            {
                const FVulkanResourceView::FTypedBufferView& TypedBufferView = ShaderResourceView->GetTypedBufferInfo();
                DSBuilder.WriteUniformTexelBuffer(BindingIndex, TypedBufferView.BufferView);
                SetBoundBuffer(ShaderResourceView->GetResource(), EResourceAccess::ShaderResource, DescriptorSetIndex, BindingIndex);
                break;
            }

            case FVulkanResourceView::EType::AccelerationStructureView:
            {
                const FVulkanResourceView::FAccelerationStructureView& AsInfo = ShaderResourceView->GetAccelerationStructureInfo();
                DSBuilder.WriteAccelerationStructure(BindingIndex, AsInfo.AccelerationStructure);
                BoundResourceViews[DescriptorSetIndex][BindingIndex] = ShaderResourceView;
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

    DirtyResources();
}

void FVulkanDescriptorState::SetUAV(FVulkanUnorderedAccessViewRHI* UnorderedAccessView, uint32 DescriptorSetIndex, uint32 BindingIndex)
{
    CHECK(DescriptorSetIndex < static_cast<uint32>(DescriptorSetBuilders.Size()));

    BoundResourceViews[DescriptorSetIndex][BindingIndex] = nullptr;
    BoundBuffers[DescriptorSetIndex][BindingIndex]       = FBoundBuffer();

    FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[DescriptorSetIndex];
    if (UnorderedAccessView)
    {
        switch(UnorderedAccessView->GetType())
        {
            case FVulkanResourceView::EType::ImageView:
            {
                const FVulkanResourceView::FImageView& ImageViewInfo = UnorderedAccessView->GetImageViewInfo();
                DSBuilder.WriteStorageImage(BindingIndex, ImageViewInfo.ImageView, VK_IMAGE_LAYOUT_GENERAL);
                BoundResourceViews[DescriptorSetIndex][BindingIndex] = UnorderedAccessView;
                break;
            }

            case FVulkanResourceView::EType::StructuredBufferView:
            {
                const FVulkanResourceView::FStructuredBufferView& StructuredBufferView = UnorderedAccessView->GetStructuredBufferInfo();
                DSBuilder.WriteStorageBuffer(BindingIndex, StructuredBufferView.Buffer, StructuredBufferView.Offset, StructuredBufferView.Range);
                SetBoundBuffer(UnorderedAccessView->GetResource(), EResourceAccess::UnorderedAccess, DescriptorSetIndex, BindingIndex);
                break;
            }

            case FVulkanResourceView::EType::TypedBufferView:
            {
                const FVulkanResourceView::FTypedBufferView& TypedBufferView = UnorderedAccessView->GetTypedBufferInfo();
                DSBuilder.WriteStorageTexelBuffer(BindingIndex, TypedBufferView.BufferView);
                SetBoundBuffer(UnorderedAccessView->GetResource(), EResourceAccess::UnorderedAccess, DescriptorSetIndex, BindingIndex);
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

    DirtyResources();
}

void FVulkanDescriptorState::SetBoundBuffer(FRHIResource* Resource, EResourceAccess Access, uint32 DescriptorSetIndex, uint32 BindingIndex)
{
    FBoundBuffer& BoundBuffer = BoundBuffers[DescriptorSetIndex][BindingIndex];
    BoundBuffer.Buffer = FVulkanDeviceRHI::ResourceCast(static_cast<FRHIBuffer*>(Resource));
    BoundBuffer.Access = Access;
}

void FVulkanDescriptorState::SetUniformBuffer(FVulkanBufferRHI* UniformBuffer, uint32 DescriptorSetIndex, uint32 BindingIndex)
{
    CHECK(DescriptorSetIndex < static_cast<uint32>(DescriptorSetBuilders.Size()));

    BoundBuffers[DescriptorSetIndex][BindingIndex] = FBoundBuffer{ UniformBuffer, EResourceAccess::ConstantBuffer };

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

            CHECK((DynamicOffset % GetDevice()->GetPhysicalDevice()->GetProperties().limits.minUniformBufferOffsetAlignment) == 0);

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

    DirtyResources();
}

void FVulkanDescriptorState::SetSampler(FVulkanSamplerStateRHI* SamplerState, uint32 DescriptorSetIndex, uint32 BindingIndex)
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

    DirtyResources();
}

void FVulkanDescriptorState::ResolveSampledImageLayouts(FVulkanTextureRHI* ReadOnlyDepthTexture, VkImageLayout ReadOnlyDepthLayout)
{
    for (int32 SetIndex = 0; SetIndex < DescriptorSetWrites.Size(); SetIndex++)
    {
        const FVulkanDescriptorWrites& DSWrites  = DescriptorSetWrites[SetIndex];
        FVulkanDescriptorSetBuilder&   DSBuilder = DescriptorSetBuilders[SetIndex];

        for (int32 BindingIndex = 0; BindingIndex < DSWrites.DescriptorWrites.Size(); BindingIndex++)
        {
            if (DSWrites.DescriptorWrites[BindingIndex].descriptorType != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
            {
                continue;
            }

            FVulkanResourceView* View = BoundResourceViews[SetIndex][BindingIndex];
            if (!View)
            {
                continue;
            }

            FVulkanShaderResourceViewRHI* ShaderResourceView = static_cast<FVulkanShaderResourceViewRHI*>(View);
            FVulkanTextureRHI*            Texture            = FVulkanDeviceRHI::ResourceCast(static_cast<FRHITexture*>(ShaderResourceView->GetResource()));

            const bool          bIsReadOnlyDepth = ReadOnlyDepthTexture && Texture == ReadOnlyDepthTexture;
            const VkImageLayout DesiredLayout    = bIsReadOnlyDepth ? ReadOnlyDepthLayout : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            const VkDescriptorImageInfo* ImageInfo = DSWrites.DescriptorWrites[BindingIndex].pImageInfo;
            if (ImageInfo && ImageInfo->imageLayout != DesiredLayout)
            {
                DSBuilder.WriteSampledImage(BindingIndex, View->GetImageViewInfo().ImageView, DesiredLayout);
                DirtyResources();
            }
        }
    }
}

void FVulkanDescriptorState::TransitionBoundResources(FVulkanCommandContext& Context)
{
    for (int32 SetIndex = 0; SetIndex < DescriptorSetWrites.Size(); SetIndex++)
    {
        const FVulkanDescriptorWrites& DSWrites = DescriptorSetWrites[SetIndex];
        for (int32 BindingIndex = 0; BindingIndex < DSWrites.DescriptorWrites.Size(); BindingIndex++)
        {
            const FBoundBuffer& BoundBuffer = BoundBuffers[SetIndex][BindingIndex];
            if (BoundBuffer.Buffer)
            {
                Context.RequireBufferState(BoundBuffer.Buffer, BoundBuffer.Access);
            }

            FVulkanResourceView* View = BoundResourceViews[SetIndex][BindingIndex];
            if (!View)
            {
                continue;
            }

            const VkDescriptorType Type = DSWrites.DescriptorWrites[BindingIndex].descriptorType;
            if (Type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
            {
                Context.TransitionImageLayout(static_cast<FVulkanUnorderedAccessViewRHI*>(View));
            }
            else if (Type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
            {
                const VkDescriptorImageInfo* ImageInfo = DSWrites.DescriptorWrites[BindingIndex].pImageInfo;
                if (ImageInfo)
                {
                    Context.TransitionImageLayout(static_cast<FVulkanShaderResourceViewRHI*>(View), ImageInfo->imageLayout);
                }
            }
        }
    }
}

void FVulkanDescriptorState::UpdateDescriptorSets(FVulkanTransientDescriptorAllocator* TransientAllocator)
{
    for (int32 Index = 0; Index < DescriptorSetHandles.Size(); Index++)
    {
    #if VULKAN_VALIDATE_NO_NULL_DESCRIPTORS
        const FVulkanDescriptorWrites&        DSWrites      = DescriptorSetWrites[Index];
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
                #if VULKAN_BREAK_ON_NULL_DESCRIPTORS
                    DEBUG_BREAK();
                #endif
                }
            }
            else if (WriteInfo.pImageInfo)
            {
                if (WriteInfo.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || WriteInfo.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                {
                    if (WriteInfo.pImageInfo->imageView == DefaultResources.GetNullImageView(DSWrites.NullViewTypes[BindIdx]))
                    {
                        VULKAN_WARNING("Null image view descriptor '%s' (register t%u) at set=%d binding=%u (%s)",
                            BindingName, OriginalBinding, Index, WriteInfo.dstBinding, GetDescriptorTypeName(WriteInfo.descriptorType));
                    #if VULKAN_BREAK_ON_NULL_DESCRIPTORS
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
                    #if VULKAN_BREAK_ON_NULL_DESCRIPTORS
                        DEBUG_BREAK();
                    #endif
                    }
                }
            }
        }
     #endif

        FVulkanDescriptorSetBuilder& DSBuilder = DescriptorSetBuilders[Index];

        #if VULKAN_USE_DESCRIPTOR_CACHE
            UNREFERENCED_VARIABLE(TransientAllocator);
            DSBuilder.UpdateHash();

            // Must run every draw even when not dirty. The cache hit updates LastUsedFrame,
            // preventing the eviction logic from reclaiming the entry and recycling its pool.
            
            FVulkanDescriptorSetCache& DescriptorSetCache = GetDevice()->GetDescriptorSetCache();
            if (!DescriptorSetCache.FindOrCreateDescriptorSet(DescriptorPoolInfos[Index], DSBuilder, DescriptorSetHandles[Index]))
            {
                VULKAN_ERROR_CRITICAL("Failed to find or create DescriptorSet");
                return;
            }
        #else
            CHECK(TransientAllocator != nullptr);

            if (TransientAllocator->GetDescriptorSetVersion() != DescriptorSetVersion)
            {
                DescriptorSetVersion = TransientAllocator->GetDescriptorSetVersion();
                Memory::Memzero(DescriptorSetHandles.Data(), DescriptorSetHandles.SizeInBytes());
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
        #endif
    }
}

void FVulkanDescriptorState::Reset()
{
    for (int32 DescriptorSetIndex = 0; DescriptorSetIndex < DescriptorSetWrites.Size(); DescriptorSetIndex++)
    {
        FVulkanDescriptorWrites& DSWrites = DescriptorSetWrites[DescriptorSetIndex];
        for (int32 BindingIndex = 0; BindingIndex < DSWrites.DescriptorWrites.Size(); BindingIndex++)
        {
            BoundResourceViews[DescriptorSetIndex][BindingIndex] = nullptr;
            BoundBuffers[DescriptorSetIndex][BindingIndex]       = FBoundBuffer();
            ResetDescriptorBinding(DescriptorSetIndex, BindingIndex);
        }
    }

    DirtyResources();
}

void FVulkanDescriptorState::BindDescriptorSets(class FVulkanCommandBuffer& CommandBuffer, VkPipelineBindPoint BindPoint)
{
    // When this PSO opts in to bindless, the bindless descriptor set lives at slot 0 and the
    // regular per-stage descriptor sets follow at slot 1+. PSOs that don't reference the heap
    // skip the bindless bind entirely and place their regular sets at slot 0+.

    const bool bHasBindless = Layout->HasBindlessSet();
    if (bHasBindless)
    {
        FVulkanBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();
        if (BindlessManager && BindlessManager->IsEnabled())
        {
            VkDescriptorSet BindlessSet = BindlessManager->GetDescriptorSet();
            if (VULKAN_CHECK_HANDLE(BindlessSet))
            {
                CommandBuffer->BindDescriptorSets(
                    BindPoint, 
                    Layout->GetVkPipelineLayout(),
                    VULKAN_BINDLESS_RUNTIME_SET_INDEX, 
                    1, 
                    &BindlessSet, 
                    0,
                    nullptr);
            }
        }
    }

    if (DescriptorSetHandles.Size() > 0)
    {
        const uint32 RegularFirstSet = bHasBindless ? (VULKAN_BINDLESS_RUNTIME_SET_INDEX + 1) : 0;
        CommandBuffer->BindDescriptorSets(
            BindPoint, 
            Layout->GetVkPipelineLayout(), 
            RegularFirstSet, 
            DescriptorSetHandles.Size(),
            DescriptorSetHandles.Data(), 
            DynamicOffsets.Size(), 
            DynamicOffsets.Data());
    }

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
            const VkImageView NullView = DefaultResources.GetNullImageView(DSWrites.NullViewTypes[BindingIndex]);
            DSBuilder.WriteStorageImage(BindingIndex, NullView, VK_IMAGE_LAYOUT_GENERAL);
            break;
        }

        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            const VkImageView NullView = DefaultResources.GetNullImageView(DSWrites.NullViewTypes[BindingIndex]);
            DSBuilder.WriteSampledImage(BindingIndex, NullView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            break;
        }

        case VK_DESCRIPTOR_TYPE_SAMPLER:
        {
            DSBuilder.WriteSampler(BindingIndex, DefaultResources.NullSampler);
            break;
        }

        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        {
            DSBuilder.WriteAccelerationStructure(BindingIndex, VK_NULL_HANDLE);
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
            FVulkanDeviceRHI::DeferDeletion(&PoolManager, Set.ActivePool);
            Set.ActivePool = nullptr;
        }

        for (FVulkanDescriptorPool* Pool : Set.UsedPools)
        {
            FVulkanDeviceRHI::DeferDeletion(&PoolManager, Pool);
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


FVulkanBindlessDescriptorManager::FVulkanBindlessDescriptorManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , bIsEnabled(false)
    , DescriptorPool(VK_NULL_HANDLE)
    , SetLayout(VK_NULL_HANDLE)
    , DescriptorSet(VK_NULL_HANDLE)
    , ResourceCapacity(0)
    , SamplerCapacity(0)
    , NextFreshResourceSlot(0)
    , NextFreshSamplerSlot(0)
    , FreeResourceStack()
    , FreeSamplerStack()
    , AllocCS()
    , PendingWrites()
    , PendingWritesCS()
{
}

FVulkanBindlessDescriptorManager::~FVulkanBindlessDescriptorManager()
{
    Release();
}

bool FVulkanBindlessDescriptorManager::Initialize()
{
#if !VK_EXT_mutable_descriptor_type
    return false;
#else
    if (!GVulkanSupportsBindless || !CVarVulkanEnableBindless.GetValue())
    {
        return false;
    }

    const uint32 RequestedResources = static_cast<uint32>(Math::Max<int32>(0, CVarVulkanNumBindlessResourceDescriptors.GetValue()));
    const uint32 RequestedSamplers  = static_cast<uint32>(Math::Max<int32>(0, CVarVulkanNumBindlessSamplerDescriptors.GetValue()));

    if (RequestedResources == 0 && RequestedSamplers == 0)
    {
        return false;
    }

    ResourceCapacity = Math::Min<uint32>(RequestedResources, GVulkanMaxDescriptorSetSampledImages);
    SamplerCapacity  = Math::Min<uint32>(RequestedSamplers,  GVulkanMaxDescriptorSetSamplers);

    if (ResourceCapacity == 0 && SamplerCapacity == 0)
    {
        VULKAN_WARNING("Bindless capacities clamped to zero; bindless descriptor manager disabled");
        return false;
    }
    
    constexpr uint32 NumBaseMutableDescriptorTypes            = 6;
    constexpr uint32 MaxMutableDescriptorTypesWithAccelStruct = NumBaseMutableDescriptorTypes + 1;

    VkDescriptorType MutableTypes[MaxMutableDescriptorTypesWithAccelStruct] =
    {
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
    };

    uint32 NumMutableTypes = NumBaseMutableDescriptorTypes;
    if (GVulkanSupportsAccelerationStructures)
    {
        MutableTypes[NumMutableTypes++] = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    }

    VkMutableDescriptorTypeListEXT MutableLists[2] = {};
    MutableLists[0].descriptorTypeCount = NumMutableTypes;
    MutableLists[0].pDescriptorTypes    = MutableTypes;
    MutableLists[1].descriptorTypeCount = 0;
    MutableLists[1].pDescriptorTypes    = nullptr;

    VkMutableDescriptorTypeCreateInfoEXT MutableCreateInfo = {};
    MutableCreateInfo.sType                          = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT;
    MutableCreateInfo.mutableDescriptorTypeListCount = ARRAY_COUNT(MutableLists);
    MutableCreateInfo.pMutableDescriptorTypeLists    = MutableLists;

    VkDescriptorSetLayoutBinding Bindings[2] = {};
    Bindings[0].binding         = VULKAN_BINDLESS_RESOURCE_BINDING;
    Bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
    Bindings[0].descriptorCount = ResourceCapacity;
    Bindings[0].stageFlags      = VK_SHADER_STAGE_ALL;
    Bindings[1].binding         = VULKAN_BINDLESS_SAMPLER_BINDING;
    Bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
    Bindings[1].descriptorCount = SamplerCapacity;
    Bindings[1].stageFlags      = VK_SHADER_STAGE_ALL;

    constexpr VkDescriptorBindingFlags BindingFlagsCommon =
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
        VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;

    VkDescriptorBindingFlags BindingFlags[2] = { BindingFlagsCommon, BindingFlagsCommon };

    VkDescriptorSetLayoutBindingFlagsCreateInfo BindingFlagsCreateInfo = {};
    BindingFlagsCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    BindingFlagsCreateInfo.bindingCount  = ARRAY_COUNT(BindingFlags);
    BindingFlagsCreateInfo.pBindingFlags = BindingFlags;
    BindingFlagsCreateInfo.pNext         = &MutableCreateInfo;

    VkDescriptorSetLayoutCreateInfo SetLayoutCreateInfo = {};
    SetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    SetLayoutCreateInfo.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    SetLayoutCreateInfo.bindingCount = ARRAY_COUNT(Bindings);
    SetLayoutCreateInfo.pBindings    = Bindings;
    SetLayoutCreateInfo.pNext        = &BindingFlagsCreateInfo;

    if (VULKAN_FAILED(vkCreateDescriptorSetLayout(GetDevice()->GetVkDevice(), &SetLayoutCreateInfo, nullptr, &SetLayout)))
    {
        VULKAN_ERROR("Failed to create bindless descriptor set layout");
        Release();
        return false;
    }

    VkDescriptorPoolSize PoolSizes[2] = {};
    PoolSizes[0].type            = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
    PoolSizes[0].descriptorCount = ResourceCapacity;
    PoolSizes[1].type            = VK_DESCRIPTOR_TYPE_SAMPLER;
    PoolSizes[1].descriptorCount = SamplerCapacity;

    VkDescriptorPoolCreateInfo PoolCreateInfo = {};
    PoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolCreateInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    PoolCreateInfo.maxSets       = 1;
    PoolCreateInfo.poolSizeCount = (ResourceCapacity > 0 ? 1 : 0) + (SamplerCapacity > 0 ? 1 : 0);
    PoolCreateInfo.pPoolSizes    = (ResourceCapacity > 0) ? &PoolSizes[0] : &PoolSizes[1];

    if (VULKAN_FAILED(vkCreateDescriptorPool(GetDevice()->GetVkDevice(), &PoolCreateInfo, nullptr, &DescriptorPool)))
    {
        VULKAN_ERROR("Failed to create bindless descriptor pool");
        Release();
        return false;
    }

    VkDescriptorSetAllocateInfo SetAllocateInfo = {};
    SetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    SetAllocateInfo.descriptorPool     = DescriptorPool;
    SetAllocateInfo.descriptorSetCount = 1;
    SetAllocateInfo.pSetLayouts        = &SetLayout;

    if (VULKAN_FAILED(vkAllocateDescriptorSets(GetDevice()->GetVkDevice(), &SetAllocateInfo, &DescriptorSet)))
    {
        VULKAN_ERROR("Failed to allocate bindless descriptor set");
        Release();
        return false;
    }

    bIsEnabled = true;

    VULKAN_INFO("Bindless descriptor manager initialized: Resources=%u Samplers=%u (runtime set=%u)",
        ResourceCapacity, SamplerCapacity, VULKAN_BINDLESS_RUNTIME_SET_INDEX);

    return true;
#endif
}

void FVulkanBindlessDescriptorManager::Release()
{
    bIsEnabled = false;

    {
        TScopedLock Lock(PendingWritesCS);
        PendingWrites.Clear();
    }

    if (DescriptorPool != VK_NULL_HANDLE)
    {
        // The descriptor set is freed implicitly when the pool is destroyed.
        vkDestroyDescriptorPool(GetDevice()->GetVkDevice(), DescriptorPool, nullptr);
        DescriptorPool = VK_NULL_HANDLE;
        DescriptorSet  = VK_NULL_HANDLE;
    }

    if (SetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(GetDevice()->GetVkDevice(), SetLayout, nullptr);
        SetLayout = VK_NULL_HANDLE;
    }

    {
        TScopedLock Lock(AllocCS);
        
        FreeResourceStack.Clear();
        FreeSamplerStack.Clear();

        NextFreshResourceSlot = 0;
        NextFreshSamplerSlot  = 0;
        ResourceCapacity      = 0;
        SamplerCapacity       = 0;
    }
}

FRHIDescriptorHandle FVulkanBindlessDescriptorManager::Allocate(EDescriptorType InType)
{
    if (!bIsEnabled || InType == EDescriptorType::Unknown)
    {
        return FRHIDescriptorHandle();
    }

    TScopedLock Lock(AllocCS);

    if (InType == EDescriptorType::Sampler)
    {
        uint32 SlotIndex = 0;
        if (!FreeSamplerStack.IsEmpty())
        {
            SlotIndex = FreeSamplerStack.LastElement();
            FreeSamplerStack.Pop();
        }
        else
        {
            if (NextFreshSamplerSlot >= SamplerCapacity)
            {
                VULKAN_ERROR("Bindless sampler heap exhausted (Capacity=%u)", SamplerCapacity);
                return FRHIDescriptorHandle();
            }

            SlotIndex = NextFreshSamplerSlot++;
        }

        return FRHIDescriptorHandle(EDescriptorType::Sampler, SlotIndex);
    }

    uint32 SlotIndex = 0;
    if (!FreeResourceStack.IsEmpty())
    {
        SlotIndex = FreeResourceStack.LastElement();
        FreeResourceStack.Pop();
    }
    else
    {
        if (NextFreshResourceSlot >= ResourceCapacity)
        {
            VULKAN_ERROR("Bindless resource heap exhausted (Capacity=%u)", ResourceCapacity);
            return FRHIDescriptorHandle();
        }

        SlotIndex = NextFreshResourceSlot++;
    }

    return FRHIDescriptorHandle(InType, SlotIndex);
}

void FVulkanBindlessDescriptorManager::Free(FRHIDescriptorHandle Handle)
{
    if (!bIsEnabled || !Handle.IsValid())
    {
        return;
    }

    RecycleSlot(Handle);
}

void FVulkanBindlessDescriptorManager::RecycleSlot(FRHIDescriptorHandle Handle)
{
    TScopedLock Lock(AllocCS);

    if (Handle.Type == EDescriptorType::Sampler)
    {
        CHECK(Handle.Index < SamplerCapacity);
        FreeSamplerStack.Add(Handle.Index);
    }
    else
    {
        CHECK(Handle.Index < ResourceCapacity);
        FreeResourceStack.Add(Handle.Index);
    }
}

void FVulkanBindlessDescriptorManager::EnqueueImageWrite(FRHIDescriptorHandle Handle, VkImageView ImageView, VkImageLayout ImageLayout, VkDescriptorType DescriptorType)
{
    if (!bIsEnabled || !Handle.IsValid() || ImageView == VK_NULL_HANDLE)
    {
        return;
    }

    TScopedLock Lock(PendingWritesCS);

    FVulkanPendingBindlessWrite& Entry = PendingWrites.Emplace();
    Entry.Binding           = VULKAN_BINDLESS_RESOURCE_BINDING;
    Entry.ArraySlot         = Handle.Index;
    Entry.DescriptorType    = DescriptorType;
    Entry.Image.sampler     = VK_NULL_HANDLE;
    Entry.Image.imageView   = ImageView;
    Entry.Image.imageLayout = ImageLayout;
}

void FVulkanBindlessDescriptorManager::EnqueueBufferWrite(FRHIDescriptorHandle Handle, VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Range, VkDescriptorType DescriptorType)
{
    if (!bIsEnabled || !Handle.IsValid() || Buffer == VK_NULL_HANDLE)
    {
        return;
    }

    TScopedLock Lock(PendingWritesCS);

    FVulkanPendingBindlessWrite& Entry = PendingWrites.Emplace();
    Entry.Binding        = VULKAN_BINDLESS_RESOURCE_BINDING;
    Entry.ArraySlot      = Handle.Index;
    Entry.DescriptorType = DescriptorType;
    Entry.Buffer.buffer  = Buffer;
    Entry.Buffer.offset  = Offset;
    Entry.Buffer.range   = Range;
}

void FVulkanBindlessDescriptorManager::EnqueueTexelBufferWrite(FRHIDescriptorHandle Handle, VkBufferView BufferView, VkDescriptorType DescriptorType)
{
    if (!bIsEnabled || !Handle.IsValid() || BufferView == VK_NULL_HANDLE)
    {
        return;
    }

    TScopedLock Lock(PendingWritesCS);

    FVulkanPendingBindlessWrite& Entry = PendingWrites.Emplace();
    Entry.Binding        = VULKAN_BINDLESS_RESOURCE_BINDING;
    Entry.ArraySlot      = Handle.Index;
    Entry.DescriptorType = DescriptorType;
    Entry.TexelBuffer    = BufferView;
}

void FVulkanBindlessDescriptorManager::EnqueueAccelerationStructureWrite(FRHIDescriptorHandle Handle, VkAccelerationStructureKHR AccelerationStructure)
{
    if (!bIsEnabled || !Handle.IsValid() || AccelerationStructure == VK_NULL_HANDLE)
    {
        return;
    }

    TScopedLock Lock(PendingWritesCS);

    FVulkanPendingBindlessWrite& Entry = PendingWrites.Emplace();
    Entry.Binding                                          = VULKAN_BINDLESS_RESOURCE_BINDING;
    Entry.ArraySlot                                        = Handle.Index;
    Entry.DescriptorType                                   = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    Entry.AccelerationStructureHandle                      = AccelerationStructure;
    Entry.AccelerationStructure.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    Entry.AccelerationStructure.accelerationStructureCount = 1;
    Entry.AccelerationStructure.pAccelerationStructures    = nullptr;
}

void FVulkanBindlessDescriptorManager::EnqueueSamplerWrite(FRHIDescriptorHandle Handle, VkSampler Sampler)
{
    if (!bIsEnabled || !Handle.IsValid() || Sampler == VK_NULL_HANDLE)
    {
        return;
    }

    TScopedLock Lock(PendingWritesCS);

    FVulkanPendingBindlessWrite& Entry = PendingWrites.Emplace();
    Entry.Binding           = VULKAN_BINDLESS_SAMPLER_BINDING;
    Entry.ArraySlot         = Handle.Index;
    Entry.DescriptorType    = VK_DESCRIPTOR_TYPE_SAMPLER;
    Entry.Image.sampler     = Sampler;
    Entry.Image.imageView   = VK_NULL_HANDLE;
    Entry.Image.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void FVulkanBindlessDescriptorManager::Flush()
{
    if (!bIsEnabled)
    {
        return;
    }

    TArray<FVulkanPendingBindlessWrite> LocalWrites;
    {
        TScopedLock Lock(PendingWritesCS);
        
        if (PendingWrites.IsEmpty())
        {
            return;
        }

        LocalWrites = Move(PendingWrites);
        PendingWrites.Clear();
    }

    const int32 NumWrites = LocalWrites.Size();
    if (NumWrites <= 0)
    {
        return;
    }

    TArray<VkWriteDescriptorSet> Writes;
    Writes.Resize(NumWrites);

    Memory::Memzero(Writes.Data(), Writes.SizeInBytes());

    for (int32 Index = 0; Index < NumWrites; ++Index)
    {
        FVulkanPendingBindlessWrite& Entry = LocalWrites[Index];
        VkWriteDescriptorSet&        Write = Writes[Index];

        Write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        Write.dstSet          = DescriptorSet;
        Write.dstBinding      = Entry.Binding;
        Write.dstArrayElement = Entry.ArraySlot;
        Write.descriptorCount = 1;
        Write.descriptorType  = Entry.DescriptorType;

        switch (Entry.DescriptorType)
        {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            {
                Write.pImageInfo = &Entry.Image;
                break;
            }

            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            {
                Write.pBufferInfo = &Entry.Buffer;
                break;
            }

            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            {
                Write.pTexelBufferView = &Entry.TexelBuffer;
                break;
            }

            case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            {
                Entry.AccelerationStructure.pAccelerationStructures = &Entry.AccelerationStructureHandle;
                Write.pNext = &Entry.AccelerationStructure;
                break;
            }

            default:
            {
                VULKAN_WARNING("Unhandled bindless descriptor type %d in Flush()", static_cast<int32>(Entry.DescriptorType));
                break;
            }
        }
    }

    vkUpdateDescriptorSets(GetDevice()->GetVkDevice(), static_cast<uint32>(NumWrites), Writes.Data(), 0, nullptr);
}
