#include "VulkanPipelineLayout.h"

FVulkanPipelineLayout::FVulkanPipelineLayout(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , LayoutHandle(VK_NULL_HANDLE)
    , DescriptorSetLayoutHandles()
{
}

FVulkanPipelineLayout::~FVulkanPipelineLayout()
{
    if (VULKAN_CHECK_HANDLE(LayoutHandle))
    {
        vkDestroyPipelineLayout(GetDevice()->GetVkDevice(), LayoutHandle, nullptr);
        LayoutHandle = VK_NULL_HANDLE;
    }
    
    for (VkDescriptorSetLayout DescriptorSetLayout : DescriptorSetLayoutHandles)
    {
        if (VULKAN_CHECK_HANDLE(DescriptorSetLayout))
        {
            vkDestroyDescriptorSetLayout(GetDevice()->GetVkDevice(), DescriptorSetLayout, nullptr);
            DescriptorSetLayout = VK_NULL_HANDLE;
        }
    }
}

bool FVulkanPipelineLayout::Initialize(const FVulkanPipelineLayoutCreateInfo& LayoutCreateInfo)
{
    /*// ShaderStages
    VkShaderStageFlagBits ShaderVisibility[] =
    {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
        VK_SHADER_STAGE_GEOMETRY_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorType DescriptorType[] =
    {
        // ConstantBuffers
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // Binding 0
        // SRV + UAV Buffers
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // Binding 8
        // Samplers
        VK_DESCRIPTOR_TYPE_SAMPLER,        // Binding 24
        // UAV Textures
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // Binding 32
    };

    // Create Descriptor Bindings
    TArray<VkDescriptorSetLayoutBinding> LayoutBindings;
    for (uint32 ShaderVisibilityIndex = 0; ShaderVisibilityIndex < ARRAY_COUNT(ShaderVisibility); ShaderVisibilityIndex++)
    {
        // Clear layout bindings for each DescriptorSet
        LayoutBindings.Clear();

        // ConstantBuffers, SRV+UAV Buffers, Samplers, UAV Images
        int32 BindingsStartIndex = 0;
        for (uint32 DescriptorTypeIndex = 0; DescriptorTypeIndex < ARRAY_COUNT(DescriptorType); DescriptorTypeIndex++)
        {
            VkDescriptorType CurrentDescriptorType = DescriptorType[DescriptorTypeIndex];
            BindingsStartIndex = LayoutBindings.Size();

            // StorageBuffers needs to have SRV+UAV number of bindings
            constexpr uint32 NumStorageBufferBindings     = VULKAN_DEFAULT_NUM_DESCRIPTOR_BINDINGS + VULKAN_DEFAULT_NUM_DESCRIPTOR_BINDINGS;
            constexpr uint32 NumDefaultDescriptorBindings = VULKAN_DEFAULT_NUM_DESCRIPTOR_BINDINGS;

            const uint32 NumBindings = CurrentDescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ? NumStorageBufferBindings : NumDefaultDescriptorBindings;
            for (uint32 Index = 0; Index < NumBindings; Index++)
            {
                VkDescriptorSetLayoutBinding LayoutBinding;
                LayoutBinding.descriptorCount    = 1;
                LayoutBinding.binding            = BindingsStartIndex + Index;
                LayoutBinding.pImmutableSamplers = nullptr;
                LayoutBinding.stageFlags         = ShaderVisibility[ShaderVisibilityIndex];
                LayoutBinding.descriptorType     = CurrentDescriptorType;
                LayoutBindings.Add(LayoutBinding);
            }
        }

        // Texture (SRV) bindings
        BindingsStartIndex = LayoutBindings.Size();
        for (uint32 Index = 0; Index < VULKAN_DEFAULT_NUM_SAMPLED_IMAGE_DESCRIPTOR_BINDINGS; Index++)
        {
            // SRV Textures
            VkDescriptorSetLayoutBinding LayoutBinding;
            LayoutBinding.descriptorCount    = 1;
            LayoutBinding.binding            = BindingsStartIndex + Index;
            LayoutBinding.pImmutableSamplers = nullptr;
            LayoutBinding.stageFlags         = ShaderVisibility[ShaderVisibilityIndex];
            LayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            LayoutBindings.Add(LayoutBinding);
        }

        // Create DescriptorSetLayout
        VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
        FMemory::Memzero(&DescriptorSetLayoutCreateInfo);

        DescriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        DescriptorSetLayoutCreateInfo.bindingCount = LayoutBindings.Size();
        DescriptorSetLayoutCreateInfo.pBindings    = LayoutBindings.Data();

        VkResult Result = vkCreateDescriptorSetLayout(GetDevice()->GetVkDevice(), &DescriptorSetLayoutCreateInfo, nullptr, &DescriptorSetLayouts[ShaderVisibilityIndex]);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("Failed to create DescriptorSetLayout");
            return false;
        }
    }

    // Create ConstantRange
    VkPushConstantRange ConstantRange;
    ConstantRange.size       = VULKAN_MAX_NUM_PUSH_CONSTANTS * sizeof(uint32);
    ConstantRange.offset     = 0;
    ConstantRange.stageFlags = VK_SHADER_STAGE_ALL;

    // Create PipelineLayout
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
    FMemory::Memzero(&PipelineLayoutCreateInfo);

    PipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutCreateInfo.setLayoutCount         = ARRAY_COUNT(DescriptorSetLayouts);
    PipelineLayoutCreateInfo.pSetLayouts            = DescriptorSetLayouts;
    PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    PipelineLayoutCreateInfo.pPushConstantRanges    = &ConstantRange;

    VkResult Result = vkCreatePipelineLayout(GetDevice()->GetVkDevice(), &PipelineLayoutCreateInfo, nullptr, &PipelineLayout);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create PipelineLayout");
        return false;
    }*/
    
    return true;
}


FVulkanPipelineLayoutManager::FVulkanPipelineLayoutManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
{
}

FVulkanPipelineLayoutManager::~FVulkanPipelineLayoutManager()
{
}

FVulkanPipelineLayout* FVulkanPipelineLayoutManager::GetOrCreateLayout(const FVulkanPipelineLayoutCreateInfo& PipelineLayoutCreateInfo)
{
    return nullptr;
}
