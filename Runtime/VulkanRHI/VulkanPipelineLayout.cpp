#include "VulkanPipelineLayout.h"
#include "VulkanShader.h"
#include "Core/Memory/Memory.h"

static inline EShaderVisibility GetShaderVisibilityFromShaderFlag(VkShaderStageFlags ShaderStage)
{
    switch(ShaderStage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:                  return ShaderVisibility_Vertex;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:    return ShaderVisibility_Domain;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return ShaderVisibility_Hull;
    case VK_SHADER_STAGE_GEOMETRY_BIT:                return ShaderVisibility_Geometry;
    case VK_SHADER_STAGE_FRAGMENT_BIT:                return ShaderVisibility_Pixel;
    case VK_SHADER_STAGE_COMPUTE_BIT:
    default: return ShaderVisibility_Compute;
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

    for (const FVulkanShaderInfo::FResourceBinding& Binding : ShaderInfo.ResourceBindings)
    {
        VkDescriptorSetLayoutBinding LayoutBinding;
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
    }

    SetLayoutInfos.Add(Move(LayoutInfo));
    SetLayoutRemappings.Add(Move(LayoutRemappings));
}

FVulkanPipelineLayout::FVulkanPipelineLayout(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , LayoutHandle(VK_NULL_HANDLE)
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
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
    FMemory::Memzero(&PipelineLayoutCreateInfo);

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
        VULKAN_ERROR("Failed to create PipelineLayout");
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
    }

    // Store the number of push constants for use later when binding push-constants
    SetupResourceMapping(LayoutInfo);
    return true;
}

void FVulkanPipelineLayout::SetDebugName(const CHAR* InName)
{
    FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName, LayoutHandle, VK_OBJECT_TYPE_PIPELINE_LAYOUT);
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
            case BindingType_UniformBuffer:
                StageMapping.UniformMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
                break;
            case BindingType_Sampler:
                StageMapping.SamplerMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
                break;
            case BindingType_SampledImage:
            case BindingType_StorageBufferRead:
                StageMapping.SRVMappings[RemappingInfo.OriginalBindingIndex] = static_cast<uint8>(BindingIndex);
                break;
            case BindingType_StorageImage:
            case BindingType_StorageBufferReadWrite:
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
    Release();
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
    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
    FMemory::Memzero(&DescriptorSetLayoutCreateInfo);

    DescriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorSetLayoutCreateInfo.bindingCount = SetLayoutInfo.Bindings.Size();
    DescriptorSetLayoutCreateInfo.pBindings    = SetLayoutInfo.Bindings.Data();

    VkDescriptorSetLayout NewSetLayout = VK_NULL_HANDLE;
    VkResult Result = vkCreateDescriptorSetLayout(GetDevice()->GetVkDevice(), &DescriptorSetLayoutCreateInfo, nullptr, &NewSetLayout);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create DescriptorSetLayout");
        return VK_NULL_HANDLE;
    }
    else
    {
        SetLayouts.Add(SetLayoutInfo, NewSetLayout);
        return NewSetLayout;
    }
}

void FVulkanPipelineLayoutManager::Release()
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
