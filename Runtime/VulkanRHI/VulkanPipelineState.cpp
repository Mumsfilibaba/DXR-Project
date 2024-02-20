#include "VulkanPipelineState.h"
#include "VulkanDevice.h"
#include "VulkanShader.h"

FVulkanVertexInputLayout::FVulkanVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer)
    : FRHIVertexInputLayout()
{
    const int32 NumAttributes = Initializer.Elements.Size();
    VertexInputAttributeDescriptions.Reserve(NumAttributes);

    int32 Location         = 0;
    int32 CurrentBinding   = -1;
    int32 CurrentInputSlot = -1;
    for (const FVertexInputElement& Element : Initializer.Elements)
    {
        // Create a new binding for each InputSlot we have
        if (CurrentInputSlot != static_cast<int32>(Element.InputSlot))
        {
            VkVertexInputBindingDescription BindingDescription;
            BindingDescription.binding   = Element.InputSlot;
            BindingDescription.stride    = Element.VertexStride;
            BindingDescription.inputRate = Element.InputClass == EVertexInputClass::Vertex ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
            
            // Get the current binding for the attributes
            CurrentBinding   = VertexInputBindingDescriptions.Size();
            CurrentInputSlot = Element.InputSlot;
            VertexInputBindingDescriptions.Add(BindingDescription);

            // Reset the location
            Location = 0;
        }

        // Fill in the attribute
        VkVertexInputAttributeDescription VertexInputAttributeDescription;
        VertexInputAttributeDescription.format   = ConvertFormat(Element.Format);
        VertexInputAttributeDescription.offset   = Element.ByteOffset;
        VertexInputAttributeDescription.binding  = CurrentBinding;
        VertexInputAttributeDescription.location = Location++; // This turns into the variable index in the structure
        VertexInputAttributeDescriptions.Add(VertexInputAttributeDescription);
    }

    VertexInputBindingDescriptions.Shrink();
}


FVulkanDepthStencilState::FVulkanDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
    : FRHIDepthStencilState()
    , Initializer(InInitializer)
{
    FMemory::Memzero(&CreateInfo);
    
    CreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    CreateInfo.depthTestEnable       = InInitializer.bDepthEnable;
    CreateInfo.depthWriteEnable      = InInitializer.bDepthWriteEnable;
    CreateInfo.depthCompareOp        = ConvertComparisonFunc(InInitializer.DepthFunc);
    CreateInfo.depthBoundsTestEnable = VK_FALSE;
    CreateInfo.stencilTestEnable     = InInitializer.bStencilEnable;
    CreateInfo.front                 = ConvertStencilState(InInitializer.FrontFace);
    CreateInfo.back                  = ConvertStencilState(InInitializer.BackFace);
    CreateInfo.minDepthBounds        = 0.0f;
    CreateInfo.maxDepthBounds        = 1.0f;
    
    CreateInfo.front.compareMask = CreateInfo.back.compareMask = InInitializer.StencilReadMask;
    CreateInfo.front.writeMask   = CreateInfo.back.writeMask   = InInitializer.StencilWriteMask;
}


