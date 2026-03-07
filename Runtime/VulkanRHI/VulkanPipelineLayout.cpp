#include "Core/Memory/Memory.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanPipelineLayout.h"
#include "VulkanRHI/VulkanConstants.h"
#include "VulkanRHI/VulkanShader.h"

static inline EShaderVisibility GetShaderVisibilityFromShaderFlag(VkShaderStageFlags ShaderStage)
{
    switch(ShaderStage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:                  return ShaderVisibility_Vertex;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    return ShaderVisibility_Hull;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return ShaderVisibility_Domain;
    case VK_SHADER_STAGE_GEOMETRY_BIT:                return ShaderVisibility_Geometry;
    case VK_SHADER_STAGE_FRAGMENT_BIT:                return ShaderVisibility_Pixel;
    case VK_SHADER_STAGE_COMPUTE_BIT:                 return ShaderVisibility_Compute;
    default:                                          return ShaderVisibility_Compute;
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

        FVulkanDescriptorRemappingInfo::FRemappingInfo RemappingInfo;
        RemappingInfo.BindingType          = Binding.BindingType;
        RemappingInfo.BindingIndex         = Binding.BindingIndex;
        RemappingInfo.OriginalBindingIndex = Binding.OriginalBindingIndex;
        LayoutRemappings.RemappingInfo.Add(RemappingInfo);

    #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
        LayoutRemappings.DebugNames.Add(Binding.DebugName);
    #endif
    }

    SetLayoutInfos.Add(Move(LayoutInfo));
    SetLayoutRemappings.Add(Move(LayoutRemappings));
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
            SetRemappingInfo.RemappingInfo[BindingIndex].BindingType = VulkanBindingType_UniformBufferDynamic;
            RemainingBudget -= DynamicUBCostDwords;
        }
    }
}

FVulkanPipelineLayout::FVulkanPipelineLayout(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , LayoutHandle(VK_NULL_HANDLE)
    , TotalDynamicOffsets(0)
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
    // Create Descriptor Bindings
    TArray<VkDescriptorSetLayout> SetLayouts;
    SetLayouts.Reserve(LayoutInfo.SetLayoutInfos.Size());
    
    FVulkanPipelineLayoutManager& PipelineLayoutManager = GetDevice()->GetPipelineLayoutManager();
    for (int32 SetIndex = 0; SetIndex < LayoutInfo.SetLayoutInfos.Size(); SetIndex++)
    {
        const FVulkanDescriptorSetLayoutInfo& SetLayoutInfo = LayoutInfo.SetLayoutInfos[SetIndex];
        
        // Retrieve a DescriptorSetLayout by creating or using a cached one
        VkDescriptorSetLayout NewSetLayout = PipelineLayoutManager.FindOrCreateSetLayouts(SetLayoutInfo);
        if (VULKAN_CHECK_HANDLE(NewSetLayout))
        {
            SetLayouts.Add(NewSetLayout);
        }
        else
        {
            return false;
        }
    }

    // Create PipelineLayout
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
        
        // Ensure that we store the layout handles
        SetLayoutHandles = Move(SetLayouts);
        
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
    uint32 UserDataCostDwords = 0;
    UserDataCostDwords += SetLayoutHandles.Size();
    UserDataCostDwords += LayoutInfo.ConstantsInfo.NumConstants;
    UserDataCostDwords += TotalDynamicOffsets * DynamicBufferCostDwords;

    if (UserDataCostDwords > VULKAN_RECOMMENDED_MAX_USER_DATA_DWORDS)
    {
        LOG_WARNING("[FVulkanPipelineLayout] UserDataCost=%u DWORDs exceeds recommended %u (Sets=%d, PushConstants=%u, DynamicBuffers=%u)", 
            UserDataCostDwords, VULKAN_RECOMMENDED_MAX_USER_DATA_DWORDS, SetLayoutHandles.Size(), LayoutInfo.ConstantsInfo.NumConstants, TotalDynamicOffsets);
    }
    else
    {
        LOG_INFO("[FVulkanPipelineLayout] UserDataCost=%u DWORDs (Sets=%d, PushConstants=%u, DynamicBuffers=%u)", 
            UserDataCostDwords, SetLayoutHandles.Size(), LayoutInfo.ConstantsInfo.NumConstants, TotalDynamicOffsets);
    }

    SetupResourceMapping(LayoutInfo);
    return true;
}

void FVulkanPipelineLayout::SetDebugName(const CHAR* InName)
{
    if (InName)
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), InName, LayoutHandle, VK_OBJECT_TYPE_PIPELINE_LAYOUT);
        DebugName = InName;
    }
}

