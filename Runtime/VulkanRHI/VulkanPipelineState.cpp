#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Tasks/Tasks.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Misc/Paths.h"
#include "VulkanRHI/VulkanPipelineState.h"
#include "VulkanRHI/VulkanStats.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanDeviceDebug.h"

static TAutoConsoleVariable<String> CVarPipelineCacheFileName(
    "VulkanRHI.PipelineCacheFileName",
    "FileName for the file storing the PipelineCache",
    "PipelineCache.vkpsocache");

static TAutoConsoleVariable<int32> CVarPipelineCacheSaveInterval(
    "VulkanRHI.PipelineCacheSaveInterval",
    "Minimum interval in seconds between automatic pipeline cache saves",
    30);

FVulkanInputLayoutRHI::FVulkanInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
    : FRHIInputLayout()
	, InputElements(InInputElements)
    , VertexInputBindingDescriptions()
    , VertexInputAttributeDescriptions()
    , CreateInfo{}
{
    // Create a binding for each input-slot
    for (const FRHIInputElementDesc& Element : InInputElements)
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
    Memory::Memzero(&CreateInfo);
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

FVulkanInputLayoutRHI::~FVulkanInputLayoutRHI()
{
}

void* FVulkanInputLayoutRHI::GetRHINativeState() const
{
    return nullptr;
}

const FRHIInputElementDesc* FVulkanInputLayoutRHI::GetInputElementDesc(uint32 Index) const
{
    return &InputElements[Index];
}

uint32 FVulkanInputLayoutRHI::GetNumInputElementDescs() const
{
    return InputElements.Size();
}

FVulkanDepthStencilStateRHI::FVulkanDepthStencilStateRHI(const FRHIDepthStencilStateDesc& InDesc)
    : FRHIDepthStencilState(InDesc)
{
    Memory::Memzero(&CreateInfo);
    
    CreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    CreateInfo.depthTestEnable       = InDesc.bDepthEnable;
    CreateInfo.depthWriteEnable      = InDesc.bDepthWriteEnable;
    CreateInfo.depthCompareOp        = ConvertComparisonFunc(InDesc.DepthFunc);
    CreateInfo.depthBoundsTestEnable = VK_FALSE;
    CreateInfo.stencilTestEnable     = InDesc.bStencilEnable;
    CreateInfo.front                 = ConvertStencilState(InDesc.FrontFace);
    CreateInfo.back                  = ConvertStencilState(InDesc.BackFace);
    CreateInfo.minDepthBounds        = 0.0f;
    CreateInfo.maxDepthBounds        = 1.0f;
    
    CreateInfo.front.compareMask = CreateInfo.back.compareMask = InDesc.StencilReadMask;
    CreateInfo.front.writeMask   = CreateInfo.back.writeMask   = InDesc.StencilWriteMask;
}

FVulkanDepthStencilStateRHI::~FVulkanDepthStencilStateRHI()
{
}

void* FVulkanDepthStencilStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FVulkanRasterizerStateRHI::FVulkanRasterizerStateRHI(FVulkanDevice* InDevice, const FRHIRasterizerStateDesc& InDesc)
    : FRHIRasterizerState(InDesc)
    , FVulkanDeviceChild(InDevice)
{
    Memory::Memzero(&CreateInfo);
    
    CreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    CreateInfo.rasterizerDiscardEnable = VK_FALSE;
    CreateInfo.polygonMode             = ConvertFillMode(InDesc.FillMode);
    CreateInfo.cullMode                = ConvertCullMode(InDesc.CullMode);
    CreateInfo.frontFace               = InDesc.bFrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    CreateInfo.depthBiasEnable         = InDesc.bEnableDepthBias ? VK_TRUE : VK_FALSE;
    CreateInfo.depthBiasConstantFactor = InDesc.DepthBias;
    CreateInfo.depthBiasClamp          = InDesc.DepthBiasClamp;
    CreateInfo.depthBiasSlopeFactor    = InDesc.SlopeScaledDepthBias;
    CreateInfo.lineWidth               = 1.0f;

    CreateInfo.depthClampEnable = (!InDesc.bDepthClipEnable && GVulkanSupportsDepthClamp) ? VK_TRUE : VK_FALSE;
    
#if VK_EXT_depth_clip_enable
    Memory::Memzero(&DepthClipStateCreateInfo);

    DepthClipStateCreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
    DepthClipStateCreateInfo.depthClipEnable = InDesc.bDepthClipEnable ? VK_TRUE : VK_FALSE;
    
    if (GVulkanSupportsDepthClip)
    {
        CreateInfo.depthClampEnable = GVulkanSupportsDepthClamp ? VK_TRUE : VK_FALSE;
    }
#endif
    
#if VK_EXT_conservative_rasterization
    if (GVulkanSupportsConservativeRasterization)
    {
        Memory::Memzero(&ConservativeStateCreateInfo);
        ConservativeStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
        
        if (InDesc.bEnableConservativeRaster)
        {
            ConservativeStateCreateInfo.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
        }
        else
        {
            ConservativeStateCreateInfo.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT;
        }

        ConservativeStateCreateInfo.extraPrimitiveOverestimationSize = GVulkanMaxExtraPrimitiveOverestimationSize;
    }
#endif

#if VK_EXT_depth_clip_enable
    const bool bUseDepthClipExt = GVulkanSupportsDepthClip;
    if (bUseDepthClipExt)
    {
        AddToStructChain(CreateInfo, DepthClipStateCreateInfo);
    }
#endif
#if VK_EXT_conservative_rasterization
    const bool bUseConservativeExt = GVulkanSupportsConservativeRasterization;
    if (bUseConservativeExt)
    {
        AddToStructChain(CreateInfo, ConservativeStateCreateInfo);
    }
#endif
}

FVulkanRasterizerStateRHI::~FVulkanRasterizerStateRHI()
{
}

void* FVulkanRasterizerStateRHI::GetRHINativeState() const
{
    return nullptr;
}

FVulkanBlendStateRHI::FVulkanBlendStateRHI(const FRHIBlendStateDesc& InDesc)
    : FRHIBlendState(InDesc)
{
    Memory::Memzero(&CreateInfo);

    Memory::Memzero(BlendAttachmentStates, sizeof(BlendAttachmentStates));

    // NOTE: Blend constants are configured as dynamic state
    CreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    CreateInfo.logicOpEnable   = InDesc.bLogicOpEnable;
    CreateInfo.logicOp         = ConvertLogicOp(InDesc.LogicOp);
    CreateInfo.attachmentCount = InDesc.NumRenderTargets;
    CreateInfo.pAttachments    = BlendAttachmentStates;

    for (int32 Index = 0; Index < InDesc.NumRenderTargets; Index++)
    {
        BlendAttachmentStates[Index].blendEnable         = InDesc.RenderTargets[Index].bBlendEnable ? VK_TRUE : VK_FALSE;
        BlendAttachmentStates[Index].srcColorBlendFactor = ConvertBlend(InDesc.RenderTargets[Index].SrcBlend);
        BlendAttachmentStates[Index].dstColorBlendFactor = ConvertBlend(InDesc.RenderTargets[Index].DstBlend);
        BlendAttachmentStates[Index].colorBlendOp        = ConvertBlendOp(InDesc.RenderTargets[Index].BlendOp);
        BlendAttachmentStates[Index].srcAlphaBlendFactor = ConvertBlend(InDesc.RenderTargets[Index].SrcBlendAlpha);
        BlendAttachmentStates[Index].dstAlphaBlendFactor = ConvertBlend(InDesc.RenderTargets[Index].DstBlendAlpha);
        BlendAttachmentStates[Index].alphaBlendOp        = ConvertBlendOp(InDesc.RenderTargets[Index].BlendOpAlpha);
        BlendAttachmentStates[Index].colorWriteMask      = ConvertColorWriteFlags(InDesc.RenderTargets[Index].ColorWriteMask);
    }
}

FVulkanBlendStateRHI::~FVulkanBlendStateRHI()
{
}

void* FVulkanBlendStateRHI::GetRHINativeState() const
{
    return nullptr;
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

void FVulkanPipeline::SetDebugName(const String& InName)
{
    VulkanSetObjectName(GetDevice()->GetVkDevice(), *InName, Pipeline, VK_OBJECT_TYPE_PIPELINE);
#if VULKAN_STORE_DEBUG_NAMES
    DebugName = InName;
#endif
}

FVulkanGraphicsPipelineStateRHI::FVulkanGraphicsPipelineStateRHI(FVulkanDevice* InDevice)
    : FRHIGraphicsPipelineState()
    , FVulkanPipeline(InDevice)
    , bUsesSampleLocations(false)
{
}

FVulkanGraphicsPipelineStateRHI::~FVulkanGraphicsPipelineStateRHI()
{
}

void* FVulkanGraphicsPipelineStateRHI::GetRHINativeState() const
{
    return reinterpret_cast<void*>(GetVkPipeline());
}

void FVulkanGraphicsPipelineStateRHI::SetDebugName(const String& InName)
{
    FVulkanPipeline::SetDebugName(InName);
}

void FVulkanGraphicsPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}

bool FVulkanGraphicsPipelineStateRHI::Initialize(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    // Gather Shaders for PipelineLayout
    FVulkanShader* Shaders[EShaderVisibility::Count];
    if (FVulkanVertexShaderRHI* VulkanVertexShader = FVulkanDeviceRHI::ResourceCast(InDesc.VertexShader))
    {
        Shaders[EShaderVisibility::Vertex] = VulkanVertexShader;
    }
    else
    {
        VULKAN_ERROR_CRITICAL("VertexShader cannot be nullptr");
        return false;
    }

    Shaders[EShaderVisibility::Hull]     = FVulkanDeviceRHI::ResourceCast(InDesc.HullShader);
    Shaders[EShaderVisibility::Domain]   = FVulkanDeviceRHI::ResourceCast(InDesc.DomainShader);
    Shaders[EShaderVisibility::Geometry] = FVulkanDeviceRHI::ResourceCast(InDesc.GeometryShader);
    Shaders[EShaderVisibility::Pixel]    = FVulkanDeviceRHI::ResourceCast(InDesc.PixelShader);
    
    FVulkanPipelineLayoutInfo LayoutInfo;
    LayoutInfo.AddSetForStage(VK_SHADER_STAGE_VERTEX_BIT, Shaders[EShaderVisibility::Vertex]->GetShaderInfo());
    LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_VERTEX_BIT, Shaders[EShaderVisibility::Vertex]->GetShaderInfo());

    if (Shaders[EShaderVisibility::Hull])
    {
        LayoutInfo.AddSetForStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, Shaders[EShaderVisibility::Hull]->GetShaderInfo());
        LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, Shaders[EShaderVisibility::Hull]->GetShaderInfo());
    }
    if (Shaders[EShaderVisibility::Domain])
    {
        LayoutInfo.AddSetForStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, Shaders[EShaderVisibility::Domain]->GetShaderInfo());
        LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, Shaders[EShaderVisibility::Domain]->GetShaderInfo());
    }
    if (Shaders[EShaderVisibility::Geometry])
    {
        LayoutInfo.AddSetForStage(VK_SHADER_STAGE_GEOMETRY_BIT, Shaders[EShaderVisibility::Geometry]->GetShaderInfo());
        LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_GEOMETRY_BIT, Shaders[EShaderVisibility::Geometry]->GetShaderInfo());
    }
    if (Shaders[EShaderVisibility::Pixel])
    {
        LayoutInfo.AddSetForStage(VK_SHADER_STAGE_FRAGMENT_BIT, Shaders[EShaderVisibility::Pixel]->GetShaderInfo());
        LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_FRAGMENT_BIT, Shaders[EShaderVisibility::Pixel]->GetShaderInfo());
    }
    
