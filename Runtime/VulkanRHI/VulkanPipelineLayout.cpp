#include "VulkanPipelineLayout.h"

FVulkanPipelineLayout::FVulkanPipelineLayout(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , LayoutHandle(VK_NULL_HANDLE)
    , DescriptorSetLayoutHandles()
{
    FMemory::Memzero(DescriptorSetLayoutHandles, sizeof(DescriptorSetLayoutHandles));
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

bool FVulkanPipelineLayout::Initialize(const FVulkanPipelineLayoutCreateInfo& CreateInfo)
{
    // ShaderStages
    const VkShaderStageFlagBits ShaderVisibility[] =
    {
        VK_SHADER_STAGE_VERTEX_BIT,                  // ShaderVisibility_Vertex
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,    // ShaderVisibility_Hull
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, // ShaderVisibility_Domain
        VK_SHADER_STAGE_GEOMETRY_BIT,                // ShaderVisibility_Geometry
        VK_SHADER_STAGE_FRAGMENT_BIT,                // ShaderVisibility_Pixel
        VK_SHADER_STAGE_COMPUTE_BIT,                 // ShaderVisibility_Compute
    };

    const VkDescriptorType DescriptorType[] =
    {
        // ConstantBuffers
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // Binding 0
        // SRV + UAV Buffers
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // Binding 8
        // Samplers
        VK_DESCRIPTOR_TYPE_SAMPLER,        // Binding 24
        // UAV Textures
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // Binding 32
        // SRV Images
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  // Binding 40
    };

    // Create Descriptor Bindings
    TArray<VkDescriptorSetLayout>        Layouts;
    TArray<VkDescriptorSetLayoutBinding> LayoutBindings;

    for (uint32 SetIndex = 0; SetIndex < CreateInfo.NumSetLayouts; SetIndex++)
    {
        // Clear layout bindings for each DescriptorSet
        LayoutBindings.Clear();
        
        const FDescriptorSetLayout& SetLayoutInfo = CreateInfo.StageSetLayouts[SetIndex];
        CHECK(SetLayoutInfo.ShaderStage < ARRAY_COUNT(ShaderVisibility));

        // LookUp table for descriptor counts
        const uint32 DescriptorTypeBindingCount[] =
        {
            SetLayoutInfo.NumUniformBuffers,
            SetLayoutInfo.NumStorageBuffers,
            SetLayoutInfo.NumSamplers,
            SetLayoutInfo.NumStorageImages,
            SetLayoutInfo.NumImages
        };

        // Setup all the bindings
        int32 BindingsStartIndex = 0;
        for (uint32 DescriptorTypeIndex = 0; DescriptorTypeIndex < ARRAY_COUNT(DescriptorType); DescriptorTypeIndex++)
        {
            VkDescriptorType CurrentDescriptorType = DescriptorType[DescriptorTypeIndex];
            BindingsStartIndex = LayoutBindings.Size();

            const uint32 NumBindings = DescriptorTypeBindingCount[DescriptorTypeIndex];
            for (uint32 Index = 0; Index < NumBindings; Index++)
            {
                VkDescriptorSetLayoutBinding LayoutBinding;
                LayoutBinding.descriptorCount    = 1;
                LayoutBinding.binding            = BindingsStartIndex + Index;
                LayoutBinding.pImmutableSamplers = nullptr;
                LayoutBinding.stageFlags         = ShaderVisibility[SetLayoutInfo.ShaderStage];
                LayoutBinding.descriptorType     = CurrentDescriptorType;
                LayoutBindings.Add(LayoutBinding);
            }
        }

        VkDescriptorSetLayout NewSetLayout = VK_NULL_HANDLE;
        if (!LayoutBindings.IsEmpty())
        {
            // Create DescriptorSetLayout
            VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
            FMemory::Memzero(&DescriptorSetLayoutCreateInfo);

            DescriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            DescriptorSetLayoutCreateInfo.bindingCount = LayoutBindings.Size();
            DescriptorSetLayoutCreateInfo.pBindings    = LayoutBindings.Data();

            VkResult Result = vkCreateDescriptorSetLayout(GetDevice()->GetVkDevice(), &DescriptorSetLayoutCreateInfo, nullptr, &NewSetLayout);
            if (VULKAN_FAILED(Result))
            {
                VULKAN_ERROR("Failed to create DescriptorSetLayout");
                return false;
            }
            else
            {
                // Only set the actual handles when we create a new layout
                DescriptorSetLayoutHandles[SetLayoutInfo.ShaderStage] = Layouts.Add(NewSetLayout);
            }
        }
        else
        {
            // Default layout is only used to create the VkPipelineLayout and not when actually use the layout later when we bind/create DescriptorSets
            NewSetLayout = GetDevice()->GetPipelineLayoutManager().GetDefaultSetLayout();
            Layouts.Add(NewSetLayout);
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
    PipelineLayoutCreateInfo.setLayoutCount         = Layouts.Size();
    PipelineLayoutCreateInfo.pSetLayouts            = Layouts.Data();
    PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    PipelineLayoutCreateInfo.pPushConstantRanges    = &ConstantRange;

    VkResult Result = vkCreatePipelineLayout(GetDevice()->GetVkDevice(), &PipelineLayoutCreateInfo, nullptr, &LayoutHandle);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create PipelineLayout");
        return false;
    }
    else
    {
        return true;
    }
}


FVulkanPipelineLayoutManager::FVulkanPipelineLayoutManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , DefaultSetLayout(VK_NULL_HANDLE)
    , Layouts()
    , LayoutsCS()
{
}

FVulkanPipelineLayoutManager::~FVulkanPipelineLayoutManager()
{
    Release();
}

bool FVulkanPipelineLayoutManager::Initialize()
{
    // Create DescriptorSetLayout
    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
    FMemory::Memzero(&DescriptorSetLayoutCreateInfo);

    DescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    VkResult Result = vkCreateDescriptorSetLayout(GetDevice()->GetVkDevice(), &DescriptorSetLayoutCreateInfo, nullptr, &DefaultSetLayout);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Default DescriptorSetLayout");
        return false;
    }
    else
    {
        return true;
    }
}

void FVulkanPipelineLayoutManager::Release()
{
    if (VULKAN_CHECK_HANDLE(DefaultSetLayout))
    {
        vkDestroyDescriptorSetLayout(GetDevice()->GetVkDevice(), DefaultSetLayout, nullptr);
        DefaultSetLayout = VK_NULL_HANDLE;
    }

    Layouts.Clear();
}

TSharedRef<FVulkanPipelineLayout> FVulkanPipelineLayoutManager::CreateLayout(const FVulkanPipelineLayoutCreateInfo& CreateInfo)
{
    TScopedLock Lock(LayoutsCS);

    if (TSharedRef<FVulkanPipelineLayout>* ExistingLayout = Layouts.Find(CreateInfo))
    {
        return *ExistingLayout;
    }
    
    TSharedRef<FVulkanPipelineLayout> NewLayout = new FVulkanPipelineLayout(GetDevice());
    if (NewLayout->Initialize(CreateInfo))
    {
        Layouts.Add(CreateInfo, NewLayout);
        LOG_INFO("Created a new PipelineLayout NumPipelineLayouts=%d", Layouts.Size());
        return NewLayout;
    }
    else
    {
        return nullptr;
    }
}