FVulkanRasterizerState::FVulkanRasterizerState(FVulkanDevice* InDevice, const FRHIRasterizerStateInitializer& InInitializer)
    : FRHIRasterizerState()
    , FVulkanDeviceChild(InDevice)
    , Initializer(InInitializer)
{
    FMemory::Memzero(&CreateInfo);
    
    // Helper for checking for extensions
    FVulkanStructureHelper CreateInfoHelper(CreateInfo);
    
    CreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    CreateInfo.rasterizerDiscardEnable = VK_FALSE;
    CreateInfo.polygonMode             = ConvertFillMode(InInitializer.FillMode);
    CreateInfo.cullMode                = ConvertCullMode(InInitializer.CullMode);
    CreateInfo.frontFace               = InInitializer.bFrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    CreateInfo.depthBiasEnable         = InInitializer.bEnableDepthBias ? VK_TRUE : VK_FALSE;
    CreateInfo.depthBiasConstantFactor = InInitializer.DepthBias;
    CreateInfo.depthBiasClamp          = InInitializer.DepthBiasClamp;
    CreateInfo.depthBiasSlopeFactor    = InInitializer.SlopeScaledDepthBias;
    CreateInfo.lineWidth               = 1.0f;

    // NOTE: we are forced to disable this since there are not really any equivalent in D3D12
    // The feature described in the spec, is always enabled in D3D12, and the only controllable
    // aspect in D3D12 is DepthClip, which is disabled when 'depthClampEnable' is set to true.
    CreateInfo.depthClampEnable = VK_FALSE;
    
    // NOTE: This extension is the only way to get parity with D3D12, see the above comment for more information
#if VK_EXT_depth_clip_enable
    FMemory::Memzero(&DepthClipStateCreateInfo);
    DepthClipStateCreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
    DepthClipStateCreateInfo.depthClipEnable = InInitializer.bDepthClipEnable ? VK_TRUE : VK_FALSE;
    
    if (GetDevice()->IsDepthClipSupported())
    {
        // NOTE: Since this feature is always enabled in D3D12, for now, we do the same in Vulkan
        // since the Depth-clipping is now controlled by a separate value as in D3D12
        CreateInfo.depthClampEnable = VK_TRUE;
        CreateInfoHelper.AddNext(DepthClipStateCreateInfo);
    }
#endif
    
#if VK_EXT_conservative_rasterization
    FMemory::Memzero(&ConservativeStateCreateInfo);
    ConservativeStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
    
    if (InInitializer.bEnableConservativeRaster)
    {
        ConservativeStateCreateInfo.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
    }
    else
    {
        ConservativeStateCreateInfo.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT;
    }

    if (GetDevice()->IsConservativeRasterizationSupported())
    {
        const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& ConservativeRasterizationProperties = GetDevice()->GetPhysicalDevice()->GetConservativeRasterizationProperties();
        ConservativeStateCreateInfo.extraPrimitiveOverestimationSize = ConservativeRasterizationProperties.maxExtraPrimitiveOverestimationSize;
        CreateInfoHelper.AddNext(ConservativeStateCreateInfo);
    }
#endif
}


FVulkanBlendState::FVulkanBlendState(const FRHIBlendStateInitializer& InInitializer)
    : FRHIBlendState()
    , Initializer(InInitializer)
{
    FMemory::Memzero(&CreateInfo);

    // NOTE: Blend constants are configured as dynamic state
    CreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    CreateInfo.logicOpEnable   = InInitializer.bLogicOpEnable;
    CreateInfo.logicOp         = ConvertLogicOp(InInitializer.LogicOp);
    CreateInfo.attachmentCount = InInitializer.NumRenderTargets;
    CreateInfo.pAttachments    = BlendAttachmentStates;

    for (int32 Index = 0; Index < InInitializer.NumRenderTargets; Index++)
    {
        BlendAttachmentStates[Index].blendEnable         = InInitializer.RenderTargets[Index].bBlendEnable ? VK_TRUE : VK_FALSE;
        BlendAttachmentStates[Index].srcColorBlendFactor = ConvertBlend(InInitializer.RenderTargets[Index].SrcBlend);
        BlendAttachmentStates[Index].dstColorBlendFactor = ConvertBlend(InInitializer.RenderTargets[Index].DstBlend);
        BlendAttachmentStates[Index].colorBlendOp        = ConvertBlendOp(InInitializer.RenderTargets[Index].BlendOp);
        BlendAttachmentStates[Index].srcAlphaBlendFactor = ConvertBlend(InInitializer.RenderTargets[Index].SrcBlendAlpha);
        BlendAttachmentStates[Index].dstAlphaBlendFactor = ConvertBlend(InInitializer.RenderTargets[Index].DstBlendAlpha);
        BlendAttachmentStates[Index].alphaBlendOp        = ConvertBlendOp(InInitializer.RenderTargets[Index].BlendOpAlpha);
        BlendAttachmentStates[Index].colorWriteMask      = ConvertColorWriteFlags(InInitializer.RenderTargets[Index].ColorWriteMask);
    }
}


