#include "VulkanPipelineState.h"
#include "VulkanDevice.h"
#include "VulkanShader.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Threading/TaskManager.h"
#include "Core/Misc/ConsoleManager.h"
#include "Project/ProjectManager.h"

static TAutoConsoleVariable<FString> CVarPipelineCacheFileName(
    "VulkanRHI.PipelineCacheFileName",
    "FileName for the file storing the PipelineCache",
    "PipelineCache.vkpsocache");

FVulkanVertexInputLayout::FVulkanVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer)
    : FRHIVertexInputLayout()
{
    const int32 NumAttributes = Initializer.Elements.Size();
    VertexInputAttributeDescriptions.Reserve(NumAttributes);

    // NOTE: The input struct on the ShaderSide, needs to match the CPU side struct for vertices
    // otherwise we need to reflect the location from the vertex-shader and use the vertex-shader as 
    // input, similar to D3D11 style input-layout
    for (const FVertexInputElement& Element : Initializer.Elements)
    {
        // Create a new unique binding for each input slot that we need
        int32 CurrentBinding = -1;
        for (int32 Index = 0; Index < VertexInputBindingDescriptions.Size(); Index++)
        {
            if (VertexInputBindingDescriptions[Index].binding == Element.InputSlot)
            {
                CurrentBinding = Index;
                break;
            }
        }

        if (CurrentBinding < 0)
        {
            VkVertexInputBindingDescription BindingDescription;
            BindingDescription.binding   = Element.InputSlot;
            BindingDescription.stride    = Element.VertexStride;
            BindingDescription.inputRate = Element.InputClass == EVertexInputClass::Vertex ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
            
            // Get the current binding for the attributes
            CurrentBinding = VertexInputBindingDescriptions.Size();
            VertexInputBindingDescriptions.Add(BindingDescription);
        }

        // Find location for the element which turns into the variable index in the structure
        uint32 Location = 0;
        for (int32 Index = 0; Index < VertexInputAttributeDescriptions.Size(); Index++)
        {
            VkVertexInputAttributeDescription& Attribute = VertexInputAttributeDescriptions[Index];
            if (Attribute.binding != static_cast<uint32>(CurrentBinding))
            {
                continue;
            }

            // Put this element after this existing attribute if the offset in the struct is larger than the existing one
            if (Element.ByteOffset > Attribute.offset)
            {
                Location++;
            }

            // if this existing attribute has a larger offset than the new one increase the location of the existing attribute
            if (Attribute.offset > Element.ByteOffset)
            {
                Attribute.location++;
            }
        }

        // Fill in the attribute
        VkVertexInputAttributeDescription VertexInputAttributeDescription;
        VertexInputAttributeDescription.format   = ConvertFormat(Element.Format);
        VertexInputAttributeDescription.offset   = Element.ByteOffset;
        VertexInputAttributeDescription.binding  = CurrentBinding;
        VertexInputAttributeDescription.location = Location;
        VertexInputAttributeDescriptions.Add(VertexInputAttributeDescription);
    }

    VertexInputBindingDescriptions.Shrink();
    
    // VertexInputStateCreateInfo
    FMemory::Memzero(&CreateInfo);
    CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    if (!VertexInputBindingDescriptions.IsEmpty())
    {
        CreateInfo.vertexBindingDescriptionCount = VertexInputBindingDescriptions.Size();
        CreateInfo.pVertexBindingDescriptions    = VertexInputBindingDescriptions.Data();
    }
    
    if (!VertexInputAttributeDescriptions.IsEmpty())
    {
        CreateInfo.vertexAttributeDescriptionCount = VertexInputAttributeDescriptions.Size();
        CreateInfo.pVertexAttributeDescriptions    = VertexInputAttributeDescriptions.Data();
    }
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
    , PipelineLayout(nullptr)
{
}

FVulkanPipeline::~FVulkanPipeline()
{
    if (VULKAN_CHECK_HANDLE(Pipeline))
    {
        vkDestroyPipeline(GetDevice()->GetVkDevice(), Pipeline, nullptr);
        Pipeline = VK_NULL_HANDLE;
    }
    
    // Layout is destroyed by the PipelineLayoutManager
    PipelineLayout = nullptr;
}

void FVulkanPipeline::SetDebugName(const FString& InName)
{
    FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.GetCString(), Pipeline, VK_OBJECT_TYPE_PIPELINE);
    DebugName = InName;
}

