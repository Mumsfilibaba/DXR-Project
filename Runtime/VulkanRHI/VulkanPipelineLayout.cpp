#include "Core/Memory/Memory.h"
#include "RHI/RHISamplerState.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanCore.h"
#include "VulkanRHI/VulkanPipelineLayout.h"
#include "VulkanRHI/VulkanConstants.h"
#include "VulkanRHI/VulkanDescriptorSet.h"
#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanDeviceDebug.h"

static inline EShaderVisibility::Type GetShaderVisibilityFromShaderFlag(VkShaderStageFlags ShaderStage)
{
    constexpr VkShaderStageFlags RayTracingStageMask =
        VK_SHADER_STAGE_RAYGEN_BIT_KHR       | VK_SHADER_STAGE_MISS_BIT_KHR     |
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR  | VK_SHADER_STAGE_ANY_HIT_BIT_KHR  |
        VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_KHR;

    if ((ShaderStage & RayTracingStageMask) != 0)
    {
        return EShaderVisibility::RayTracing;
    }

    switch(ShaderStage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:                  return EShaderVisibility::Vertex;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    return EShaderVisibility::Hull;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return EShaderVisibility::Domain;
    case VK_SHADER_STAGE_GEOMETRY_BIT:                return EShaderVisibility::Geometry;
    case VK_SHADER_STAGE_FRAGMENT_BIT:                return EShaderVisibility::Pixel;
    case VK_SHADER_STAGE_COMPUTE_BIT:                 return EShaderVisibility::Compute;
#if VK_EXT_mesh_shader
    case VK_SHADER_STAGE_TASK_BIT_EXT:                return EShaderVisibility::Task;
    case VK_SHADER_STAGE_MESH_BIT_EXT:                return EShaderVisibility::Mesh;
#endif
    default:                                          return EShaderVisibility::Compute;
    }
}

static EResourceType::Type GetResourceBucket(EVulkanBindingType::Type BindingType)
{
    switch (BindingType)
    {
    case EVulkanBindingType::UniformBuffer:
    case EVulkanBindingType::UniformBufferDynamic:   return EResourceType::UniformBuffer;
    case EVulkanBindingType::Sampler:
    case EVulkanBindingType::ImmutableSampler:       return EResourceType::Sampler;
    case EVulkanBindingType::StorageImage:
    case EVulkanBindingType::StorageBufferReadWrite: return EResourceType::UAV;
    default:                                         return EResourceType::SRV; // SampledImage, StorageBufferRead, AccelerationStructure
    }
}

static VkShaderStageFlags GetVkStageFlagsFromShaderStage(EShaderStage Stage)
{
    switch (Stage)
    {
    case EShaderStage::Vertex:        return VK_SHADER_STAGE_VERTEX_BIT;
    case EShaderStage::Hull:          return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case EShaderStage::Domain:        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case EShaderStage::Geometry:      return VK_SHADER_STAGE_GEOMETRY_BIT;
    case EShaderStage::Pixel:         return VK_SHADER_STAGE_FRAGMENT_BIT;
    case EShaderStage::Compute:       return VK_SHADER_STAGE_COMPUTE_BIT;
#if VK_EXT_mesh_shader
    case EShaderStage::Mesh:          return VK_SHADER_STAGE_MESH_BIT_EXT;
    case EShaderStage::Amplification: return VK_SHADER_STAGE_TASK_BIT_EXT;
#endif
    default:                          return 0;
    }
}