#if VULKAN_ENABLE_DYNAMIC_UNIFORM_BUFFERS
    LayoutInfo.PromoteUniformBuffersToDynamic();
#endif

    if (InDesc.StaticSamplers.Size() > 0)
    {
        LayoutInfo.ApplyImmutableSamplers(GetDevice(), InDesc.StaticSamplers);
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
    
    TArray<VkPipelineShaderStageCreateInfo> ShaderStages;
    if (TSharedRef<FVulkanShaderModule> ShaderModule = Shaders[EShaderVisibility::Vertex]->GetOrCreateShaderModule(PipelineLayout))
    {
        ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
        ShaderStageCreateInfo.pName  = *Shaders[EShaderVisibility::Vertex]->GetEntryPointName();
        ShaderStages.Add(ShaderStageCreateInfo);
    }
    else
    {
        VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
        return false;
    }
    
    if (Shaders[EShaderVisibility::Hull])
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = Shaders[EShaderVisibility::Hull]->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
            ShaderStageCreateInfo.pName  = *Shaders[EShaderVisibility::Hull]->GetEntryPointName();
            ShaderStages.Add(ShaderStageCreateInfo);
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
            return false;
        }
    }
    if (Shaders[EShaderVisibility::Domain])
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = Shaders[EShaderVisibility::Domain]->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
            ShaderStageCreateInfo.pName  = *Shaders[EShaderVisibility::Domain]->GetEntryPointName();
            ShaderStages.Add(ShaderStageCreateInfo);
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
            return false;
        }
    }
    if (Shaders[EShaderVisibility::Geometry])
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = Shaders[EShaderVisibility::Geometry]->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_GEOMETRY_BIT;
            ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
            ShaderStageCreateInfo.pName  = *Shaders[EShaderVisibility::Geometry]->GetEntryPointName();
            ShaderStages.Add(ShaderStageCreateInfo);
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
            return false;
        }
    }
    if (Shaders[EShaderVisibility::Pixel])
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = Shaders[EShaderVisibility::Pixel]->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
            ShaderStageCreateInfo.pName  = *Shaders[EShaderVisibility::Pixel]->GetEntryPointName();
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
    if (FVulkanInputLayoutRHI* InputLayout = FVulkanDeviceRHI::ResourceCast(InDesc.InputLayout))
    {
        VertexInputStateCreateInfo = InputLayout->GetVkCreateInfo();
    }
    else
    {
        Memory::Memzero(&VertexInputStateCreateInfo, sizeof(VkPipelineVertexInputStateCreateInfo));
        VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    }

    // InputAssembly CreateInfo
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo = {};
    InputAssemblyCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCreateInfo.topology               = ConvertPrimitiveTopology(InDesc.PrimitiveTopology);
    InputAssemblyCreateInfo.primitiveRestartEnable = InDesc.bPrimitiveRestartEnable ? VK_TRUE : VK_FALSE;

    // Viewport CreateInfo
    VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = {};
    ViewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportStateCreateInfo.viewportCount = 1;
    ViewportStateCreateInfo.scissorCount  = 1;

    // RasterizerState CreateInfo
    VkPipelineRasterizationStateCreateInfo RasterizerStateCreateInfo;
    if (FVulkanRasterizerStateRHI* RasterizerState = FVulkanDeviceRHI::ResourceCast(InDesc.RasterizerState))
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