bool FVulkanPipelineLayout::GetDescriptorBinding(EShaderVisibility ShaderStage, EResourceType ResourceType, int32 ResourceIndex, uint32& OutDescriptorSetIndex, uint32& OutBinding)
{
    FStageDescriptorMap& StageMapping = DescriptorBindMap[ShaderStage];
    if (StageMapping.DescriptorSetIndex == UINT8_MAX)
    {
        return false;
    }

    switch(ResourceType)
    {
    case ResourceType_SRV:
        OutBinding = ResourceIndex < VULKAN_DEFAULT_SHADER_RESOURCE_VIEW_COUNT ? StageMapping.SRVMappings[ResourceIndex] : UINT8_MAX;
        break;
    case ResourceType_UAV:
        OutBinding = ResourceIndex < VULKAN_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT ? StageMapping.UAVMappings[ResourceIndex] : UINT8_MAX;
        break;
    case ResourceType_UniformBuffer:
        OutBinding = ResourceIndex < VULKAN_DEFAULT_UNIFORM_BUFFER_COUNT ? StageMapping.UniformMappings[ResourceIndex] : UINT8_MAX;
        break;
    case ResourceType_Sampler:
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

bool FVulkanPipelineLayout::GetDescriptorSetIndex(EShaderVisibility ShaderStage, uint32& OutDescriptorSetIndex)
{
    FStageDescriptorMap& StageMapping = DescriptorBindMap[ShaderStage];
    if (StageMapping.DescriptorSetIndex != UINT8_MAX)
    {
        OutDescriptorSetIndex = StageMapping.DescriptorSetIndex;
        return true;
    }
    else
    {
        return false;
    }
}

void FVulkanPipelineLayout::SetupResourceMapping(const FVulkanPipelineLayoutInfo& LayoutInfo)
{
    CHECK(LayoutInfo.SetLayoutInfos.Size() == LayoutInfo.SetLayoutRemappings.Size());

    // Initialize the mapping to zero
    for (FStageDescriptorMap& StageMapping : DescriptorBindMap)
    {
        StageMapping.DescriptorSetIndex = UINT8_MAX;
        FMemory::Memset(StageMapping.SRVMappings, UINT8_MAX, sizeof(StageMapping.SRVMappings));
        FMemory::Memset(StageMapping.UAVMappings, UINT8_MAX, sizeof(StageMapping.UAVMappings));
        FMemory::Memset(StageMapping.UniformMappings, UINT8_MAX, sizeof(StageMapping.UniformMappings));
        FMemory::Memset(StageMapping.SamplerMappings, UINT8_MAX, sizeof(StageMapping.SamplerMappings));
    }

    // Initialize the actual DescriptorBinding mapping
    for (int32 SetIndex = 0; SetIndex < LayoutInfo.SetLayoutInfos.Size(); SetIndex++)
    {
        const FVulkanDescriptorSetLayoutInfo& SetLayoutInfo    = LayoutInfo.SetLayoutInfos[SetIndex];
        const FVulkanDescriptorRemappingInfo& StageMappingInfo = LayoutInfo.SetLayoutRemappings[SetIndex];

        CHECK(SetLayoutInfo.Bindings.Size() == StageMappingInfo.RemappingInfo.Size());
        for (int32 BindingIndex = 0; BindingIndex < SetLayoutInfo.Bindings.Size(); BindingIndex++)
        {
            const EShaderVisibility ShaderVisibility = GetShaderVisibilityFromShaderFlag(SetLayoutInfo.Bindings[BindingIndex].stageFlags);

            FStageDescriptorMap& StageMapping = DescriptorBindMap[ShaderVisibility];
            StageMapping.DescriptorSetIndex = static_cast<uint8>(SetIndex);

            const FVulkanDescriptorRemappingInfo::FRemappingInfo& RemappingInfo = StageMappingInfo.RemappingInfo[BindingIndex];
            switch(RemappingInfo.BindingType)
            {
            case VulkanBindingType_UniformBuffer:
            case VulkanBindingType_UniformBufferDynamic:
                StageMapping.UniformMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
                break;
            case VulkanBindingType_Sampler:
                StageMapping.SamplerMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
                break;
            case VulkanBindingType_SampledImage:
            case VulkanBindingType_StorageBufferRead:
                StageMapping.SRVMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
                break;
            case VulkanBindingType_StorageImage:
            case VulkanBindingType_StorageBufferReadWrite:
                StageMapping.UAVMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
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
        const FString DebugName = FString::CreateFormatted("PipelineLayout %d", Layouts.Size());
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
    
    // Create the DescriptorSetLayout or assign an "default" empty DescriptorSetLayout
    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {};
    DescriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorSetLayoutCreateInfo.bindingCount = SetLayoutInfo.Bindings.Size();
    DescriptorSetLayoutCreateInfo.pBindings    = SetLayoutInfo.Bindings.Data();

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