void FVulkanPipelineLayoutInfo::AddSetForStage(VkShaderStageFlagBits ShaderStage, const FVulkanShaderInfo& ShaderInfo)
{
    // Setup all the bindings
    FVulkanDescriptorSetLayoutInfo LayoutInfo;
    LayoutInfo.Bindings.Reserve(ShaderInfo.ResourceBindings.Size());
    
    // Setup all the remapping info
    FVulkanDescriptorRemappingInfo LayoutRemappings;
    LayoutRemappings.RemappingInfo.Reserve(ShaderInfo.ResourceBindings.Size());

#if VULKAN_ENABLE_BINDING_DEBUG_NAMES
    LayoutRemappings.DebugNames.Reserve(ShaderInfo.ResourceBindings.Size());
#endif

    for (const FVulkanShaderInfo::FResourceBinding& Binding : ShaderInfo.ResourceBindings)
    {
        VkDescriptorSetLayoutBinding LayoutBinding = {};
        LayoutBinding.descriptorCount    = 1;
        LayoutBinding.binding            = Binding.BindingIndex;
        LayoutBinding.pImmutableSamplers = nullptr;
        LayoutBinding.stageFlags         = ShaderStage;
        LayoutBinding.descriptorType     = GetDescriptorTypeFromBindingType(Binding.BindingType);
        LayoutInfo.Bindings.Add(LayoutBinding);

        FVulkanDescriptorRemappingInfo::FRemappingInfo RemappingInfo = {};
        RemappingInfo.BindingType          = Binding.BindingType;
        RemappingInfo.BindingIndex         = Binding.BindingIndex;
        RemappingInfo.NullViewType         = Binding.NullViewType;
        RemappingInfo.OriginalBindingIndex = Binding.OriginalBindingIndex;
        LayoutRemappings.RemappingInfo.Add(RemappingInfo);

    #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
        LayoutRemappings.DebugNames.Add(Binding.DebugName);
    #endif
    }

    SetLayoutInfos.Add(Move(LayoutInfo));
    SetLayoutRemappings.Add(Move(LayoutRemappings));

    if (ShaderInfo.UsesBindlessHeap())
    {
        bAnyStageUsesBindless = true;
    }
}

void FVulkanPipelineLayoutInfo::MergeSetForStage(VkShaderStageFlagBits ShaderStage, const FVulkanShaderInfo& ShaderInfo)
{
    if (SetLayoutInfos.IsEmpty())
    {
        AddSetForStage(ShaderStage, ShaderInfo);
        return;
    }

    FVulkanDescriptorSetLayoutInfo& LayoutInfo  = SetLayoutInfos[0];
    FVulkanDescriptorRemappingInfo& LayoutRemap = SetLayoutRemappings[0];

    for (const FVulkanShaderInfo::FResourceBinding& Binding : ShaderInfo.ResourceBindings)
    {
        const VkDescriptorType DescriptorType = GetDescriptorTypeFromBindingType(Binding.BindingType);

        const EResourceType::Type Bucket = GetResourceBucket(Binding.BindingType);
        int32 ExistingIndex = -1;
        for (int32 Index = 0; Index < LayoutInfo.Bindings.Size(); ++Index)
        {
            if (LayoutRemap.RemappingInfo[Index].OriginalBindingIndex == Binding.OriginalBindingIndex &&
                GetResourceBucket(LayoutRemap.RemappingInfo[Index].BindingType) == Bucket)
            {
                ExistingIndex = Index;
                break;
            }
        }

        if (ExistingIndex != -1)
        {
            const bool bTypeConflict = (LayoutInfo.Bindings[ExistingIndex].descriptorType != DescriptorType) ||
                (LayoutRemap.RemappingInfo[ExistingIndex].NullViewType != Binding.NullViewType);

            bool bNameConflict = false;
        #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
            bNameConflict = (LayoutRemap.DebugNames[ExistingIndex] != Binding.DebugName);
        #endif
            if (bTypeConflict || bNameConflict)
            {
                VULKAN_ERROR("Ray tracing register collision at register %u (class %d): stages disagree on the resource bound here; RT globals share one register namespace - give them distinct registers",
                    Binding.OriginalBindingIndex, static_cast<int32>(Bucket));
            }

            LayoutInfo.Bindings[ExistingIndex].stageFlags |= ShaderStage;
            continue;
        }

        const uint32 MergedBinding = static_cast<uint32>(LayoutInfo.Bindings.Size());

        VkDescriptorSetLayoutBinding LayoutBinding = {};
        LayoutBinding.descriptorCount    = 1;
        LayoutBinding.binding            = MergedBinding;
        LayoutBinding.pImmutableSamplers = nullptr;
        LayoutBinding.stageFlags         = ShaderStage;
        LayoutBinding.descriptorType     = DescriptorType;

        LayoutInfo.Bindings.Add(LayoutBinding);

        FVulkanDescriptorRemappingInfo::FRemappingInfo RemappingInfo = {};
        RemappingInfo.BindingType          = Binding.BindingType;
        RemappingInfo.BindingIndex         = static_cast<uint8>(MergedBinding);
        RemappingInfo.NullViewType         = Binding.NullViewType;
        RemappingInfo.OriginalBindingIndex = Binding.OriginalBindingIndex;

        LayoutRemap.RemappingInfo.Add(RemappingInfo);
    #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
        LayoutRemap.DebugNames.Add(Binding.DebugName);
    #endif
    }

    if (ShaderInfo.UsesBindlessHeap())
    {
        bAnyStageUsesBindless = true;
    }
}