#if VK_EXT_sample_locations
    const bool bUseSampleLocations = InDesc.MultiSampleState.bProgrammableSamplePositions
        && GVulkanSupportsSampleLocations
        && (GVulkanSampleLocationSampleCounts & MultisamplingCreateInfo.rasterizationSamples) != 0;

    VkPipelineSampleLocationsStateCreateInfoEXT SampleLocationsCreateInfo = {};
    if (bUseSampleLocations)
    {
        SampleLocationsCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT;
        SampleLocationsCreateInfo.sampleLocationsEnable     = VK_TRUE;
        SampleLocationsCreateInfo.sampleLocationsInfo.sType = VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;
        AddToStructChain(MultisamplingCreateInfo, SampleLocationsCreateInfo);
    }
#else
    const bool bUseSampleLocations = false;
#endif

    // DepthStencilState CreateInfo
    VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo;
    if (FVulkanDepthStencilStateRHI* DepthStencilState = FVulkanDeviceRHI::ResourceCast(InDesc.DepthStencilState))
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
    if (FVulkanBlendStateRHI* BlendState = FVulkanDeviceRHI::ResourceCast(InDesc.BlendState))
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
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
        VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT, // Must stay last: only counted when the pipeline opts in
    };

    VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
    DynamicStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateCreateInfo.dynamicStateCount = ARRAY_COUNT(DynamicStates) - (bUseSampleLocations ? 0 : 1);
    DynamicStateCreateInfo.pDynamicStates    = DynamicStates;

    bUsesSampleLocations = bUseSampleLocations;

    if (InDesc.ViewInstancingState.bEnableViewInstancing)
    {
        ViewInstancingState = InDesc.ViewInstancingState;
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
    PipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
    PipelineCreateInfo.basePipelineIndex   = -1;

    VkFormat ColorAttachmentFormats[RHI_MAX_RENDER_TARGETS] = {};
    
    VkPipelineRenderingCreateInfoKHR PipelineRenderingInfo = {};
    if (GVulkanUseDynamicRendering)
    {
        for (uint8 Index = 0; Index < InDesc.RasterizerOutputFormats.NumRenderTargets; Index++)
        {
            ColorAttachmentFormats[Index] = ConvertFormat(InDesc.RasterizerOutputFormats.RenderTargetFormats[Index]);
        }

        const VkFormat DepthStencilVkFormat = ConvertFormat(InDesc.RasterizerOutputFormats.DepthStencilFormat);

        PipelineRenderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        PipelineRenderingInfo.colorAttachmentCount    = InDesc.RasterizerOutputFormats.NumRenderTargets;
        PipelineRenderingInfo.pColorAttachmentFormats = ColorAttachmentFormats;
        PipelineRenderingInfo.depthAttachmentFormat   = DepthStencilVkFormat;
        PipelineRenderingInfo.stencilAttachmentFormat = IsStencilFormat(DepthStencilVkFormat) ? DepthStencilVkFormat : VK_FORMAT_UNDEFINED;

        if (GVulkanSupportsMultiviews && InDesc.ViewInstancingState.bEnableViewInstancing)
        {
            constexpr uint32 MaxArraySlices = 32;
            const uint32 NumViews = Math::Min<uint32>(InDesc.ViewInstancingState.NumArraySlices, MaxArraySlices);

            uint32 ViewMask = 0;
            for (uint32 Index = 0; Index < NumViews; Index++)
            {
                const uint32 BitIndex = InDesc.ViewInstancingState.StartRenderTargetArrayIndex + Index;
                CHECK(BitIndex < 32);
                ViewMask |= (1u << BitIndex);
            }

            PipelineRenderingInfo.viewMask = ViewMask;
        }

        PipelineCreateInfo.pNext      = &PipelineRenderingInfo;
        PipelineCreateInfo.renderPass = VK_NULL_HANDLE;
    }
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    else
    {
        FVulkanRenderPassKey RenderPassKey;
        RenderPassKey.NumSamples                      = InDesc.MultiSampleState.SampleCount;
        RenderPassKey.DepthStencilFormat              = InDesc.RasterizerOutputFormats.DepthStencilFormat;
        RenderPassKey.DepthStencilActions.LoadAction  = EAttachmentLoadAction::Load;
        RenderPassKey.DepthStencilActions.StoreAction = EAttachmentStoreAction::Store;
        RenderPassKey.NumRenderTargets                = InDesc.RasterizerOutputFormats.NumRenderTargets;

        for (uint8 Index = 0; Index < InDesc.RasterizerOutputFormats.NumRenderTargets; Index++)
        {
            RenderPassKey.RenderTargetFormats[Index]             = InDesc.RasterizerOutputFormats.RenderTargetFormats[Index];
            RenderPassKey.RenderTargetActions[Index].LoadAction  = EAttachmentLoadAction::Load;
            RenderPassKey.RenderTargetActions[Index].StoreAction = EAttachmentStoreAction::Store;
        }

        if (InDesc.ViewInstancingState.bEnableViewInstancing)
        {
            RenderPassKey.ViewInstancingState = InDesc.ViewInstancingState;
        }

        VkRenderPass RenderPass = GetDevice()->GetRenderPassCache().GetRenderPass(RenderPassKey);
        if (!VULKAN_CHECK_HANDLE(RenderPass))
        {
            return false;
        }

        PipelineCreateInfo.renderPass = RenderPass;
        PipelineCreateInfo.subpass    = 0;
    }
#endif // VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH

    FVulkanPipelineStateManager& PipelineCache = GetDevice()->GetPipelineStateManager();
    if (PipelineCache.CreateGraphicsPipeline(PipelineCreateInfo, Pipeline))
    {
        STAT_ADD(STAT_Vulkan_NumGraphicsPipelineStates, 1);
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
        STAT_ADD(STAT_Vulkan_NumGraphicsPipelineStates, 1);
        return true;
    }
}