FVulkanPipeline::FVulkanPipeline(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Pipeline(VK_NULL_HANDLE)
    , PipelineLayout(VK_NULL_HANDLE)
{
}

FVulkanPipeline::~FVulkanPipeline()
{
    if (VULKAN_CHECK_HANDLE(Pipeline))
    {
        vkDestroyPipeline(GetDevice()->GetVkDevice(), Pipeline, nullptr);
        Pipeline = VK_NULL_HANDLE;
    }
}

void FVulkanPipeline::SetDebugName(const FString& InName)
{
    FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.GetCString(), Pipeline, VK_OBJECT_TYPE_PIPELINE);
}


FVulkanGraphicsPipelineState::FVulkanGraphicsPipelineState(FVulkanDevice* InDevice)
    : FRHIGraphicsPipelineState()
    , FVulkanPipeline(InDevice)
    , VertexShader(nullptr)
    , HullShader(nullptr)
    , DomainShader(nullptr)
    , GeometryShader(nullptr)
    , PixelShader(nullptr)
{
}

bool FVulkanGraphicsPipelineState::Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    // Shader-Stages
    TArray<EShaderVisibility>               ShaderVisibility;
    TArray<VkPipelineShaderStageCreateInfo> ShaderStages;
    TArray<const FVulkanShaderLayout*>      ShaderLayouts;

    {
        VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
        FMemory::Memzero(&ShaderStageCreateInfo);

        ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStageCreateInfo.pName = "main";

        if (FVulkanVertexShader* VulkanVertexShader = static_cast<FVulkanVertexShader*>(Initializer.ShaderState.VertexShader))
        {
            ShaderStageCreateInfo.module = VulkanVertexShader->GetVkShaderModule();
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
            ShaderStages.Add(ShaderStageCreateInfo);
            ShaderVisibility.Add(ShaderVisibility_Vertex);
            ShaderLayouts.Add(VulkanVertexShader->GetShaderLayout());
            VertexShader = MakeSharedRef<FVulkanVertexShader>(VulkanVertexShader);
        }
        else
        {
            VULKAN_ERROR("VertexShader cannot be nullptr");
            return false;
        }

        if (FVulkanHullShader* VulkanHullShader = static_cast<FVulkanHullShader*>(Initializer.ShaderState.HullShader))
        {
            ShaderStageCreateInfo.module = VulkanHullShader->GetVkShaderModule();
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            ShaderStages.Add(ShaderStageCreateInfo);
            ShaderVisibility.Add(ShaderVisibility_Hull);
            ShaderLayouts.Add(VulkanHullShader->GetShaderLayout());
            HullShader = MakeSharedRef<FVulkanHullShader>(VulkanHullShader);
        }
        else
        {
            ShaderLayouts.Add(nullptr);
        }

        if (FVulkanDomainShader* VulkanDomainShader = static_cast<FVulkanDomainShader*>(Initializer.ShaderState.DomainShader))
        {
            ShaderStageCreateInfo.module = VulkanDomainShader->GetVkShaderModule();
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            ShaderStages.Add(ShaderStageCreateInfo);
            ShaderVisibility.Add(ShaderVisibility_Domain);
            ShaderLayouts.Add(VulkanDomainShader->GetShaderLayout());
            DomainShader = MakeSharedRef<FVulkanDomainShader>(VulkanDomainShader);
        }
        else
        {
            ShaderLayouts.Add(nullptr);
        }
        
        if (FVulkanGeometryShader* VulkanGeometryShader = static_cast<FVulkanGeometryShader*>(Initializer.ShaderState.GeometryShader))
        {
            ShaderStageCreateInfo.module = VulkanGeometryShader->GetVkShaderModule();
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_GEOMETRY_BIT;
            ShaderStages.Add(ShaderStageCreateInfo);
            ShaderVisibility.Add(ShaderVisibility_Geometry);
            ShaderLayouts.Add(VulkanGeometryShader->GetShaderLayout());
            GeometryShader = MakeSharedRef<FVulkanGeometryShader>(VulkanGeometryShader);
        }
        else
        {
            ShaderLayouts.Add(nullptr);
        }
        
        if (FVulkanPixelShader* VulkanPixelShader = static_cast<FVulkanPixelShader*>(Initializer.ShaderState.PixelShader))
        {
            ShaderStageCreateInfo.module = VulkanPixelShader->GetVkShaderModule();
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            ShaderStages.Add(ShaderStageCreateInfo);
            ShaderVisibility.Add(ShaderVisibility_Pixel);
            ShaderLayouts.Add(VulkanPixelShader->GetShaderLayout());
            PixelShader = MakeSharedRef<FVulkanPixelShader>(VulkanPixelShader);
        }
        else
        {
            ShaderLayouts.Add(nullptr);
        }
    }

    // VertexInputStateCreateInfo
    VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo;
    FMemory::Memzero(&VertexInputStateCreateInfo);

    {
        VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        if (FVulkanVertexInputLayout* VertexInputLayout = static_cast<FVulkanVertexInputLayout*>(Initializer.VertexInputLayout))
        {
            VertexInputStateCreateInfo.vertexBindingDescriptionCount   = VertexInputLayout->GetNumVertexInputBindingDescriptions();
            VertexInputStateCreateInfo.pVertexBindingDescriptions      = VertexInputLayout->GetVertexInputBindingDescriptions();
            VertexInputStateCreateInfo.vertexAttributeDescriptionCount = VertexInputLayout->GetNumVertexInputAttributeDescriptions();
            VertexInputStateCreateInfo.pVertexAttributeDescriptions    = VertexInputLayout->GetVertexInputAttributeDescriptions();
        }
    }

    // InputAssembly CreateInfo
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo;
    FMemory::Memzero(&InputAssemblyCreateInfo);

    InputAssemblyCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCreateInfo.topology               = ConvertPrimitiveTopology(Initializer.PrimitiveTopology);
    InputAssemblyCreateInfo.primitiveRestartEnable = Initializer.bPrimitiveRestartEnable ? VK_TRUE : VK_FALSE;

    // Viewport CreateInfo
    VkPipelineViewportStateCreateInfo ViewportStateCreateInfo;
    FMemory::Memzero(&ViewportStateCreateInfo);

    // NOTE: We use dynamic state for Viewports and Scissors
    ViewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportStateCreateInfo.viewportCount = 1;
    ViewportStateCreateInfo.scissorCount  = 1;

    // RasterizerState CreateInfo
    VkPipelineRasterizationStateCreateInfo RasterizerStateCreateInfo;
    FMemory::Memzero(&RasterizerStateCreateInfo);

    if (FVulkanRasterizerState* RasterizerState = static_cast<FVulkanRasterizerState*>(Initializer.RasterizerState))
    {
        RasterizerStateCreateInfo = RasterizerState->GetVkCreateInfo();
    }
    else
    {
        VULKAN_ERROR("RasterizerState cannot be nullptr");
        return false;
    }

    // MultiSampling CreateInfo
    VkPipelineMultisampleStateCreateInfo MultisamplingCreateInfo;
    FMemory::Memzero(&MultisamplingCreateInfo);

    MultisamplingCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisamplingCreateInfo.sampleShadingEnable   = VK_FALSE;
    MultisamplingCreateInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    MultisamplingCreateInfo.minSampleShading      = 1.0f;
    MultisamplingCreateInfo.pSampleMask           = nullptr;
    MultisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
    MultisamplingCreateInfo.alphaToOneEnable      = VK_FALSE;

    // DepthStencilState CreateInfo
    VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo;
    FMemory::Memzero(&DepthStencilStateCreateInfo);

    if (FVulkanDepthStencilState* DepthStencilState = static_cast<FVulkanDepthStencilState*>(Initializer.DepthStencilState))
    {
        DepthStencilStateCreateInfo = DepthStencilState->GetVkCreateInfo();
    }
    else
    {
        VULKAN_ERROR("DepthStencilState cannot be nullptr");
        return false;
    }

    // BlendState CreateInfo
    VkPipelineColorBlendStateCreateInfo BlendStateCreateInfo;
    FMemory::Memzero(&BlendStateCreateInfo);

    if (FVulkanBlendState* BlendState = static_cast<FVulkanBlendState*>(Initializer.BlendState))
    {
        BlendStateCreateInfo = BlendState->GetVkCreateInfo();
    }
    else
    {
        VULKAN_ERROR("BlendState cannot be nullptr");
        return false;
    }

    // Dynamic-State CreateInfo
    VkDynamicState DynamicStates[] = 
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
    };

    VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo;
    FMemory::Memzero(&DynamicStateCreateInfo);

    DynamicStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateCreateInfo.dynamicStateCount = ARRAY_COUNT(DynamicStates);
    DynamicStateCreateInfo.pDynamicStates    = DynamicStates;

    // Create PipelineLayout
    constexpr uint8 NumGraphicsShaderStages = 5;

    FVulkanPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
    PipelineLayoutCreateInfo.NumGlobalConstants = VULKAN_MAX_NUM_PUSH_CONSTANTS;
    PipelineLayoutCreateInfo.StageSetLayouts.Resize(NumGraphicsShaderStages);
    
    for (EShaderVisibility ShaderStage = ShaderVisibility_Vertex; ShaderStage <= ShaderVisibility_Pixel; ShaderStage = EShaderVisibility(ShaderStage + 1))
    {
        FDescriptorSetLayout& SetLayout = PipelineLayoutCreateInfo.StageSetLayouts[ShaderStage];
        SetLayout.ShaderVisibility = ShaderStage;

        if (const FVulkanShaderLayout* ShaderLayout = ShaderLayouts[ShaderStage])
        {
            SetLayout.Bindings.Append(ShaderLayout->SampledImageBindings);
            SetLayout.Bindings.Append(ShaderLayout->SamplerBindings);
            SetLayout.Bindings.Append(ShaderLayout->SRVStorageBufferBindings);
            SetLayout.Bindings.Append(ShaderLayout->UAVStorageBufferBindings);
            SetLayout.Bindings.Append(ShaderLayout->StorageImageBindings);
            SetLayout.Bindings.Append(ShaderLayout->UniformBufferBindings);
            SetLayout.Bindings.Shrink();
            
            const uint32 NumPushConstants = FMath::Min<uint32>(ShaderLayout->NumPushConstants, VULKAN_MAX_NUM_PUSH_CONSTANTS);
            PipelineLayoutCreateInfo.NumGlobalConstants = FMath::Max<uint32>(PipelineLayoutCreateInfo.NumGlobalConstants, NumPushConstants);
        }
    }

    FVulkanPipelineLayoutManager& PipelineLayoutManager = GetDevice()->GetPipelineLayoutManager();
    PipelineLayout = PipelineLayoutManager.CreateLayout(PipelineLayoutCreateInfo);
    if (!PipelineLayout)
    {
        return false;
    }

    // Retrieve a compatible RenderPass
    // NOTE: The RenderPass only needs to be compatible, and does not actually need to be the same one that actually will be used
    FVulkanRenderPassKey RenderPassKey;
    RenderPassKey.NumSamples                      = Initializer.SampleCount;
    RenderPassKey.DepthStencilFormat              = Initializer.PipelineFormats.DepthStencilFormat;
    RenderPassKey.DepthStencilActions.LoadAction  = EAttachmentLoadAction::Load;
    RenderPassKey.DepthStencilActions.StoreAction = EAttachmentStoreAction::Store;
    RenderPassKey.NumRenderTargets                = Initializer.PipelineFormats.NumRenderTargets;

    for (uint8 Index = 0; Index < Initializer.PipelineFormats.NumRenderTargets; Index++)
    {
        RenderPassKey.RenderTargetActions[Index].LoadAction  = EAttachmentLoadAction::Load;
        RenderPassKey.RenderTargetActions[Index].StoreAction = EAttachmentStoreAction::Store;
        RenderPassKey.RenderTargetFormats[Index] = Initializer.PipelineFormats.RenderTargetFormats[Index];
    }

    VkRenderPass RenderPass = GetDevice()->GetRenderPassCache().GetRenderPass(RenderPassKey);
    if (!VULKAN_CHECK_HANDLE(RenderPass))
    {
        return false;
    }

    // Create PipelineState
    VkGraphicsPipelineCreateInfo PipelineCreateInfo;
    FMemory::Memzero(&PipelineCreateInfo);

    PipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.stageCount          = ShaderStages.Size();
    PipelineCreateInfo.pStages             = ShaderStages.Data();
    PipelineCreateInfo.pVertexInputState   = &VertexInputStateCreateInfo;
    PipelineCreateInfo.pInputAssemblyState = &InputAssemblyCreateInfo;
    PipelineCreateInfo.pViewportState      = &ViewportStateCreateInfo;
    PipelineCreateInfo.pRasterizationState = &RasterizerStateCreateInfo;
    PipelineCreateInfo.pMultisampleState   = &MultisamplingCreateInfo;
    PipelineCreateInfo.pDepthStencilState  = &DepthStencilStateCreateInfo;
    PipelineCreateInfo.pColorBlendState    = &BlendStateCreateInfo;
    PipelineCreateInfo.pDynamicState       = &DynamicStateCreateInfo;
    PipelineCreateInfo.layout              = PipelineLayout->GetVkPipelineLayout();
    PipelineCreateInfo.renderPass          = RenderPass;
    PipelineCreateInfo.subpass             = 0;
    PipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
    PipelineCreateInfo.basePipelineIndex   = -1;

    VkResult Result = vkCreateGraphicsPipelines(GetDevice()->GetVkDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &Pipeline);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create GraphicsPipeline");
        return false;
    }
    else
    {
        return true;
    }
}