void FVulkanPipelineLayoutInfo::PromoteUniformBuffersToDynamic()
{
    // AMD RDNA: 4 DWORDs per dynamic UB with robust buffer access, 2 without
    const uint32 DynamicUBCostDwords = GVulkanRobustBufferAccessEnabled ? 4 : 2;

    uint32 BaseCost = SetLayoutInfos.Size() + ConstantsInfo.NumConstants;
    int32 RemainingBudget = static_cast<int32>(VULKAN_RECOMMENDED_MAX_USER_DATA_DWORDS) - static_cast<int32>(BaseCost);

    for (int32 SetIndex = 0; SetIndex < SetLayoutInfos.Size(); SetIndex++)
    {
        FVulkanDescriptorSetLayoutInfo& SetLayoutInfo   = SetLayoutInfos[SetIndex];
        FVulkanDescriptorRemappingInfo& SetRemappingInfo = SetLayoutRemappings[SetIndex];

        CHECK(SetLayoutInfo.Bindings.Size() == SetRemappingInfo.RemappingInfo.Size());

        for (int32 BindingIndex = 0; BindingIndex < SetLayoutInfo.Bindings.Size(); BindingIndex++)
        {
            VkDescriptorSetLayoutBinding& Binding = SetLayoutInfo.Bindings[BindingIndex];
            if (Binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            {
                continue;
            }

            if (RemainingBudget < static_cast<int32>(DynamicUBCostDwords))
            {
                return;
            }

            Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            SetRemappingInfo.RemappingInfo[BindingIndex].BindingType = EVulkanBindingType::UniformBufferDynamic;
            RemainingBudget -= DynamicUBCostDwords;
        }
    }
}

void FVulkanPipelineLayoutInfo::ApplyImmutableSamplers(FVulkanDevice* Device, const TArrayView<const FRHIStaticSamplerInfo>& StaticSamplers)
{
    if (StaticSamplers.Size() == 0)
    {
        return;
    }

    for (int32 SetIndex = 0; SetIndex < SetLayoutInfos.Size(); SetIndex++)
    {
        FVulkanDescriptorSetLayoutInfo& SetLayout = SetLayoutInfos[SetIndex];
        if (SetLayout.ImmutableSamplers.Size() == 0 && SetLayout.Bindings.Size() > 0)
        {
            SetLayout.ImmutableSamplers.Resize(SetLayout.Bindings.Size());
            Memory::Memzero(SetLayout.ImmutableSamplers.Data(), SetLayout.ImmutableSamplers.SizeInBytes());
        }
    }

    for (const FRHIStaticSamplerInfo& StaticSampler : StaticSamplers)
    {
        VkSampler Sampler = VK_NULL_HANDLE;
        if (!Device->FindOrCreateSampler(StaticSampler, Sampler))
        {
            VULKAN_ERROR("Failed to create immutable sampler for register s%u", StaticSampler.ShaderRegister);
            continue;
        }

        const VkShaderStageFlags TargetStageFlags = GetVkStageFlagsFromShaderStage(StaticSampler.ShaderVisibility);

        for (int32 SetIndex = 0; SetIndex < SetLayoutInfos.Size(); SetIndex++)
        {
            FVulkanDescriptorSetLayoutInfo& SetLayout = SetLayoutInfos[SetIndex];
            FVulkanDescriptorRemappingInfo& SetRemap  = SetLayoutRemappings[SetIndex];

            for (int32 BindingIndex = 0; BindingIndex < SetLayout.Bindings.Size(); BindingIndex++)
            {
                const VkDescriptorSetLayoutBinding& Binding = SetLayout.Bindings[BindingIndex];
                const FVulkanDescriptorRemappingInfo::FRemappingInfo& Remap = SetRemap.RemappingInfo[BindingIndex];

                if (Binding.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER)
                {
                    continue;
                }

                if (Remap.OriginalBindingIndex != StaticSampler.ShaderRegister)
                {
                    continue;
                }

                if (TargetStageFlags != 0 && (Binding.stageFlags & TargetStageFlags) == 0)
                {
                    continue;
                }

                SetLayout.ImmutableSamplers[BindingIndex] = Sampler;
                SetRemap.RemappingInfo[BindingIndex].BindingType = EVulkanBindingType::ImmutableSampler;
            }
        }
    }
}

FVulkanPipelineLayout::FVulkanPipelineLayout(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , LayoutHandle(VK_NULL_HANDLE)
    , BindlessSetLayoutHandle(VK_NULL_HANDLE)
    , TotalDynamicOffsets(0)
    , RegularSetCount(0)
    , bHasBindlessSet(false)
{
}

FVulkanPipelineLayout::~FVulkanPipelineLayout()
{
    if (VULKAN_CHECK_HANDLE(LayoutHandle))
    {
        vkDestroyPipelineLayout(GetDevice()->GetVkDevice(), LayoutHandle, nullptr);
        LayoutHandle = VK_NULL_HANDLE;
    }
}

bool FVulkanPipelineLayout::Initialize(const FVulkanPipelineLayoutInfo& LayoutInfo)
{
    TArray<VkDescriptorSetLayout> RegularLayouts;
    RegularLayouts.Reserve(LayoutInfo.SetLayoutInfos.Size());

    FVulkanPipelineLayoutManager& PipelineLayoutManager = GetDevice()->GetPipelineLayoutManager();
    for (int32 SetIndex = 0; SetIndex < LayoutInfo.SetLayoutInfos.Size(); SetIndex++)
    {
        const FVulkanDescriptorSetLayoutInfo& SetLayoutInfo = LayoutInfo.SetLayoutInfos[SetIndex];

        VkDescriptorSetLayout NewSetLayout = PipelineLayoutManager.FindOrCreateSetLayouts(SetLayoutInfo);
        if (VULKAN_CHECK_HANDLE(NewSetLayout))
        {
            RegularLayouts.Add(NewSetLayout);
        }
        else
        {
            return false;
        }
    }

    RegularSetCount = static_cast<uint32>(RegularLayouts.Size());
    bHasBindlessSet = false;
    BindlessSetLayoutHandle = VK_NULL_HANDLE;

    FVulkanBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();

    const bool bAttachBindless = LayoutInfo.bAnyStageUsesBindless && (BindlessManager != nullptr) && BindlessManager->IsEnabled();
    if (bAttachBindless)
    {
        BindlessSetLayoutHandle = BindlessManager->GetLayout();
        bHasBindlessSet = VULKAN_CHECK_HANDLE(BindlessSetLayoutHandle);
    }

    if (LayoutInfo.bAnyStageUsesBindless && !bHasBindlessSet)
    {
        VULKAN_ERROR("FVulkanPipelineLayout: Shader indexes the bindless heap but no bindless descriptor set is available");
        return false;
    }

    TArray<VkDescriptorSetLayout> SetLayouts;
    SetLayouts.Reserve(RegularLayouts.Size() + (bHasBindlessSet ? 1 : 0));
    
    if (bHasBindlessSet)
    {
        SetLayouts.Add(BindlessSetLayoutHandle);
    }

    for (VkDescriptorSetLayout RegularLayout : RegularLayouts)
    {
        SetLayouts.Add(RegularLayout);
    }

    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
    PipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutCreateInfo.setLayoutCount = SetLayouts.Size();
    PipelineLayoutCreateInfo.pSetLayouts    = SetLayouts.Data();

    // Create ConstantRange
    VkPushConstantRange ConstantRange;
    if (LayoutInfo.ConstantsInfo.NumConstants > 0)
    {
        ConstantRange.offset     = 0;
        ConstantRange.size       = LayoutInfo.ConstantsInfo.NumConstants * sizeof(uint32);
        ConstantRange.stageFlags = LayoutInfo.ConstantsInfo.StageFlags;

        PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        PipelineLayoutCreateInfo.pPushConstantRanges    = &ConstantRange;
    }
    else
    {
        PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        PipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;
    }

    VkResult Result = vkCreatePipelineLayout(GetDevice()->GetVkDevice(), &PipelineLayoutCreateInfo, nullptr, &LayoutHandle);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create PipelineLayout");
        return false;
    }
    else
    {
        // Ensure that the information about constants are stored
        ConstantsInfo = LayoutInfo.ConstantsInfo;

        RegularSetLayoutHandles = Move(RegularLayouts);

        // Ensure that the remapping info is copied for later use
        SetLayoutRemappings = LayoutInfo.SetLayoutRemappings;

        // Count dynamic offsets per descriptor set
        DynamicOffsetCounts.Resize(LayoutInfo.SetLayoutInfos.Size());
        TotalDynamicOffsets = 0;

        for (int32 SetIndex = 0; SetIndex < LayoutInfo.SetLayoutInfos.Size(); SetIndex++)
        {
            uint32 DynamicCount = 0;
            for (const VkDescriptorSetLayoutBinding& Binding : LayoutInfo.SetLayoutInfos[SetIndex].Bindings)
            {
                if (Binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                    Binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
                {
                    DynamicCount++;
                }
            }

            DynamicOffsetCounts[SetIndex] = DynamicCount;
            TotalDynamicOffsets += DynamicCount;
        }
    }

    // Calculate user data cost in DWORDs (AMD RDNA):
    //   Descriptor sets  = 1 DWORD each
    //   Push constants   = 1 DWORD per 4 bytes
    //   Dynamic buffers  = 4 DWORDs each (with robust buffer access) or 2 DWORDs (without)

    const uint32 DynamicBufferCostDwords = GVulkanRobustBufferAccessEnabled ? 4 : 2;
    const uint32 TotalSetCount = RegularSetLayoutHandles.Size() + (bHasBindlessSet ? 1u : 0u);
    uint32 UserDataCostDwords = 0;
    UserDataCostDwords += TotalSetCount;
    UserDataCostDwords += LayoutInfo.ConstantsInfo.NumConstants;
    UserDataCostDwords += TotalDynamicOffsets * DynamicBufferCostDwords;

    if (UserDataCostDwords > VULKAN_RECOMMENDED_MAX_USER_DATA_DWORDS)
    {
        LOG_WARNING("[FVulkanPipelineLayout] UserDataCost=%u DWORDs exceeds recommended %u (Sets=%u [Bindless=%s], PushConstants=%u, DynamicBuffers=%u)",
            UserDataCostDwords, VULKAN_RECOMMENDED_MAX_USER_DATA_DWORDS, TotalSetCount, 
            bHasBindlessSet ? "yes" : "no", LayoutInfo.ConstantsInfo.NumConstants, TotalDynamicOffsets);
    }
    else
    {
        LOG_INFO("[FVulkanPipelineLayout] UserDataCost=%u DWORDs (Sets=%u [Bindless=%s], PushConstants=%u, DynamicBuffers=%u)",
            UserDataCostDwords, TotalSetCount, bHasBindlessSet ? "yes" : "no", LayoutInfo.ConstantsInfo.NumConstants, TotalDynamicOffsets);
    }

    SetupResourceMapping(LayoutInfo);
    return true;
}

void FVulkanPipelineLayout::SetDebugName(const CHAR* InName)
{
    if (InName)
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), InName, LayoutHandle, VK_OBJECT_TYPE_PIPELINE_LAYOUT);
    #if VULKAN_STORE_DEBUG_NAMES
        DebugName = InName;
    #endif
    }
}