FVulkanComputePipelineStateRHI::FVulkanComputePipelineStateRHI(FVulkanDevice* InDevice)
    : FRHIComputePipelineState()
    , FVulkanPipeline(InDevice)
{
}

FVulkanComputePipelineStateRHI::~FVulkanComputePipelineStateRHI()
{
}

void* FVulkanComputePipelineStateRHI::GetRHINativeState() const
{
    return reinterpret_cast<void*>(GetVkPipeline());
}

void FVulkanComputePipelineStateRHI::SetDebugName(const String& InName)
{
    FVulkanPipeline::SetDebugName(InName);
}

void FVulkanComputePipelineStateRHI::GetDebugName(String& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}

bool FVulkanComputePipelineStateRHI::Initialize(const FRHIComputePipelineStateDesc& InDesc)
{
    FVulkanComputeShaderRHI* VulkanComputeShader = FVulkanDeviceRHI::ResourceCast(InDesc.Shader);
    if (!VulkanComputeShader)
    {
        VULKAN_ERROR_CRITICAL("Compute Shader cannot be nullptr");
        return false;
    }

    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo = {};
    ShaderStageCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    ShaderStageCreateInfo.pName  = *VulkanComputeShader->GetEntryPointName();
    
    // PipelineLayout
    FVulkanPipelineLayoutInfo LayoutInfo;
    LayoutInfo.AddSetForStage(VK_SHADER_STAGE_COMPUTE_BIT, VulkanComputeShader->GetShaderInfo());
    LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_COMPUTE_BIT, VulkanComputeShader->GetShaderInfo());