FVulkanGraphicsPipelineState::FVulkanGraphicsPipelineState(FVulkanDevice* InDevice)
    : FRHIGraphicsPipelineState()
    , FVulkanPipeline(InDevice)
{
}

bool FVulkanGraphicsPipelineState::Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer)
{
    // Gather Shaders for PipelineLayout
    FVulkanShader* Shaders[ShaderVisibility_Count];
    if (FVulkanVertexShader* VulkanVertexShader = static_cast<FVulkanVertexShader*>(Initializer.ShaderState.VertexShader))
    {
        Shaders[ShaderVisibility_Vertex] = VulkanVertexShader;
    }
    else
    {
        VULKAN_ERROR("VertexShader cannot be nullptr");
        return false;
    }

    Shaders[ShaderVisibility_Hull]     = static_cast<FVulkanHullShader*>(Initializer.ShaderState.HullShader);
    Shaders[ShaderVisibility_Domain]   = static_cast<FVulkanDomainShader*>(Initializer.ShaderState.DomainShader);
    Shaders[ShaderVisibility_Geometry] = static_cast<FVulkanGeometryShader*>(Initializer.ShaderState.GeometryShader);
    Shaders[ShaderVisibility_Pixel]    = static_cast<FVulkanPixelShader*>(Initializer.ShaderState.PixelShader);
    
    FVulkanPipelineLayoutInfo LayoutInfo;
    LayoutInfo.AddSetForStage(VK_SHADER_STAGE_VERTEX_BIT, Shaders[ShaderVisibility_Vertex]->GetShaderInfo());
    LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_VERTEX_BIT, Shaders[ShaderVisibility_Vertex]->GetShaderInfo());

    if (Shaders[ShaderVisibility_Hull])
    {
        LayoutInfo.AddSetForStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, Shaders[ShaderVisibility_Hull]->GetShaderInfo());
        LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, Shaders[ShaderVisibility_Hull]->GetShaderInfo());
    }
    if (Shaders[ShaderVisibility_Domain])
    {
        LayoutInfo.AddSetForStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, Shaders[ShaderVisibility_Domain]->GetShaderInfo());
        LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, Shaders[ShaderVisibility_Domain]->GetShaderInfo());
    }
    if (Shaders[ShaderVisibility_Geometry])
    {
        LayoutInfo.AddSetForStage(VK_SHADER_STAGE_GEOMETRY_BIT, Shaders[ShaderVisibility_Geometry]->GetShaderInfo());
        LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_GEOMETRY_BIT, Shaders[ShaderVisibility_Geometry]->GetShaderInfo());
    }
    if (Shaders[ShaderVisibility_Pixel])
    {
        LayoutInfo.AddSetForStage(VK_SHADER_STAGE_FRAGMENT_BIT, Shaders[ShaderVisibility_Pixel]->GetShaderInfo());
        LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_FRAGMENT_BIT, Shaders[ShaderVisibility_Pixel]->GetShaderInfo());
    }
    
    // Generate Hash here since it is saved and not generated all the time
    LayoutInfo.GenerateHash();
    
    // Create PipelineLayout
    FVulkanPipelineLayoutManager& PipelineLayoutManager = GetDevice()->GetPipelineLayoutManager();
    PipelineLayout = PipelineLayoutManager.FindOrCreateLayout(LayoutInfo);
    if (!PipelineLayout)
    {
        return false;
    }
    
    // Gather ShaderModules
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
    FMemory::Memzero(&ShaderStageCreateInfo);

    ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStageCreateInfo.pName = "main";
    
    TArray<VkPipelineShaderStageCreateInfo> ShaderStages;
    if (TSharedRef<FVulkanShaderModule> ShaderModule = Shaders[ShaderVisibility_Vertex]->GetOrCreateShaderModule(PipelineLayout))
    {
        ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
        ShaderStages.Add(ShaderStageCreateInfo);
    }
    else
    {
        VULKAN_ERROR("Failed to create ShaderModule");
        return false;
    }
    
    if (Shaders[ShaderVisibility_Hull])
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = Shaders[ShaderVisibility_Hull]->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
            ShaderStages.Add(ShaderStageCreateInfo);
        }
        else
        {
            VULKAN_ERROR("Failed to create ShaderModule");
            return false;
        }
    }
    if (Shaders[ShaderVisibility_Domain])
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = Shaders[ShaderVisibility_Domain]->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
            ShaderStages.Add(ShaderStageCreateInfo);
        }
        else
        {
            VULKAN_ERROR("Failed to create ShaderModule");
            return false;
        }
    }
    if (Shaders[ShaderVisibility_Geometry])
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = Shaders[ShaderVisibility_Geometry]->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_GEOMETRY_BIT;
            ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
            ShaderStages.Add(ShaderStageCreateInfo);
        }
        else
        {
            VULKAN_ERROR("Failed to create ShaderModule");
            return false;
        }
    }
    if (Shaders[ShaderVisibility_Pixel])
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = Shaders[ShaderVisibility_Pixel]->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
            ShaderStages.Add(ShaderStageCreateInfo);
        }
        else
        {
            VULKAN_ERROR("Failed to create ShaderModule");
            return false;
        }
    }
    
    // VertexInputStateCreateInfo
    VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo;
    if (FVulkanVertexInputLayout* VertexInputLayout = static_cast<FVulkanVertexInputLayout*>(Initializer.VertexInputLayout))
    {
        VertexInputStateCreateInfo = VertexInputLayout->GetVkCreateInfo();
    }
    else
    {
        FMemory::Memzero(&VertexInputStateCreateInfo, sizeof(VkPipelineVertexInputStateCreateInfo));
        VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
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

    FVulkanPipelineStateManager& PipelineCache = GetDevice()->GetPipelineStateManager();
    if (PipelineCache.CreateGraphicsPipeline(PipelineCreateInfo, Pipeline))
    {
        return true;
    }
    else
    {
        VULKAN_WARNING("GraphicsPipeline was not found in PipelineCache");
    }
    
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

    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
    FMemory::Memzero(&ShaderStageCreateInfo);

    ShaderStageCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    ShaderStageCreateInfo.pName  = "main";
    
    // PipelineLayout
    FVulkanPipelineLayoutInfo LayoutInfo;
    LayoutInfo.AddSetForStage(VK_SHADER_STAGE_COMPUTE_BIT, VulkanComputeShader->GetShaderInfo());
    LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_COMPUTE_BIT, VulkanComputeShader->GetShaderInfo());
    LayoutInfo.GenerateHash();

    FVulkanPipelineLayoutManager& PipelineLayoutManager = GetDevice()->GetPipelineLayoutManager();
    PipelineLayout = PipelineLayoutManager.FindOrCreateLayout(LayoutInfo);
    if (!PipelineLayout)
    {
        return false;
    }

    if (TSharedRef<FVulkanShaderModule> ShaderModule = VulkanComputeShader->GetOrCreateShaderModule(PipelineLayout))
    {
        ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
    }
    else
    {
        VULKAN_ERROR("Failed to create ShaderModule");
        return false;
    }

    // Create the ComputePipeline
    VkComputePipelineCreateInfo PipelineCreateInfo;
    FMemory::Memzero(&PipelineCreateInfo);

    PipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.layout = PipelineLayout->GetVkPipelineLayout();
    PipelineCreateInfo.stage  = ShaderStageCreateInfo;

    FVulkanPipelineStateManager& PipelineCache = GetDevice()->GetPipelineStateManager();
    if (PipelineCache.CreateComputePipeline(PipelineCreateInfo, Pipeline))
    {
        return true;
    }
    else
    {
        VULKAN_WARNING("GraphicsPipeline was not found in PipelineCache");
    }
    
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