bool FVulkanPipelineLayout::GetDescriptorBinding(EShaderVisibility::Type ShaderStage, EResourceType::Type ResourceType, int32 ResourceIndex, uint32& OutDescriptorSetIndex, uint32& OutBinding)
{
    FStageDescriptorMap& StageMapping = DescriptorBindMap[ShaderStage];
    if (StageMapping.DescriptorSetIndex == UINT8_MAX)
    {
        return false;
    }

    switch(ResourceType)
    {
    case EResourceType::SRV:
        OutBinding = ResourceIndex < VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT ? StageMapping.SRVMappings[ResourceIndex] : UINT8_MAX;
        break;

    case EResourceType::UAV:
        OutBinding = ResourceIndex < VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT ? StageMapping.UAVMappings[ResourceIndex] : UINT8_MAX;
        break;

    case EResourceType::UniformBuffer:
        OutBinding = ResourceIndex < VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT ? StageMapping.UniformMappings[ResourceIndex] : UINT8_MAX;
        break;

    case EResourceType::Sampler:
        OutBinding = ResourceIndex < VULKAN_DEFAULT_SAMPLER_STATE_COUNT ? StageMapping.SamplerMappings[ResourceIndex] : UINT8_MAX;
        break;
    };

    if (OutBinding == UINT8_MAX)
    {
        return false;
    }

    OutDescriptorSetIndex = StageMapping.DescriptorSetIndex;
    return true;
}