#if VULKAN_ENABLE_DYNAMIC_UNIFORM_BUFFERS
    LayoutInfo.PromoteUniformBuffersToDynamic();
#endif

    if (InDesc.StaticSamplers.Size() > 0)
    {
        LayoutInfo.ApplyImmutableSamplers(GetDevice(), InDesc.StaticSamplers);
    }

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
        STAT_ADD(STAT_Vulkan_NumComputePipelineStates, 1);
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
        STAT_ADD(STAT_Vulkan_NumComputePipelineStates, 1);
        return true;
    }
}

FVulkanMeshletPipelineStateRHI::FVulkanMeshletPipelineStateRHI(FVulkanDevice* InDevice)
    : FRHIMeshletPipelineState()
    , FVulkanPipeline(InDevice)
    , ViewInstancingState()
    , bUsesSampleLocations(false)
{
}

FVulkanMeshletPipelineStateRHI::~FVulkanMeshletPipelineStateRHI()
{
}

void* FVulkanMeshletPipelineStateRHI::GetRHINativeState() const
{
    return reinterpret_cast<void*>(GetVkPipeline());
}

void FVulkanMeshletPipelineStateRHI::SetDebugName(const String& InName)
{
    FVulkanPipeline::SetDebugName(InName);
}

void FVulkanMeshletPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
#if VULKAN_STORE_DEBUG_NAMES
    OutDebugName = DebugName;
#else
    OutDebugName.Clear();
#endif
}

bool FVulkanMeshletPipelineStateRHI::Initialize(const FRHIMeshletPipelineStateDesc& InDesc)
{
#if VK_EXT_mesh_shader
    if (!GVulkanSupportsMeshShaders)
    {
        VULKAN_ERROR_CRITICAL("Mesh shaders are not supported on this device");
        return false;
    }

    FVulkanMeshShaderRHI* VulkanMeshShader = FVulkanDeviceRHI::ResourceCast(InDesc.MeshShader);
    if (!VulkanMeshShader)
    {
        VULKAN_ERROR_CRITICAL("MeshShader cannot be nullptr");
        return false;
    }

    FVulkanAmplificationShaderRHI* VulkanAmplificationShader = FVulkanDeviceRHI::ResourceCast(InDesc.AmplificationShader);
    FVulkanPixelShaderRHI*         VulkanPixelShader         = FVulkanDeviceRHI::ResourceCast(InDesc.PixelShader);

    if (VulkanAmplificationShader && !GVulkanSupportsTaskShaders)
    {
        VULKAN_ERROR_CRITICAL("Meshlet pipeline specifies an amplification shader but task (amplification) shaders are not supported on this device");
        return false;
    }

    // PipelineLayout
    FVulkanPipelineLayoutInfo LayoutInfo;
    if (VulkanAmplificationShader)
    {
        LayoutInfo.AddSetForStage(VK_SHADER_STAGE_TASK_BIT_EXT, VulkanAmplificationShader->GetShaderInfo());
        LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_TASK_BIT_EXT, VulkanAmplificationShader->GetShaderInfo());
    }

    LayoutInfo.AddSetForStage(VK_SHADER_STAGE_MESH_BIT_EXT, VulkanMeshShader->GetShaderInfo());
    LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_MESH_BIT_EXT, VulkanMeshShader->GetShaderInfo());

    if (VulkanPixelShader)
    {
        LayoutInfo.AddSetForStage(VK_SHADER_STAGE_FRAGMENT_BIT, VulkanPixelShader->GetShaderInfo());
        LayoutInfo.UpdateConstantsForStage(VK_SHADER_STAGE_FRAGMENT_BIT, VulkanPixelShader->GetShaderInfo());
    }

