#include "VulkanPipelineLayout.h"

FVulkanPipelineLayout::FVulkanPipelineLayout(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , LayoutHandle(VK_NULL_HANDLE)
    , DescriptorSetLayoutHandles()
    , NumPushConstants(0)
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
    // ShaderStages Lookup-table
    const VkShaderStageFlagBits ShaderVisibility[] =
    {
        VK_SHADER_STAGE_VERTEX_BIT,                  // ShaderVisibility_Vertex
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,    // ShaderVisibility_Hull
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, // ShaderVisibility_Domain
        VK_SHADER_STAGE_GEOMETRY_BIT,                // ShaderVisibility_Geometry
        VK_SHADER_STAGE_FRAGMENT_BIT,                // ShaderVisibility_Pixel
        VK_SHADER_STAGE_COMPUTE_BIT,                 // ShaderVisibility_Compute
    };
    static_assert(ARRAY_COUNT(ShaderVisibility) == ShaderVisibility_Count, "The ShaderVisibility array is out of date");
    
    // DescriptorType Lookup-table
    const VkDescriptorType DescriptorTypes[] =
    {
        // ConstantBuffers
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        // SRV Images
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        // UAV Textures
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        // SRV + UAV Buffers
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        // Samplers
        VK_DESCRIPTOR_TYPE_SAMPLER,
    };
    static_assert(ARRAY_COUNT(DescriptorTypes) == BindingType_Count, "The DescriptorTypes array is out of date");
    
    // Create Descriptor Bindings
    TArray<VkDescriptorSetLayout>        Layouts;
    TArray<VkDescriptorSetLayoutBinding> LayoutBindings;

    for (int32 SetIndex = 0; SetIndex < CreateInfo.StageSetLayouts.Size(); SetIndex++)
    {
        // Clear layout bindings for each DescriptorSet
        LayoutBindings.Clear();
        
        const FDescriptorSetLayout& SetLayoutInfo = CreateInfo.StageSetLayouts[SetIndex];
        CHECK(SetLayoutInfo.ShaderVisibility < ShaderVisibility_Count);

        // Setup all the bindings
        for (const FVulkanShaderBinding& Binding : SetLayoutInfo.Bindings)
        {
            VkDescriptorSetLayoutBinding LayoutBinding;
            LayoutBinding.descriptorCount    = 1;
            LayoutBinding.binding            = Binding.Binding;
            LayoutBinding.pImmutableSamplers = nullptr;
            LayoutBinding.stageFlags         = ShaderVisibility[SetLayoutInfo.ShaderVisibility];
            LayoutBinding.descriptorType     = DescriptorTypes[Binding.BindingType];
            CHECK(Binding.BindingType < BindingType_Count);
            
            LayoutBindings.Add(LayoutBinding);
        }
        
        // Create the DescriptorSetLayout or assign an "default" empty DescriptorSetLayout
        VkDescriptorSetLayout NewSetLayout = VK_NULL_HANDLE;
        if (!LayoutBindings.IsEmpty())
        {
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
                DescriptorSetLayoutHandles[SetLayoutInfo.ShaderVisibility] = Layouts.Add(NewSetLayout);
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
    ConstantRange.size       = CreateInfo.NumGlobalConstants * sizeof(uint32);
    ConstantRange.offset     = 0;
    ConstantRange.stageFlags = VK_SHADER_STAGE_ALL;

    // Create PipelineLayout
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
    FMemory::Memzero(&PipelineLayoutCreateInfo);

    PipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutCreateInfo.setLayoutCount = Layouts.Size();
    PipelineLayoutCreateInfo.pSetLayouts    = Layouts.Data();
    
    if (CreateInfo.NumGlobalConstants > 0)
    {
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
        // Store the number of push constants for use later when binding pushconstants
        NumPushConstants = CreateInfo.NumGlobalConstants;
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