bool FVulkanPipelineLayout::GetDescriptorSetIndex(EShaderVisibility::Type ShaderStage, uint32& OutDescriptorSetIndex)
{
    FStageDescriptorMap& StageMapping = DescriptorBindMap[ShaderStage];
    if (StageMapping.DescriptorSetIndex != UINT8_MAX)
    {
        const uint32 BindlessOffset = bHasBindlessSet ? 1u : 0u;
        OutDescriptorSetIndex = static_cast<uint32>(StageMapping.DescriptorSetIndex) + BindlessOffset;
        return true;
    }
    else
    {
        return false;
    }
}

bool FVulkanPipelineLayout::GetRemappedBinding(EShaderVisibility::Type ShaderStage, EVulkanBindingType::Type BindingType,
                                               uint16 OriginalBindingIndex, uint32& OutBinding) const
{
    const FStageDescriptorMap& StageMapping = DescriptorBindMap[ShaderStage];
    if (StageMapping.DescriptorSetIndex == UINT8_MAX)
    {
        return false;
    }

    const EResourceType::Type             Bucket = GetResourceBucket(BindingType);
    const FVulkanDescriptorRemappingInfo& Remap  = SetLayoutRemappings[StageMapping.DescriptorSetIndex];

    for (const FVulkanDescriptorRemappingInfo::FRemappingInfo& Info : Remap.RemappingInfo)
    {
        if (Info.OriginalBindingIndex == OriginalBindingIndex && GetResourceBucket(Info.BindingType) == Bucket)
        {
            OutBinding = Info.BindingIndex;
            return true;
        }
    }

    return false;
}