#if VULKAN_ENABLE_DYNAMIC_UNIFORM_BUFFERS
    LayoutInfo.PromoteUniformBuffersToDynamic();
#endif

    if (InDesc.StaticSamplers.Size() > 0)
    {
        LayoutInfo.ApplyImmutableSamplers(GetDevice(), InDesc.StaticSamplers);
    }

    LayoutInfo.GenerateHash();

    FVulkanPipelineLayoutManager& PipelineLayoutManager = GetDevice()->GetPipelineLayoutManager();
    PipelineLayout = PipelineLayoutManager.FindOrCreateLayout(LayoutInfo);
    if (!PipelineLayout)
    {
        return false;
    }

    // Gather ShaderModules
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo = {};
    ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    TArray<VkPipelineShaderStageCreateInfo> ShaderStages;
    if (VulkanAmplificationShader)
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = VulkanAmplificationShader->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_TASK_BIT_EXT;
            ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
            ShaderStageCreateInfo.pName  = *VulkanAmplificationShader->GetEntryPointName();
            ShaderStages.Add(ShaderStageCreateInfo);
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
            return false;
        }
    }

    if (TSharedRef<FVulkanShaderModule> ShaderModule = VulkanMeshShader->GetOrCreateShaderModule(PipelineLayout))
    {
        ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_MESH_BIT_EXT;
        ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
        ShaderStageCreateInfo.pName  = *VulkanMeshShader->GetEntryPointName();
        ShaderStages.Add(ShaderStageCreateInfo);
    }
    else
    {
        VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
        return false;
    }

    if (VulkanPixelShader)
    {
        if (TSharedRef<FVulkanShaderModule> ShaderModule = VulkanPixelShader->GetOrCreateShaderModule(PipelineLayout))
        {
            ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            ShaderStageCreateInfo.module = ShaderModule->GetVkShaderModule();
            ShaderStageCreateInfo.pName  = *VulkanPixelShader->GetEntryPointName();
            ShaderStages.Add(ShaderStageCreateInfo);
        }
        else
        {
            VULKAN_ERROR_CRITICAL("Failed to create ShaderModule");
            return false;
        }
    }

    // Viewport CreateInfo
    VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = {};
    ViewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportStateCreateInfo.viewportCount = 1;
    ViewportStateCreateInfo.scissorCount  = 1;

    // RasterizerState CreateInfo
    VkPipelineRasterizationStateCreateInfo RasterizerStateCreateInfo;
    if (FVulkanRasterizerStateRHI* RasterizerState = FVulkanDeviceRHI::ResourceCast(InDesc.RasterizerState))
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

#if VK_EXT_sample_locations
    const bool bUseSampleLocations = InDesc.MultiSampleState.bProgrammableSamplePositions
        && GVulkanSupportsSampleLocations
        && (GVulkanSampleLocationSampleCounts & MultisamplingCreateInfo.rasterizationSamples) != 0;

    VkPipelineSampleLocationsStateCreateInfoEXT SampleLocationsCreateInfo = {};
    if (bUseSampleLocations)
    {
        SampleLocationsCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT;
        SampleLocationsCreateInfo.sampleLocationsEnable     = VK_TRUE;
        SampleLocationsCreateInfo.sampleLocationsInfo.sType = VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;
        AddToStructChain(MultisamplingCreateInfo, SampleLocationsCreateInfo);
    }
#else
    const bool bUseSampleLocations = false;
