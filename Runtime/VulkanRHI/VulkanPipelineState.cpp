#include "VulkanPipelineState.h"
#include "VulkanDevice.h"
#include "VulkanShader.h"

FVulkanVertexInputLayout::FVulkanVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer)
    : FRHIVertexInputLayout()
    , FVulkanRefCounted()
{
    const int32 NumAttributes = Initializer.Elements.Size();
    VertexInputAttributeDescriptions.Reserve(NumAttributes);
    
    int32 CurrentBinding   = -1;
    int32 CurrentInputSlot = -1;
    
    for (const FVertexInputElement& Element : Initializer.Elements)
    {
        // Create a new binding for each inputslot we have
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
        }
        
        // Fill in the attribute
        VkVertexInputAttributeDescription VertexInputAttributeDescription;
        VertexInputAttributeDescription.format   = ConvertFormat(Element.Format);
        VertexInputAttributeDescription.offset   = Element.ByteOffset;
        VertexInputAttributeDescription.binding  = CurrentBinding;
        VertexInputAttributeDescription.location = 0; // We use structures so always location zero
        VertexInputAttributeDescriptions.Add(VertexInputAttributeDescription);
    }
    
    VertexInputBindingDescriptions.Shrink();
}


FVulkanDepthStencilState::FVulkanDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
    : FRHIDepthStencilState()
    , FVulkanRefCounted()
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
    , FVulkanDeviceObject(InDevice)
    , FVulkanRefCounted()
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
    , FVulkanRefCounted()
    , Initializer(InInitializer)
{
    FMemory::Memzero(&CreateInfo);

    CreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    CreateInfo.logicOpEnable   = InInitializer.bLogicOpEnable;
    CreateInfo.logicOp         = ConvertLogicOp(InInitializer.LogicOp);
    CreateInfo.attachmentCount = InInitializer.NumRenderTargets;
    CreateInfo.pAttachments    = BlendAttachmentStates;
    // NOTE: Blend constants are configured as dynamic state

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
    : FVulkanDeviceObject(InDevice)
    , FVulkanRefCounted()
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

    if (VULKAN_CHECK_HANDLE(PipelineLayout))
    {
        vkDestroyPipelineLayout(GetDevice()->GetVkDevice(), PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
    }
}

void FVulkanPipeline::SetDebugName(const FString& InName)
{
    FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.GetCString(), Pipeline, VK_OBJECT_TYPE_PIPELINE);
}


FVulkanGraphicsPipelineState::FVulkanGraphicsPipelineState(FVulkanDevice* InDevice)
    : FRHIGraphicsPipelineState()
    , FVulkanPipeline(InDevice)
{
}

bool FVulkanGraphicsPipelineState::Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    // Shader-Stages
    TArray<VkPipelineShaderStageCreateInfo> ShaderStages;

    {
        VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
        FMemory::Memzero(&ShaderStageCreateInfo);

        ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStageCreateInfo.pName = "Spirv_Main";

        if (FVulkanVertexShader* VertexShader = static_cast<FVulkanVertexShader*>(Initializer.ShaderState.VertexShader))
        {
            ShaderStageCreateInfo.module = VertexShader->GetVkShaderModule();
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
            ShaderStages.Add(ShaderStageCreateInfo);
        }
        else
        {
            VULKAN_ERROR("VertexShader cannot be nullptr");
            return false;
        }

        if (FVulkanPixelShader* PixelShader = static_cast<FVulkanPixelShader*>(Initializer.ShaderState.PixelShader))
        {
            ShaderStageCreateInfo.module = PixelShader->GetVkShaderModule();
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            ShaderStages.Add(ShaderStageCreateInfo);
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

    // NOTE: We use dynamic state for viewports and scissors
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


    // PipelineLayout
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
    FMemory::Memzero(&PipelineLayoutCreateInfo);

    PipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutCreateInfo.setLayoutCount         = 0;       // Optional
    PipelineLayoutCreateInfo.pSetLayouts            = nullptr; // Optional
    PipelineLayoutCreateInfo.pushConstantRangeCount = 0;       // Optional
    PipelineLayoutCreateInfo.pPushConstantRanges    = nullptr; // Optional

    VkResult Result = vkCreatePipelineLayout(GetDevice()->GetVkDevice(), &PipelineLayoutCreateInfo, nullptr, &PipelineLayout);
    VULKAN_CHECK_RESULT(Result, "Failed to create PipelineLayout");


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
    PipelineCreateInfo.layout              = PipelineLayout;
    PipelineCreateInfo.renderPass          = VK_NULL_HANDLE; // renderPass;
    PipelineCreateInfo.subpass             = 0;
    PipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
    PipelineCreateInfo.basePipelineIndex   = -1;

    Result = vkCreateGraphicsPipelines(GetDevice()->GetVkDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &Pipeline);
    VULKAN_CHECK_RESULT(Result, "Failed to create GraphicsPipeline");
    return true;
}