void FVulkanPipelineLayout::SetupResourceMapping(const FVulkanPipelineLayoutInfo& LayoutInfo)
{
    CHECK(LayoutInfo.SetLayoutInfos.Size() == LayoutInfo.SetLayoutRemappings.Size());

    // Initialize the mapping to zero
    for (FStageDescriptorMap& StageMapping : DescriptorBindMap)
    {
        StageMapping.DescriptorSetIndex = UINT8_MAX;

        Memory::Memset(StageMapping.SRVMappings, UINT8_MAX, sizeof(StageMapping.SRVMappings));
        Memory::Memset(StageMapping.UAVMappings, UINT8_MAX, sizeof(StageMapping.UAVMappings));
        Memory::Memset(StageMapping.UniformMappings, UINT8_MAX, sizeof(StageMapping.UniformMappings));
        Memory::Memset(StageMapping.SamplerMappings, UINT8_MAX, sizeof(StageMapping.SamplerMappings));
    }

    // Initialize the actual DescriptorBinding mapping
    for (int32 SetIndex = 0; SetIndex < LayoutInfo.SetLayoutInfos.Size(); SetIndex++)
    {
        const FVulkanDescriptorSetLayoutInfo& SetLayoutInfo    = LayoutInfo.SetLayoutInfos[SetIndex];
        const FVulkanDescriptorRemappingInfo& StageMappingInfo = LayoutInfo.SetLayoutRemappings[SetIndex];

        CHECK(SetLayoutInfo.Bindings.Size() == StageMappingInfo.RemappingInfo.Size());
        for (int32 BindingIndex = 0; BindingIndex < SetLayoutInfo.Bindings.Size(); BindingIndex++)
        {
            const EShaderVisibility::Type ShaderVisibility = GetShaderVisibilityFromShaderFlag(SetLayoutInfo.Bindings[BindingIndex].stageFlags);

            FStageDescriptorMap& StageMapping = DescriptorBindMap[ShaderVisibility];
            StageMapping.DescriptorSetIndex = static_cast<uint8>(SetIndex);

            const FVulkanDescriptorRemappingInfo::FRemappingInfo& RemappingInfo = StageMappingInfo.RemappingInfo[BindingIndex];

            const auto CheckSlotCollision = [&](uint8 Slot, const CHAR* BucketName)
            {
                UNREFERENCED_VARIABLE(BucketName);
                if (Slot != UINT8_MAX && Slot != static_cast<uint8>(BindingIndex))
                {
                    VULKAN_ERROR("Register namespace collision: %s register %u already maps to binding %u, now %u (RT stages must agree on the resource per register)",
                        BucketName, RemappingInfo.OriginalBindingIndex, Slot, BindingIndex);
                }
            };

            switch(RemappingInfo.BindingType)
            {
            case EVulkanBindingType::UniformBuffer:
            case EVulkanBindingType::UniformBufferDynamic:
                CheckSlotCollision(StageMapping.UniformMappings[RemappingInfo.OriginalBindingIndex], "CBV");
                StageMapping.UniformMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
                break;

            case EVulkanBindingType::Sampler:
                CheckSlotCollision(StageMapping.SamplerMappings[RemappingInfo.OriginalBindingIndex], "Sampler");
                StageMapping.SamplerMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
                break;

            case EVulkanBindingType::SampledImage:
            case EVulkanBindingType::StorageBufferRead:
            case EVulkanBindingType::AccelerationStructure:
                CheckSlotCollision(StageMapping.SRVMappings[RemappingInfo.OriginalBindingIndex], "SRV");
                StageMapping.SRVMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
                break;

            case EVulkanBindingType::StorageImage:
            case EVulkanBindingType::StorageBufferReadWrite:
                CheckSlotCollision(StageMapping.UAVMappings[RemappingInfo.OriginalBindingIndex], "UAV");
                StageMapping.UAVMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
                break;

            case EVulkanBindingType::ImmutableSampler:
                break;

            default:
                DEBUG_BREAK();
                break;
            }
        }
    }
}

