#include "Core/Platform/PlatformFile.h"
#include "Core/Threading/TaskManager.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Misc/Paths.h"
#include "VulkanRHI/VulkanPipelineState.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanShader.h"

static TAutoConsoleVariable<FString> CVarPipelineCacheFileName(
    "VulkanRHI.PipelineCacheFileName",
    "FileName for the file storing the PipelineCache",
    "PipelineCache.vkpsocache");

FVulkanInputLayout::FVulkanInputLayout(const TArray<FRHIInputElementInfo>& InInputElements)
    : FRHIInputLayout()
	, InputElements(InInputElements)
    , VertexInputBindingDescriptions()
    , VertexInputAttributeDescriptions()
    , CreateInfo{}
{
    // Create a binding for each input-slot
    for (const FRHIInputElementInfo& Element : InInputElements)
    {
        // Search for a binding for this input slot
        bool bCreateBinding = true;
        for (int32 Index = 0; Index < VertexInputBindingDescriptions.Size(); Index++)
        {
            if (VertexInputBindingDescriptions[Index].binding == Element.InputSlot)
            {
                bCreateBinding = false;
                break;
            }
        }
        
        // Check if binding does not exist, if not create a new binding
        if (bCreateBinding)
        {
            VkVertexInputBindingDescription BindingDescription;
            BindingDescription.binding   = Element.InputSlot;
            BindingDescription.stride    = Element.VertexStride;
            BindingDescription.inputRate = Element.InputClass == EVertexInputClass::Vertex ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
            
            // Get the current binding for the attributes
            VertexInputBindingDescriptions.Add(BindingDescription);
        }
    }
    
    VertexInputBindingDescriptions.Shrink();

    const int32 NumElements = InInputElements.Size();
    VertexInputAttributeDescriptions.Resize(NumElements);

    // Create a input-attribute for each element
    for (int32 Index = 0; Index < NumElements; Index++)
    {
        VkVertexInputAttributeDescription& Attribute = VertexInputAttributeDescriptions[Index];
        Attribute.format   = ConvertFormat(InInputElements[Index].Format);
        Attribute.binding  = InInputElements[Index].InputSlot;
        Attribute.location = InInputElements[Index].ShaderElementIndex;
        Attribute.offset   = InInputElements[Index].ByteOffset;
    }

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

FVulkanInputLayout::~FVulkanInputLayout()
{
}

FVulkanDepthStencilState::FVulkanDepthStencilState(const FRHIDepthStencilStateInfo& InInfo)
    : FRHIDepthStencilState()
    , Info(InInfo)
{
    FMemory::Memzero(&CreateInfo);
    
    CreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    CreateInfo.depthTestEnable       = InInfo.bDepthEnable;
    CreateInfo.depthWriteEnable      = InInfo.bDepthWriteEnable;
    CreateInfo.depthCompareOp        = ConvertComparisonFunc(InInfo.DepthFunc);
    CreateInfo.depthBoundsTestEnable = VK_FALSE;
    CreateInfo.stencilTestEnable     = InInfo.bStencilEnable;
    CreateInfo.front                 = ConvertStencilState(InInfo.FrontFace);
    CreateInfo.back                  = ConvertStencilState(InInfo.BackFace);
    CreateInfo.minDepthBounds        = 0.0f;
    CreateInfo.maxDepthBounds        = 1.0f;
    
    CreateInfo.front.compareMask = CreateInfo.back.compareMask = InInfo.StencilReadMask;
    CreateInfo.front.writeMask   = CreateInfo.back.writeMask   = InInfo.StencilWriteMask;
}

FVulkanDepthStencilState::~FVulkanDepthStencilState()
{
}

FVulkanRasterizerState::FVulkanRasterizerState(FVulkanDevice* InDevice, const FRHIRasterizerStateInfo& InInfo)
    : FRHIRasterizerState()
    , FVulkanDeviceChild(InDevice)
    , Info(InInfo)
{
    FMemory::Memzero(&CreateInfo);
    
    CreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    CreateInfo.rasterizerDiscardEnable = VK_FALSE;
    CreateInfo.polygonMode             = ConvertFillMode(InInfo.FillMode);
    CreateInfo.cullMode                = ConvertCullMode(InInfo.CullMode);
    CreateInfo.frontFace               = InInfo.bFrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    CreateInfo.depthBiasEnable         = InInfo.bEnableDepthBias ? VK_TRUE : VK_FALSE;
    CreateInfo.depthBiasConstantFactor = InInfo.DepthBias;
    CreateInfo.depthBiasClamp          = InInfo.DepthBiasClamp;
    CreateInfo.depthBiasSlopeFactor    = InInfo.SlopeScaledDepthBias;
    CreateInfo.lineWidth               = 1.0f;

    // NOTE: we are forced to disable this since there are not really any equivalent in D3D12
    // The feature described in the spec, is always enabled in D3D12, and the only controllable
    // aspect in D3D12 is DepthClip, which is disabled when 'depthClampEnable' is set to true.
    CreateInfo.depthClampEnable = VK_FALSE;
    
    // NOTE: This extension is the only way to get parity with D3D12, see the above comment for more information
#if VK_EXT_depth_clip_enable
    FMemory::Memzero(&DepthClipStateCreateInfo);

    DepthClipStateCreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
    DepthClipStateCreateInfo.depthClipEnable = InInfo.bDepthClipEnable ? VK_TRUE : VK_FALSE;
    
    if (GVulkanSupportsDepthClip)
    {
        // NOTE: Since this feature is always enabled in D3D12, for now, we do the same in Vulkan
        // since the Depth-clipping is now controlled by a separate value as in D3D12
        CreateInfo.depthClampEnable = VK_TRUE;
    }
#endif
    
#if VK_EXT_conservative_rasterization
    if (GVulkanSupportsConservativeRasterization)
    {
        FMemory::Memzero(&ConservativeStateCreateInfo);
        ConservativeStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
        
        if (InInfo.bEnableConservativeRaster)
        {
            ConservativeStateCreateInfo.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
        }
        else
        {
            ConservativeStateCreateInfo.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT;
        }

        const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& ConservativeRasterizationProperties = GetDevice()->GetPhysicalDevice()->GetConservativeRasterizationProperties();
        ConservativeStateCreateInfo.extraPrimitiveOverestimationSize = ConservativeRasterizationProperties.maxExtraPrimitiveOverestimationSize;
    }
#endif

    // Helper for checking for extensions
    FVulkanStructChain CreateInfoChain(CreateInfo);
#if VK_EXT_depth_clip_enable
    CreateInfoChain.AddNext(DepthClipStateCreateInfo);
#endif
#if VK_EXT_conservative_rasterization
    CreateInfoChain.AddNext(ConservativeStateCreateInfo);
#endif
}

FVulkanRasterizerState::~FVulkanRasterizerState()
{
}

FVulkanBlendState::FVulkanBlendState(const FRHIBlendStateInfo& InInfo)
    : FRHIBlendState()
    , Info(InInfo)
{
    FMemory::Memzero(&CreateInfo);

    // NOTE: Blend constants are configured as dynamic state
    CreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    CreateInfo.logicOpEnable   = InInfo.bLogicOpEnable;
    CreateInfo.logicOp         = ConvertLogicOp(InInfo.LogicOp);
    CreateInfo.attachmentCount = InInfo.NumRenderTargets;
    CreateInfo.pAttachments    = BlendAttachmentStates;

    for (int32 Index = 0; Index < InInfo.NumRenderTargets; Index++)
    {
        BlendAttachmentStates[Index].blendEnable         = InInfo.RenderTargets[Index].bBlendEnable ? VK_TRUE : VK_FALSE;
        BlendAttachmentStates[Index].srcColorBlendFactor = ConvertBlend(InInfo.RenderTargets[Index].SrcBlend);
        BlendAttachmentStates[Index].dstColorBlendFactor = ConvertBlend(InInfo.RenderTargets[Index].DstBlend);
        BlendAttachmentStates[Index].colorBlendOp        = ConvertBlendOp(InInfo.RenderTargets[Index].BlendOp);
        BlendAttachmentStates[Index].srcAlphaBlendFactor = ConvertBlend(InInfo.RenderTargets[Index].SrcBlendAlpha);
        BlendAttachmentStates[Index].dstAlphaBlendFactor = ConvertBlend(InInfo.RenderTargets[Index].DstBlendAlpha);
        BlendAttachmentStates[Index].alphaBlendOp        = ConvertBlendOp(InInfo.RenderTargets[Index].BlendOpAlpha);
        BlendAttachmentStates[Index].colorWriteMask      = ConvertColorWriteFlags(InInfo.RenderTargets[Index].ColorWriteMask);
    }
}

FVulkanBlendState::~FVulkanBlendState()
{
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
    VulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), *InName, Pipeline, VK_OBJECT_TYPE_PIPELINE);
    DebugName = InName;
}