FVulkanComputePipelineState::FVulkanComputePipelineState(FVulkanDevice* InDevice)
    : FRHIComputePipelineState()
    , FVulkanPipeline(InDevice)
    , ComputeShader(nullptr)
{
}

bool FVulkanComputePipelineState::Initialize(const FRHIComputePipelineStateInitializer& Initializer)
{
    FVulkanComputeShader* VulkanComputeShader = static_cast<FVulkanComputeShader*>(Initializer.Shader);
    if (!VulkanComputeShader)
    {
        VULKAN_ERROR("Compute Shader cannot be nullptr");
        return false;
    }
    else
    {
        ComputeShader = MakeSharedRef<FVulkanComputeShader>(VulkanComputeShader);
    }

    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
    FMemory::Memzero(&ShaderStageCreateInfo);

    ShaderStageCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    ShaderStageCreateInfo.module = ComputeShader->GetVkShaderModule();
    ShaderStageCreateInfo.pName  = "main";

    // PipelineLayout
    FVulkanPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
    PipelineLayoutCreateInfo.StageSetLayouts.Resize(1);
    
    FDescriptorSetLayout& SetLayout = PipelineLayoutCreateInfo.StageSetLayouts[0];
    SetLayout.ShaderVisibility = ShaderVisibility_Compute;
    
    if (const FVulkanShaderLayout* ShaderLayout = ComputeShader->GetShaderLayout())
    {
        SetLayout.Bindings.Append(ShaderLayout->SampledImageBindings);
        SetLayout.Bindings.Append(ShaderLayout->SamplerBindings);
        SetLayout.Bindings.Append(ShaderLayout->SRVStorageBufferBindings);
        SetLayout.Bindings.Append(ShaderLayout->UAVStorageBufferBindings);
        SetLayout.Bindings.Append(ShaderLayout->StorageImageBindings);
        SetLayout.Bindings.Append(ShaderLayout->UniformBufferBindings);
        SetLayout.Bindings.Shrink();
        
        PipelineLayoutCreateInfo.NumGlobalConstants = FMath::Min<uint32>(ShaderLayout->NumPushConstants, VULKAN_MAX_NUM_PUSH_CONSTANTS);
    }

    FVulkanPipelineLayoutManager& PipelineLayoutManager = GetDevice()->GetPipelineLayoutManager();
    PipelineLayout = PipelineLayoutManager.CreateLayout(PipelineLayoutCreateInfo);
    if (!PipelineLayout)
    {
        return false;
    }

    // Create the ComputePipeline
    VkComputePipelineCreateInfo PipelineCreateInfo;
    FMemory::Memzero(&PipelineCreateInfo);

    PipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.layout = PipelineLayout->GetVkPipelineLayout();
    PipelineCreateInfo.stage  = ShaderStageCreateInfo;

    VkResult Result = vkCreateComputePipelines(GetDevice()->GetVkDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &Pipeline);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create ComputePipeline");
        return false;
    }
    else
    {
        return true;
    }
}