FVulkanPipelineStateManager::FVulkanPipelineStateManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , PipelineCache(VK_NULL_HANDLE)
    , bPipelineCacheDirty(false)
{
}

FVulkanPipelineStateManager::~FVulkanPipelineStateManager()
{
    Release();
}

bool FVulkanPipelineStateManager::Initialize()
{
    if (LoadCacheFromFile())
    {
        return true;
    }
    
    VkPipelineCacheCreateInfo CreateInfo;
    FMemory::Memzero(&CreateInfo);
    
    CreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    CreateInfo.pInitialData    = nullptr;
    CreateInfo.initialDataSize = 0;

    if (GetDevice()->IsPipelineCacheControlSupported())
    {
        CreateInfo.flags = VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT_EXT;
    }
    
    VkResult Result = vkCreatePipelineCache(GetDevice()->GetVkDevice(), &CreateInfo, nullptr, &PipelineCache);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Vulkan PipelineCache");
        return false;
    }
    else
    {
        return true;
    }
}

void FVulkanPipelineStateManager::Release()
{
    if (VULKAN_CHECK_HANDLE(PipelineCache))
    {
        vkDestroyPipelineCache(GetDevice()->GetVkDevice(), PipelineCache, nullptr);
        PipelineCache = VK_NULL_HANDLE;
    }
}