FVulkanGraphicsPipelineState::FVulkanGraphicsPipelineState(FVulkanDevice* InDevice)
    : FRHIGraphicsPipelineState()
    , FVulkanPipeline(InDevice)
{
}

FVulkanGraphicsPipelineState::~FVulkanGraphicsPipelineState()
{
}

bool FVulkanGraphicsPipelineState::Initialize(const FRHIGraphicsPipelineStateInfo& Info)
{
    // Gather Shaders for PipelineLayout
    FVulkanShader* Shaders[ShaderVisibility_Count];
    if (FVulkanVertexShader* VulkanVertexShader = static_cast<FVulkanVertexShader*>(Info.VertexShader))
    {
        Shaders[ShaderVisibility_Vertex] = VulkanVertexShader;
    }
    else
    {
        VULKAN_ERROR_CRITICAL("VertexShader cannot be nullptr");
        return false;
    }

    Shaders[ShaderVisibility_Hull]     = static_cast<FVulkanHullShader*>(Info.HullShader);
    Shaders[ShaderVisibility_Domain]   = static_cast<FVulkanDomainShader*>(Info.DomainShader);
    Shaders[ShaderVisibility_Geometry] = static_cast<FVulkanGeometryShader*>(Info.GeometryShader);
    Shaders[ShaderVisibility_Pixel]    = static_cast<FVulkanPixelShader*>(Info.PixelShader);
    
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
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo = {};
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
        VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
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
            VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
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
            VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
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
            VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
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
            VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
            return false;
        }
    }
    
    // VertexInputStateCreateInfo
    VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo;
    if (FVulkanInputLayout* InputLayout = static_cast<FVulkanInputLayout*>(Info.InputLayout))
    {
        VertexInputStateCreateInfo = InputLayout->GetVkCreateInfo();
    }
    else
    {
        FMemory::Memzero(&VertexInputStateCreateInfo, sizeof(VkPipelineVertexInputStateCreateInfo));
        VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    }

    // InputAssembly CreateInfo
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo = {};
    InputAssemblyCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCreateInfo.topology               = ConvertPrimitiveTopology(Info.PrimitiveTopology);
    InputAssemblyCreateInfo.primitiveRestartEnable = Info.bPrimitiveRestartEnable ? VK_TRUE : VK_FALSE;

    // Viewport CreateInfo
    VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = {};
    ViewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportStateCreateInfo.viewportCount = 1;
    ViewportStateCreateInfo.scissorCount  = 1;

    // RasterizerState CreateInfo
    VkPipelineRasterizationStateCreateInfo RasterizerStateCreateInfo;
    if (FVulkanRasterizerState* RasterizerState = static_cast<FVulkanRasterizerState*>(Info.RasterizerState))
    {
        RasterizerStateCreateInfo = RasterizerState->GetVkCreateInfo();
    }
    else
    {
        VULKAN_ERROR_CRITICAL("RasterizerState cannot be nullptr");
        return false;
    }

    // MultiSampling CreateInfo
    VkPipelineMultisampleStateCreateInfo MultisamplingCreateInfo = {};
    MultisamplingCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisamplingCreateInfo.sampleShadingEnable   = VK_FALSE;
    MultisamplingCreateInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    MultisamplingCreateInfo.minSampleShading      = 1.0f;
    MultisamplingCreateInfo.pSampleMask           = nullptr;
    MultisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
    MultisamplingCreateInfo.alphaToOneEnable      = VK_FALSE;

    // DepthStencilState CreateInfo
    VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo;
    if (FVulkanDepthStencilState* DepthStencilState = static_cast<FVulkanDepthStencilState*>(Info.DepthStencilState))
    {
        DepthStencilStateCreateInfo = DepthStencilState->GetVkCreateInfo();
    }
    else
    {
        VULKAN_ERROR_CRITICAL("DepthStencilState cannot be nullptr");
        return false;
    }

    // BlendState CreateInfo
    VkPipelineColorBlendStateCreateInfo BlendStateCreateInfo;
    if (FVulkanBlendState* BlendState = static_cast<FVulkanBlendState*>(Info.BlendState))
    {
        BlendStateCreateInfo = BlendState->GetVkCreateInfo();
    }
    else
    {
        VULKAN_ERROR_CRITICAL("BlendState cannot be nullptr");
        return false;
    }

    // Dynamic-State CreateInfo
    VkDynamicState DynamicStates[] = 
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
    };

    VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
    DynamicStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateCreateInfo.dynamicStateCount = ARRAY_COUNT(DynamicStates);
    DynamicStateCreateInfo.pDynamicStates    = DynamicStates;

    // Retrieve a compatible RenderPass
    // NOTE: The RenderPass only needs to be compatible, and does not actually need to be the same one that actually will be used
    FVulkanRenderPassKey RenderPassKey;
    RenderPassKey.NumSamples                      = Info.MultiSampleState.SampleCount;
    RenderPassKey.DepthStencilFormat              = Info.RasterizerOutputFormats.DepthStencilFormat;
    RenderPassKey.DepthStencilActions.LoadAction  = EAttachmentLoadAction::Load;
    RenderPassKey.DepthStencilActions.StoreAction = EAttachmentStoreAction::Store;
    RenderPassKey.NumRenderTargets                = Info.RasterizerOutputFormats.NumRenderTargets;

    for (uint8 Index = 0; Index < Info.RasterizerOutputFormats.NumRenderTargets; Index++)
    {
        RenderPassKey.RenderTargetActions[Index].LoadAction  = EAttachmentLoadAction::Load;
        RenderPassKey.RenderTargetActions[Index].StoreAction = EAttachmentStoreAction::Store;
        RenderPassKey.RenderTargetFormats[Index] = Info.RasterizerOutputFormats.RenderTargetFormats[Index];
    }

    if (Info.ViewInstancingState.bEnableViewInstancing)
    {
        RenderPassKey.ViewInstancingState = Info.ViewInstancingState;
        ViewInstancingState = Info.ViewInstancingState;
    }

    VkRenderPass RenderPass = GetDevice()->GetRenderPassCache().GetRenderPass(RenderPassKey);
    if (!VULKAN_CHECK_HANDLE(RenderPass))
    {
        return false;
    }

    // Create PipelineState
    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
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
        VULKAN_ERROR_CRITICAL("Failed to create GraphicsPipeline");
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