FVulkanPipelineLayoutManager::FVulkanPipelineLayoutManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Layouts()
    , LayoutsCS()
{
}

FVulkanPipelineLayoutManager::~FVulkanPipelineLayoutManager()
{
    {
        TScopedLock Lock(LayoutsCS);

        for (const auto& LayoutPair : Layouts)
        {
            delete LayoutPair.Second;
        }

        Layouts.Clear();
    }

    {
        TScopedLock Lock(SetLayoutsCS);

        for (const auto& SetLayoutPair : SetLayouts)
        {
            if (VULKAN_CHECK_HANDLE(SetLayoutPair.Second))
            {
                vkDestroyDescriptorSetLayout(GetDevice()->GetVkDevice(), SetLayoutPair.Second, nullptr);
                SetLayoutPair.Second = VK_NULL_HANDLE;
            }
        }

        SetLayouts.Clear();
    }
}

FVulkanPipelineLayout* FVulkanPipelineLayoutManager::FindOrCreateLayout(const FVulkanPipelineLayoutInfo& LayoutInfo)
{
    TScopedLock Lock(LayoutsCS);

    if (FVulkanPipelineLayout** ExistingLayout = Layouts.Find(LayoutInfo))
    {
        return *ExistingLayout;
    }
    
    FVulkanPipelineLayout* NewLayout = new FVulkanPipelineLayout(GetDevice());
    if (NewLayout->Initialize(LayoutInfo))
    {
        // Set a debug-name for the pipeline-layout
        const String DebugName = String::CreateFormatted("PipelineLayout %d", Layouts.Size());
        NewLayout->SetDebugName(DebugName.Data());
        Layouts.Add(LayoutInfo, NewLayout);

        LOG_INFO("Created a new PipelineLayout NumPipelineLayouts=%d", Layouts.Size());
        return NewLayout;
    }
    else
    {
        return nullptr;
    }
}