#endif

    // DepthStencilState CreateInfo
    VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo;
    if (FVulkanDepthStencilStateRHI* DepthStencilState = FVulkanDeviceRHI::ResourceCast(InDesc.DepthStencilState))
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
    if (FVulkanBlendStateRHI* BlendState = FVulkanDeviceRHI::ResourceCast(InDesc.BlendState))
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
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
        VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT, // Must stay last: only counted when the pipeline opts in
    };

    VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
    DynamicStateCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateCreateInfo.dynamicStateCount = ARRAY_COUNT(DynamicStates) - (bUseSampleLocations ? 0 : 1);
    DynamicStateCreateInfo.pDynamicStates    = DynamicStates;

    bUsesSampleLocations = bUseSampleLocations;

    if (InDesc.ViewInstancingState.bEnableViewInstancing)
    {
        ViewInstancingState = InDesc.ViewInstancingState;
    }

    // Create PipelineState (mesh shading pipelines omit vertex-input and input-assembly state)
    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.stageCount          = ShaderStages.Size();
    PipelineCreateInfo.pStages             = ShaderStages.Data();
    PipelineCreateInfo.pVertexInputState   = nullptr;
    PipelineCreateInfo.pInputAssemblyState = nullptr;
    PipelineCreateInfo.pViewportState      = &ViewportStateCreateInfo;
    PipelineCreateInfo.pRasterizationState = &RasterizerStateCreateInfo;
    PipelineCreateInfo.pMultisampleState   = &MultisamplingCreateInfo;
    PipelineCreateInfo.pDepthStencilState  = &DepthStencilStateCreateInfo;
    PipelineCreateInfo.pColorBlendState    = &BlendStateCreateInfo;
    PipelineCreateInfo.pDynamicState       = &DynamicStateCreateInfo;
    PipelineCreateInfo.layout              = PipelineLayout->GetVkPipelineLayout();
    PipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
    PipelineCreateInfo.basePipelineIndex   = -1;

    VkFormat ColorAttachmentFormats[RHI_MAX_RENDER_TARGETS] = {};

    VkPipelineRenderingCreateInfoKHR PipelineRenderingInfo = {};
    if (GVulkanUseDynamicRendering)
    {
        for (uint8 Index = 0; Index < InDesc.RasterizerOutputFormats.NumRenderTargets; Index++)
        {
            ColorAttachmentFormats[Index] = ConvertFormat(InDesc.RasterizerOutputFormats.RenderTargetFormats[Index]);
        }

        const VkFormat DepthStencilVkFormat = ConvertFormat(InDesc.RasterizerOutputFormats.DepthStencilFormat);

        PipelineRenderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        PipelineRenderingInfo.colorAttachmentCount    = InDesc.RasterizerOutputFormats.NumRenderTargets;
        PipelineRenderingInfo.pColorAttachmentFormats = ColorAttachmentFormats;
        PipelineRenderingInfo.depthAttachmentFormat   = DepthStencilVkFormat;
        PipelineRenderingInfo.stencilAttachmentFormat = IsStencilFormat(DepthStencilVkFormat) ? DepthStencilVkFormat : VK_FORMAT_UNDEFINED;

        if (GVulkanSupportsMultiviews && GVulkanSupportsMeshShaderMultiview && InDesc.ViewInstancingState.bEnableViewInstancing)
        {
            constexpr uint32 MaxArraySlices = 32;
            const uint32 NumViews = Math::Min<uint32>(InDesc.ViewInstancingState.NumArraySlices, MaxArraySlices);

            uint32 ViewMask = 0;
            for (uint32 Index = 0; Index < NumViews; Index++)
            {
                const uint32 BitIndex = InDesc.ViewInstancingState.StartRenderTargetArrayIndex + Index;
                CHECK(BitIndex < 32);
                ViewMask |= (1u << BitIndex);
            }

            PipelineRenderingInfo.viewMask = ViewMask;
        }
        else if (GVulkanSupportsMultiviews && InDesc.ViewInstancingState.bEnableViewInstancing)
        {
            VULKAN_WARNING("Meshlet pipeline requested view instancing but multiviewMeshShader is not supported on this device; view instancing will be disabled for this pipeline");
        }

        PipelineCreateInfo.pNext      = &PipelineRenderingInfo;
        PipelineCreateInfo.renderPass = VK_NULL_HANDLE;
    }
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    else
    {
        FVulkanRenderPassKey RenderPassKey;
        RenderPassKey.NumSamples                      = InDesc.MultiSampleState.SampleCount;
        RenderPassKey.DepthStencilFormat              = InDesc.RasterizerOutputFormats.DepthStencilFormat;
        RenderPassKey.DepthStencilActions.LoadAction  = EAttachmentLoadAction::Load;
        RenderPassKey.DepthStencilActions.StoreAction = EAttachmentStoreAction::Store;
        RenderPassKey.NumRenderTargets                = InDesc.RasterizerOutputFormats.NumRenderTargets;

        for (uint8 Index = 0; Index < InDesc.RasterizerOutputFormats.NumRenderTargets; Index++)
        {
            RenderPassKey.RenderTargetFormats[Index]             = InDesc.RasterizerOutputFormats.RenderTargetFormats[Index];
            RenderPassKey.RenderTargetActions[Index].LoadAction  = EAttachmentLoadAction::Load;
            RenderPassKey.RenderTargetActions[Index].StoreAction = EAttachmentStoreAction::Store;
        }

        if (InDesc.ViewInstancingState.bEnableViewInstancing)
        {
            RenderPassKey.ViewInstancingState = InDesc.ViewInstancingState;
        }

        VkRenderPass RenderPass = GetDevice()->GetRenderPassCache().GetRenderPass(RenderPassKey);
        if (!VULKAN_CHECK_HANDLE(RenderPass))
        {
            return false;
        }

        PipelineCreateInfo.renderPass = RenderPass;
        PipelineCreateInfo.subpass    = 0;
    }
#endif // VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH

    FVulkanPipelineStateManager& PipelineCache = GetDevice()->GetPipelineStateManager();
    if (PipelineCache.CreateGraphicsPipeline(PipelineCreateInfo, Pipeline))
    {
        STAT_ADD(STAT_Vulkan_NumMeshletPipelineStates, 1);
        return true;
    }
    else
    {
        VULKAN_WARNING("MeshletPipeline was not found in PipelineCache");
    }

    VkResult Result = vkCreateGraphicsPipelines(GetDevice()->GetVkDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &Pipeline);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create MeshletPipeline");
        return false;
    }
    else
    {
        STAT_ADD(STAT_Vulkan_NumMeshletPipelineStates, 1);
        return true;
    }
#else
    UNREFERENCED_VARIABLE(InDesc);
    VULKAN_ERROR_CRITICAL("Mesh shaders are not supported in this build");
    return false;