FVulkanComputePipelineState::~FVulkanComputePipelineState()
{
}

bool FVulkanComputePipelineState::Initialize(const FRHIComputePipelineStateInfo& InInfo)
{
    FVulkanComputeShader* VulkanComputeShader = static_cast<FVulkanComputeShader*>(InInfo.Shader);
    if (!VulkanComputeShader)
    {
        VULKAN_ERROR_CRITICAL("Compute Shader cannot be nullptr");
        return false;
    }

    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo = {};
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
        VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
        return false;
    }

    // Create the ComputePipeline
    VkComputePipelineCreateInfo PipelineCreateInfo = {};
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
        VULKAN_ERROR_CRITICAL("Failed to create ComputePipeline");
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
    if (VULKAN_CHECK_HANDLE(PipelineCache))
    {
        vkDestroyPipelineCache(GetDevice()->GetVkDevice(), PipelineCache, nullptr);
        PipelineCache = VK_NULL_HANDLE;
    }
}

bool FVulkanPipelineStateManager::Initialize()
{
    if (LoadCacheFromFile())
    {
        return true;
    }
    
    VkPipelineCacheCreateInfo CreateInfo = {};
    CreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    CreateInfo.pInitialData    = nullptr;
    CreateInfo.initialDataSize = 0;

    if (GVulkanSupportsPipelineCacheControl)
    {
        CreateInfo.flags = VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT_EXT;
    }
    
    VkResult Result = vkCreatePipelineCache(GetDevice()->GetVkDevice(), &CreateInfo, nullptr, &PipelineCache);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Vulkan PipelineCache");
        return false;
    }
    else
    {
        return true;
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
    const FString PipelineCacheFilepath = FPaths::GetAssetDir() + '/' + PipelineCacheFilename;

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
            VULKAN_ERROR_CRITICAL("Failed to retrieve size of PipelineCache");
            return false;
        }
        
        TUniquePtr<uint8[]> PipelineCacheData = MakeUniquePtr<uint8[]>(PipelineCacheSize);
        Result = vkGetPipelineCacheData(GetDevice()->GetVkDevice(), PipelineCache, &PipelineCacheSize, PipelineCacheData.Get());
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("Failed to serielize PipelineCache");
            return false;
        }

        FVulkanPipelineDataHeader DataHeader;
        FMemory::Memzero(&DataHeader, sizeof(FVulkanPipelineDataHeader));

        FMemory::Memcpy(DataHeader.Magic, "VKPSO", sizeof(DataHeader.Magic));
        DataHeader.DataCRC  = CRC32::Generate(PipelineCacheData.Get(), PipelineCacheSize);
        DataHeader.DataSize = PipelineCacheSize;

        int32 BytesWritten = CacheFile->Write(reinterpret_cast<const uint8*>(&DataHeader), sizeof(FVulkanPipelineDataHeader));
        if (BytesWritten != sizeof(FVulkanPipelineDataHeader))
        {
            VULKAN_ERROR_CRITICAL("Failed to write PipelineDataHeader to disk");
            return false;
        }

        BytesWritten = CacheFile->Write(PipelineCacheData.Get(), static_cast<uint32>(PipelineCacheSize));
        if (BytesWritten != static_cast<int32>(PipelineCacheSize))
        {
            VULKAN_ERROR_CRITICAL("Failed to write PipelineCache to disk");
            return false;
        }
        else
        {
            VULKAN_INFO("Saved PipelineCache to file '%s'", *PipelineCacheFilepath);
        }
    }
    
    bPipelineCacheDirty = false;
    return true;
}

bool FVulkanPipelineStateManager::LoadCacheFromFile()
{
    const FString PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const FString PipelineCacheFilepath = FPaths::GetAssetDir() + '/' + PipelineCacheFilename;
    
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
    TUniquePtr<uint8[]> PipelineCacheData = MakeUniquePtr<uint8[]>(DataHeader.DataSize);
    BytesRead = CacheFile->Read(PipelineCacheData.Get(), static_cast<uint32>(DataHeader.DataSize));
    if (BytesRead != static_cast<int64>(DataHeader.DataSize))
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
    const uint32 DataCRC = CRC32::Generate(PipelineCacheData.Get(), DataHeader.DataSize);
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

    if (GVulkanSupportsPipelineCacheControl)
    {
        CreateInfo.flags = VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT_EXT;
    }
    
    VkResult Result = vkCreatePipelineCache(GetDevice()->GetVkDevice(), &CreateInfo, nullptr, &PipelineCache);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Vulkan PipelineCache");
        return false;
    }
    else
    {
        return true;
    }
}