VkDescriptorSetLayout FVulkanPipelineLayoutManager::FindOrCreateSetLayouts(const FVulkanDescriptorSetLayoutInfo& SetLayoutInfo)
{
    TScopedLock Lock(SetLayoutsCS);

    if (VkDescriptorSetLayout* ExistingSetLayout = SetLayouts.Find(SetLayoutInfo))
    {
        return *ExistingSetLayout;
    }

    // Build a local copy of bindings with pImmutableSamplers pointers resolved.
    // The stored Bindings always have pImmutableSamplers=nullptr (raw pointers don't survive copy/hash);
    // the actual VkSampler handles live in the ImmutableSamplers array.

    const bool bHasImmutableSamplers = SetLayoutInfo.ImmutableSamplers.Size() > 0;
    TArray<VkDescriptorSetLayoutBinding> ResolvedBindings;

    if (bHasImmutableSamplers)
    {
        ResolvedBindings = SetLayoutInfo.Bindings;
        for (int32 i = 0; i < ResolvedBindings.Size(); i++)
        {
            if (SetLayoutInfo.ImmutableSamplers[i] != VK_NULL_HANDLE)
            {
                ResolvedBindings[i].pImmutableSamplers = &SetLayoutInfo.ImmutableSamplers[i];
            }
        }
    }

    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {};
    DescriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorSetLayoutCreateInfo.bindingCount = SetLayoutInfo.Bindings.Size();
    DescriptorSetLayoutCreateInfo.pBindings    = bHasImmutableSamplers ? ResolvedBindings.Data() : SetLayoutInfo.Bindings.Data();

    VkDescriptorSetLayout NewSetLayout = VK_NULL_HANDLE;
    VkResult Result = vkCreateDescriptorSetLayout(GetDevice()->GetVkDevice(), &DescriptorSetLayoutCreateInfo, nullptr, &NewSetLayout);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create DescriptorSetLayout");
        return VK_NULL_HANDLE;
    }
    else
    {
        SetLayouts.Add(SetLayoutInfo, NewSetLayout);
        return NewSetLayout;
    }
}