#endif // VK_EXT_mesh_shader
}

FVulkanPipelineStateManager::FVulkanPipelineStateManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , PipelineCache(VK_NULL_HANDLE)
    , bPipelineCacheDirty(false)
    , LastSaveTimestamp(FPlatformTime::QueryPerformanceCounter())
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
        STAT_ADD(STAT_Vulkan_PSOCreateCount, 1);
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
        STAT_ADD(STAT_Vulkan_PSOCreateCount, 1);
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

    const String PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const String PipelineCacheFilepath = Paths::GetAssetDir() + '/' + PipelineCacheFilename;

    TFileRef<IPlatformFile> CacheFile = FPlatformFile::OpenForWrite(PipelineCacheFilepath);
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
        
        STAT_SET(STAT_Vulkan_PSOCacheSize, static_cast<int64>(PipelineCacheSize));

        TUniquePtr<uint8[]> PipelineCacheData = MakeUniquePtr<uint8[]>(PipelineCacheSize);
        Result = vkGetPipelineCacheData(GetDevice()->GetVkDevice(), PipelineCache, &PipelineCacheSize, PipelineCacheData.Get());
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("Failed to serielize PipelineCache");
            return false;
        }

        FVulkanPipelineDataHeader DataHeader;
        Memory::Memzero(&DataHeader, sizeof(FVulkanPipelineDataHeader));

        Memory::Memcpy(DataHeader.Magic, "VKPSO", sizeof(DataHeader.Magic));
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

void FVulkanPipelineStateManager::SaveCacheDataAsync()
{
    if (!VULKAN_CHECK_HANDLE(PipelineCache) || !bPipelineCacheDirty)
    {
        return;
    }

    const uint64 CurrentTime = FPlatformTime::QueryPerformanceCounter();
    const uint64 Frequency   = FPlatformTime::QueryPerformanceFrequency();
    const double ElapsedSeconds = static_cast<double>(CurrentTime - LastSaveTimestamp) / static_cast<double>(Frequency);

    const int32 SaveInterval = CVarPipelineCacheSaveInterval.GetValue();
    if (ElapsedSeconds < static_cast<double>(SaveInterval))
    {
        return;
    }

    const String PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const String PipelineCacheFilepath = Paths::GetAssetDir() + '/' + PipelineCacheFilename;

    TSharedPtr<uint8[]> SerializedData;
    SIZE_T SerializedSize = 0;

    FVulkanPipelineDataHeader DataHeader;
    Memory::Memzero(&DataHeader, sizeof(FVulkanPipelineDataHeader));

    {
        TScopedLock Lock(PipelineCacheCS);

        VkResult Result = vkGetPipelineCacheData(GetDevice()->GetVkDevice(), PipelineCache, &SerializedSize, nullptr);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("[FVulkanPipelineStateManager] Failed to retrieve size of PipelineCache for async save");
            return;
        }

        SerializedData = MakeSharedPtr<uint8[]>(static_cast<uint32>(SerializedSize));
        Result = vkGetPipelineCacheData(GetDevice()->GetVkDevice(), PipelineCache, &SerializedSize, SerializedData.Get());
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR("[FVulkanPipelineStateManager] Failed to serialize PipelineCache for async save");
            return;
        }

        bPipelineCacheDirty = false;
    }

    LastSaveTimestamp = CurrentTime;

    Memory::Memcpy(DataHeader.Magic, "VKPSO", sizeof(DataHeader.Magic));
    DataHeader.DataCRC  = CRC32::Generate(SerializedData.Get(), SerializedSize);
    DataHeader.DataSize = SerializedSize;

    Tasks::Async([FilePath = PipelineCacheFilepath, DataHeader, Data = Move(SerializedData), DataSize = SerializedSize]()
    {
        TFileRef<IPlatformFile> CacheFile = FPlatformFile::OpenForWrite(FilePath);
        if (!CacheFile)
        {
            VULKAN_WARNING("[FVulkanPipelineStateManager] Failed to open PipelineCache file for async save");
            return;
        }

        CacheFile->Write(reinterpret_cast<const uint8*>(&DataHeader), sizeof(FVulkanPipelineDataHeader));
        CacheFile->Write(Data.Get(), static_cast<uint32>(DataSize));

        VULKAN_INFO("[FVulkanPipelineStateManager] Async saved PipelineCache to '%s'", *FilePath);
    });
}

bool FVulkanPipelineStateManager::LoadCacheFromFile()
{
    const String PipelineCacheFilename = CVarPipelineCacheFileName.GetValue();
    const String PipelineCacheFilepath = Paths::GetAssetDir() + '/' + PipelineCacheFilename;
    
    TFileRef<IPlatformFile> CacheFile = FPlatformFile::OpenForRead(PipelineCacheFilepath);
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
    if (Memory::Memcmp(DataHeader.Magic, "VKPSO", sizeof(DataHeader.Magic)) != 0)
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
    if (Memory::Memcpy(Header->UUID, DeviceProperties.pipelineCacheUUID, UUIDSize) == 0)
    {
        VULKAN_WARNING("PipelineCacheHeader contains invalid UUID");
        return false;
    }
    
    VkPipelineCacheCreateInfo CreateInfo;
    Memory::Memzero(&CreateInfo);
    
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