bool FVulkanPipelineStateManager::CreateGraphicsPipeline(const VkGraphicsPipelineCreateInfo& CreateInfo, VkPipeline& OutPipeline)
{
    TScopedLock Lock(PipelineCacheCS);
    
    VkResult Result = vkCreateGraphicsPipelines(GetDevice()->GetVkDevice(), PipelineCache, 1, &CreateInfo, nullptr, &OutPipeline);
    if (VULKAN_FAILED(Result))
    {
        return false;
    }
    else
    {
        bPipelineCacheDirty = true;
        return true;
    }
}

bool FVulkanPipelineStateManager::CreateComputePipeline(const VkComputePipelineCreateInfo& CreateInfo, VkPipeline& OutPipeline)
{
    TScopedLock Lock(PipelineCacheCS);
    
    VkResult Result = vkCreateComputePipelines(GetDevice()->GetVkDevice(), PipelineCache, 1, &CreateInfo, nullptr, &OutPipeline);
    if (VULKAN_FAILED(Result))
    {
        return false;
    }
    else
    {
        bPipelineCacheDirty = true;
        return true;
    }
}

bool FVulkanPipelineStateManager::SaveCacheData()
{
    if (!VULKAN_CHECK_HANDLE(PipelineCache))
    {
        return false;
    }
    
    // No changes has been made to the pipeline
    if (!bPipelineCacheDirty)
    {
        return true;
    }

    const FString PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const FString PipelineCacheFilepath = FString(FProjectManager::Get().GetAssetPath()) + '/' + PipelineCacheFilename;

    FFileHandleRef CacheFile = FPlatformFile::OpenForWrite(PipelineCacheFilepath);
    if (!CacheFile)
    {
        VULKAN_WARNING("Failed to open PipelineCache-file");
        return false;
    }
    
    {
        TScopedLock Lock(PipelineCacheCS);
        
        size_t PipelineCacheSize = 0;
        VkResult Result = vkGetPipelineCacheData(GetDevice()->GetVkDevice(), PipelineCache, &PipelineCacheSize, nullptr);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("Failed to retrieve size of PipelineCache");
            return false;
        }
        
        TUniquePtr<uint8[]> PipelineCacheData = MakeUnique<uint8[]>(PipelineCacheSize);
        Result = vkGetPipelineCacheData(GetDevice()->GetVkDevice(), PipelineCache, &PipelineCacheSize, PipelineCacheData.Get());
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("Failed to serielize PipelineCache");
            return false;
        }

        FVulkanPipelineDataHeader DataHeader;
        FMemory::Memzero(&DataHeader, sizeof(FVulkanPipelineDataHeader));

        FMemory::Memcpy(DataHeader.Magic, "VKPSO", sizeof(DataHeader.Magic));
        DataHeader.DataCRC  = FCRC32::Generate(PipelineCacheData.Get(), PipelineCacheSize);
        DataHeader.DataSize = PipelineCacheSize;

        int32 BytesWritten = CacheFile->Write(reinterpret_cast<const uint8*>(&DataHeader), sizeof(FVulkanPipelineDataHeader));
        if (BytesWritten != sizeof(FVulkanPipelineDataHeader))
        {
            VULKAN_ERROR("Failed to write PipelineDataHeader to disk");
            return false;
        }

        BytesWritten = CacheFile->Write(PipelineCacheData.Get(), static_cast<uint32>(PipelineCacheSize));
        if (BytesWritten != static_cast<int32>(PipelineCacheSize))
        {
            VULKAN_ERROR("Failed to write PipelineCache to disk");
            return false;
        }
        else
        {
            VULKAN_INFO("Saved PipelineCache to file '%s'", PipelineCacheFilepath.GetCString());
        }
    }
    
    bPipelineCacheDirty = false;
    return true;
}

bool FVulkanPipelineStateManager::LoadCacheFromFile()
{
    const FString PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const FString PipelineCacheFilepath = FString(FProjectManager::Get().GetAssetPath()) + '/' + PipelineCacheFilename;
    
    FFileHandleRef CacheFile = FPlatformFile::OpenForRead(PipelineCacheFilepath);
    if (!CacheFile)
    {
        VULKAN_WARNING("Failed to open PipelineCache-file");
        return false;
    }

    FVulkanPipelineDataHeader DataHeader;
    int64 BytesRead = CacheFile->Read(reinterpret_cast<uint8*>(&DataHeader), sizeof(FVulkanPipelineDataHeader));
    if (BytesRead != sizeof(FVulkanPipelineDataHeader))
    {
        VULKAN_WARNING("Something went wrong when reading PipelineCacheHeader");
        return false;
    }

    // Validate that the file is valid
    if (FMemory::Memcmp(DataHeader.Magic, "VKPSO", sizeof(DataHeader.Magic)) != 0)
    {
        VULKAN_WARNING("Invalid PipelineCacheHeader");
        return false;
    }

    // NOTE: if the cache size is more than 1GB something is probably off
    constexpr uint64 MaxCacheSize = 1024 * 1024 * 1024;
    if (DataHeader.DataSize >= MaxCacheSize)
    {
        VULKAN_WARNING("Invalid PipelineCacheHeader");
        return false;
    }

    // Load the data
    TUniquePtr<uint8[]> PipelineCacheData = MakeUnique<uint8[]>(DataHeader.DataSize);
    BytesRead = CacheFile->Read(PipelineCacheData.Get(), static_cast<uint32>(DataHeader.DataSize));
    if (BytesRead != DataHeader.DataSize)
    {
        VULKAN_WARNING("Something went wrong when reading PipelineCache");
        return false;
    }
    
    if (static_cast<uint64>(DataHeader.DataSize) < sizeof(FVulkanPipelineCacheHeader))
    {
        VULKAN_WARNING("PipelineCache is smaller than PipelineCacheHeader");
        return false;
    }

    // Validate the CRC
    const uint32 DataCRC = FCRC32::Generate(PipelineCacheData.Get(), DataHeader.DataSize);
    if (DataCRC != DataHeader.DataCRC)
    {
        VULKAN_WARNING("PipelineCacheData is invalid");
        return false;
    }
    
    const VkPhysicalDeviceProperties& DeviceProperties = GetDevice()->GetPhysicalDevice()->GetProperties();
    FVulkanPipelineCacheHeader* Header = reinterpret_cast<FVulkanPipelineCacheHeader*>(PipelineCacheData.Get());
    if (Header->VendorID != DeviceProperties.vendorID)
    {
        VULKAN_WARNING("PipelineCacheHeader contains invalid VendorID");
        return false;
    }
    
    if (Header->DeviceID != DeviceProperties.deviceID)
    {
        VULKAN_WARNING("PipelineCacheHeader contains invalid DeviceID");
        return false;
    }
    
    constexpr uint64 UUIDSize = sizeof(DeviceProperties.pipelineCacheUUID);
    if (FMemory::Memcpy(Header->UUID, DeviceProperties.pipelineCacheUUID, UUIDSize) == 0)
    {
        VULKAN_WARNING("PipelineCacheHeader contains invalid UUID");
        return false;
    }
    
    VkPipelineCacheCreateInfo CreateInfo;
    FMemory::Memzero(&CreateInfo);
    
    CreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    CreateInfo.pInitialData    = PipelineCacheData.Get();
    CreateInfo.initialDataSize = DataHeader.DataSize;

    if (GetDevice()->IsPipelineCacheControlSupported())
    {
        CreateInfo.flags = VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT_EXT;
    }
    
    VkResult Result = vkCreatePipelineCache(GetDevice()->GetVkDevice(), &CreateInfo, nullptr, &PipelineCache);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Vulkan PipelineCache");
        return false;
    }
    else
    {
        return true;
    }
}
