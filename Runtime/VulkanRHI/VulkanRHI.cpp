#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHICore.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanQuery.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanPipelineState.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanSamplerState.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanDeviceLimits.h"
#include "VulkanRHI/VulkanRayTracing.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/Platform/VulkanPlatform.h"

IMPLEMENT_ENGINE_MODULE(FVulkanRHIModule, VulkanRHI);

FRHI* FVulkanRHIModule::CreateRHI()
{
    TUniquePtr<FVulkanRHI> NewRHI = MakeUniquePtr<FVulkanRHI>();
    if (!NewRHI->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewRHI.Release();
    }
}

FVulkanRHI* FVulkanRHI::VulkanRHI = nullptr;

VkPipelineStageFlags2 FVulkanRHI::ResourceStateToPipelineStageFlags(EResourceAccess ResourceState)
{
    VkPipelineStageFlags2 AllShadersBits =
        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    VkPipelineStageFlags2 AllNonPixelShadersBits =
        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    // Add geometry shader stage if supported
    if (GVulkanGeometryShaderFeatureEnabled)
    {
        AllShadersBits         |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;
        AllNonPixelShadersBits |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;
    }

    // Add tessellation shader stages if supported
    if (GVulkanTessellationShaderFeatureEnabled)
    {
        AllShadersBits         |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;
        AllNonPixelShadersBits |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;
    }

    switch (ResourceState)
    {
        case EResourceAccess::Common:
        {
            return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        }

        case EResourceAccess::CopyDest:
        {
            return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        }

        case EResourceAccess::CopySource:
        {
            return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        }

        case EResourceAccess::DepthRead:
        {
            return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        }

        case EResourceAccess::DepthWrite:
        {
            return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        }

        case EResourceAccess::IndexBuffer:
        {
            return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
        }

        case EResourceAccess::VertexBuffer:
        {
            return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
        }

        case EResourceAccess::NonPixelShaderResource:
        {
            return AllNonPixelShadersBits;
        }

        case EResourceAccess::PixelShaderResource:
        {
            return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        }

        case EResourceAccess::Present:
        {
            return VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        }

        case EResourceAccess::RenderTarget:
        {
            return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        }

        case EResourceAccess::ResolveDest:
        {
            return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        }

        case EResourceAccess::ResolveSource:
        {
            return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        }

        case EResourceAccess::ShadingRateSource:
        {
            // Only include fragment density stage if the feature is enabled
            if (GVulkanFragmentDensityFeatureEnabled)
            {
                return VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
            }

            return VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        }

        case EResourceAccess::UnorderedAccess:
        {
            return AllShadersBits;
        }

        case EResourceAccess::ConstantBuffer:
        {
            return AllShadersBits;
        }

        case EResourceAccess::GenericRead:
        {
            return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        }

        default:
        {
            return VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        }
    }
}

VkAccessFlags2 FVulkanRHI::ResourceStateToAccessFlags(EResourceAccess ResourceState)
{
    switch (ResourceState)
    {
        case EResourceAccess::Common:
        {
            return VK_ACCESS_2_NONE;
        }

        case EResourceAccess::CopyDest:
        {
            return VK_ACCESS_2_TRANSFER_WRITE_BIT;
        }

        case EResourceAccess::CopySource:
        {
            return VK_ACCESS_2_TRANSFER_READ_BIT;
        }

        case EResourceAccess::DepthRead:
        {
            return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        }

        case EResourceAccess::DepthWrite:
        {
            return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        case EResourceAccess::IndexBuffer:
        {
            return VK_ACCESS_2_INDEX_READ_BIT;
        }

        case EResourceAccess::VertexBuffer:
        {
            return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
        }

        case EResourceAccess::NonPixelShaderResource:
        {
            return VK_ACCESS_2_SHADER_READ_BIT;
        }

        case EResourceAccess::PixelShaderResource:
        {
            return VK_ACCESS_2_SHADER_READ_BIT;
        }

        case EResourceAccess::Present:
        {
            return VK_ACCESS_2_MEMORY_READ_BIT;
        }

        case EResourceAccess::RenderTarget:
        {
            return VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        }

        case EResourceAccess::ResolveDest:
        {
            return VK_ACCESS_2_TRANSFER_WRITE_BIT;
        }

        case EResourceAccess::ResolveSource:
        {
            return VK_ACCESS_2_TRANSFER_READ_BIT;
        }

        case EResourceAccess::ShadingRateSource:
        {
            if (GVulkanFragmentDensityFeatureEnabled)
            {
                return VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
            }

            return VK_ACCESS_2_MEMORY_READ_BIT;
        }

        case EResourceAccess::UnorderedAccess:
        {
            return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
        }

        case EResourceAccess::ConstantBuffer:
        {
            return VK_ACCESS_2_UNIFORM_READ_BIT;
        }

        case EResourceAccess::GenericRead:
        {
            return VK_ACCESS_2_MEMORY_READ_BIT;
        }

        default:
        {
            return VK_ACCESS_2_NONE;
        }
    }
}

VkImageLayout FVulkanRHI::ResourceStateToImageLayout(EResourceAccess ResourceState)
{
    switch (ResourceState)
    {
        case EResourceAccess::Common:
        {
            return VK_IMAGE_LAYOUT_GENERAL;
        }

        case EResourceAccess::CopyDest:
        {
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        }

        case EResourceAccess::CopySource:
        {
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        }

        case EResourceAccess::DepthRead:
        {
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        }

        case EResourceAccess::DepthWrite:
        {
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        case EResourceAccess::IndexBuffer:
        {
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }

        case EResourceAccess::VertexBuffer:
        {
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }

        case EResourceAccess::NonPixelShaderResource:
        {
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        case EResourceAccess::PixelShaderResource:
        {
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        case EResourceAccess::Present:
        {
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        case EResourceAccess::RenderTarget:
        {
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        case EResourceAccess::ResolveDest:
        {
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        }

        case EResourceAccess::ResolveSource:
        {
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        }

        case EResourceAccess::ShadingRateSource:
        {
            if (GVulkanSupportsFragmentShadingRate)
            {
                return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
            }

            return VK_IMAGE_LAYOUT_GENERAL;
        }

        case EResourceAccess::UnorderedAccess:
        {
            return VK_IMAGE_LAYOUT_GENERAL;
        }

        case EResourceAccess::ConstantBuffer:
        {
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }

        case EResourceAccess::GenericRead:
        {
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }

        default:
        {
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }
}

FVulkanTextureRHI* FVulkanRHI::ResourceCast(FRHITexture* Texture)
{
    FVulkanTextureRHI* VulkanTexture = nullptr;
    if (Texture)
    {
        if (IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::Presentable))
        {
            VulkanTexture = static_cast<FVulkanBackBufferTexture*>(Texture);
        }
        else
        {
            VulkanTexture = static_cast<FVulkanTextureRHI*>(Texture);
        }
    }

    return VulkanTexture;
}

FVulkanTextureRHI* FVulkanRHI::ResourceCast(FVulkanCommandContext* InCommandContext, FRHITexture* Texture)
{
    FVulkanTextureRHI* VulkanTexture = nullptr;
    if (Texture)
    {
        if (IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::Presentable))
        {
            FVulkanBackBufferTexture* BackBuffer = static_cast<FVulkanBackBufferTexture*>(Texture);
            VulkanTexture = BackBuffer->GetCurrentBackBufferTexture(InCommandContext);
        }
        else
        {
            VulkanTexture = static_cast<FVulkanTextureRHI*>(Texture);
        }
    }

    return VulkanTexture;
}

FVulkanRHI::FVulkanRHI()
    : FRHI(ERHIType::Vulkan)
    , Instance()
{
    if (!VulkanRHI)
    {
        VulkanRHI = this;
    }
}

FVulkanRHI::~FVulkanRHI()
{
    const auto FlushDeletionQueue = [this]()
    {
        // NOTE: Objects could contain other objects, that now need to be flushed
        if (FRHICommandListExecutor::IsInitialized())
        {
            FRHICommandListExecutor::Get().FlushDeletedResources();
        }

        // Delete all remaining resources
        while (!DeletionQueue.IsEmpty())
        {
            TArray<FVulkanDeferredObject> Items;
            {
                TScopedLock Lock(DeletionQueueCS);
                Items = Move(DeletionQueue);
            }

            FVulkanDeferredObject::ProcessItems(Items);

            // NOTE: Objects could contain other objects, that now need to be flushed
            if (FRHICommandListExecutor::IsInitialized())
            {
                FRHICommandListExecutor::Get().FlushDeletedResources();
            }
        }
    };

    // Flush the default context before flushing the submission queue
    if (GraphicsCommandContext)
    {
        GraphicsCommandContext->Flush();
    }

    while (!PendingSubmissions.IsEmpty())
    {
        ProcessPendingCommandSubmissions();
    }

    // Flush before submitting since some objects needs the CommandContext
    FlushDeletionQueue();

    // Delete the Default Context
    SAFE_DELETE(GraphicsCommandContext);

    // Then delete all samplers
    {
        TScopedLock Lock(SamplerStateMapCS);
        SamplerStateMap.Clear();
    }

    // Then flush any potential remaining objects
    FlushDeletionQueue();

    SAFE_DELETE(GraphicsQueue);
    SAFE_DELETE(Device);
    SAFE_DELETE(PhysicalDevice);
    
    // Finally release the VkInstance
    Instance.Release();

    if (VulkanRHI == this)
    {
        VulkanRHI = nullptr;
    }
}

bool FVulkanRHI::Initialize()
{
    FVulkanInstanceCreateInfo InstanceDesc;
    InstanceDesc.RequiredExtensionNames = VulkanPlatform::GetRequiredInstanceExtensions();
    InstanceDesc.RequiredLayerNames     = VulkanPlatform::GetRequiredInstanceLayers();
    InstanceDesc.OptionalExtensionNames = VulkanPlatform::GetOptionalInstanceExtensions();
    
    bool bEnableDebugLayer = false;
    if (IConsoleVariable* CVarEnableDebugLayer = FConsoleManager::Get().FindConsoleVariable("RHI.EnableDebugLayer"))
    {
        bEnableDebugLayer = CVarEnableDebugLayer->GetBool();
    }
    
    // Turn on the DebugLayer
    if (bEnableDebugLayer)
    {
        InstanceDesc.RequiredLayerNames.Add("VK_LAYER_KHRONOS_validation");
    }
    
    // We always want to add debug utils in order to make markers work, even without the debug-layer
#if VK_EXT_debug_utils
    InstanceDesc.RequiredExtensionNames.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    if (!Instance.Initialize(InstanceDesc))
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize VulkanInstance");
        return false;
    }
    
    // Load functions that requires an instance here
    if (!VulkanLoader::LoadInstanceFunctions(GetInstance()))
    {
        return false;
    }

    FVulkanDeviceCreateInfo DeviceCreateInfo;
    DeviceCreateInfo.RequiredExtensionNames = VulkanPlatform::GetRequiredDeviceExtensions();
    DeviceCreateInfo.OptionalExtensionNames = VulkanPlatform::GetOptionalDeviceExtensions();
    
    // -------------------------------------------------------------------------------------------
    // Enable required features (These are necessary to run)
    // -------------------------------------------------------------------------------------------

    // Vulkan 1.0 Required
    DeviceCreateInfo.RequiredFeatures.samplerAnisotropy                    = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.shaderImageGatherExtended            = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.imageCubeArray                       = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.depthBiasClamp                       = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.shaderStorageImageReadWithoutFormat  = VK_TRUE;
    // Enable if supported by the device
    DeviceCreateInfo.OptionalFeatures.depthClamp                            = VK_TRUE;
    // Vulkan 1.0 Optional
    DeviceCreateInfo.OptionalFeatures.geometryShader                       = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.tessellationShader                   = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.multiDrawIndirect                    = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.robustBufferAccess                   = VK_TRUE;

    // Vulkan 1.1 Required
    DeviceCreateInfo.RequiredFeatures11.shaderDrawParameters               = VK_TRUE;
    // Vulkan 1.1 Optional
    DeviceCreateInfo.OptionalFeatures11.multiview                          = VK_TRUE;

    // Vulkan 1.2 Required 
    DeviceCreateInfo.RequiredFeatures12.hostQueryReset                     = VK_TRUE; 
    DeviceCreateInfo.RequiredFeatures12.bufferDeviceAddress                = VK_TRUE; 
    DeviceCreateInfo.RequiredFeatures12.shaderOutputLayer                  = VK_TRUE; 
    DeviceCreateInfo.RequiredFeatures12.timelineSemaphore                  = VK_TRUE; 
    // Vulkan 1.2 Optional 
    DeviceCreateInfo.OptionalFeatures12.descriptorIndexing                 = VK_TRUE; 

    // Vulkan 1.3 Required
    DeviceCreateInfo.RequiredFeatures13.dynamicRendering                   = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures13.synchronization2                   = VK_TRUE;
    // Vulkan 1.3 Optional
    DeviceCreateInfo.OptionalFeatures13.pipelineCreationCacheControl       = VK_TRUE;

    // Create physical device
    PhysicalDevice = new FVulkanPhysicalDevice(GetInstance());
    if (!PhysicalDevice->Initialize(DeviceCreateInfo))
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize VulkanPhyscicalDevice");
        return false;
    }

    Device = new FVulkanDevice(GetInstance(), GetPhysicalDevice());
    if (!Device->Initialize(DeviceCreateInfo))
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize VulkanDevice");
        return false;
    }
    
    // Load functions that requires a device here (Order is important)
    if (!VulkanLoader::LoadDeviceFunctions(Device))
    {
        return false;
    }

    // Initialize parts of the device that require device functions to be present
    if (!Device->PostLoaderInitalize())
    {
        VULKAN_ERROR_CRITICAL("Failed to PostLoaderInitalize failed to VulkanDevice");
        return false;
    }

    // Initialize Queues
    GraphicsQueue = new FVulkanQueue(Device, EVulkanCommandQueueType::Graphics);
    if (!GraphicsQueue->Initialize())
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize VulkanQueue [Graphics]");
        return false;
    }
    else
    {
        GraphicsQueue->SetDebugName("Graphics Queue");
    }

    // Initialize Default CommandContext
    GraphicsCommandContext = new FVulkanCommandContext(Device, *GraphicsQueue);
    if (!GraphicsCommandContext->Initialize())
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize VulkanCommandContext");
        return false;
    }

    // Initialize DefaultResources
    if (!Device->InitializeDefaultResources(*GraphicsCommandContext))
    {
        return false;
    }

    return true;
}

void FVulkanRHI::BeginFrame()
{
    // Update timestamp period, this is necessary on MoltenVK in order to get correct measurements
    {
        VkPhysicalDeviceProperties Properties;
        vkGetPhysicalDeviceProperties(PhysicalDevice->GetVkPhysicalDevice(), &Properties);
        VulkanDeviceLimits::TimestampPeriod = Properties.limits.timestampPeriod;
    }
}

void FVulkanRHI::EndFrame()
{
    // NOTE: Empty for now
}

FRHITexture* FVulkanRHI::CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FVulkanTextureRHIRef NewTexture = new FVulkanTextureRHI(GetDevice(), InTextureDesc);
    if (InTextureDesc.bEnableResourceStateTracking)
    {
        NewTexture->EnableStateTracking(InInitialState);
    }

    if (!NewTexture->Initialize(GraphicsCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewTexture.ReleaseOwnership();
    }
}

FRHIBuffer* FVulkanRHI::CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData)
{
    FVulkanBufferRHIRef NewBuffer = new FVulkanBufferRHI(GetDevice(), InBufferDesc, InInitialState);
    if (InBufferDesc.bEnableResourceStateTracking)
    {
        NewBuffer->EnableStateTracking(InInitialState);
    }
    
    if (!NewBuffer->Initialize(GraphicsCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FVulkanRHI::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
{
    TScopedLock Lock(SamplerStateMapCS);

    TSharedRef<FVulkanSamplerStateRHI> Result;

    // Check if there already is an existing sampler state with this description
    if (TSharedRef<FVulkanSamplerStateRHI>* ExistingSamplerState = SamplerStateMap.Find(InSamplerDesc))
    {
        Result = *ExistingSamplerState;
    }
    else
    {
        Result = new FVulkanSamplerStateRHI(GetDevice(), InSamplerDesc);
        if (!Result->Initialize())
        {
            return nullptr;
        }
        else
        {
            SamplerStateMap.Add(InSamplerDesc, Result);
        }
    }

    return Result.ReleaseOwnership();
}

FRHISwapChain* FVulkanRHI::CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc)
{
    CHECK(InSwapChainDesc.WindowHandle != nullptr);

    FVulkanSwapChainRHIRef NewSwapChain = new FVulkanSwapChainRHI(Device, InSwapChainDesc);
    if (!NewSwapChain->Initialize(GraphicsCommandContext))
    {
        return nullptr;
    }
    else
    {
        return NewSwapChain.ReleaseOwnership();
    }
}

FRHIQuery* FVulkanRHI::CreateQuery(EQueryType InQueryType)
{
    return new FVulkanQueryRHI(Device, InQueryType);
}

FRHIFence* FVulkanRHI::CreateFence()
{
    FVulkanFenceRHI* NewFence = new FVulkanFenceRHI(Device);
    if (!NewFence->Initialize())
    {
        delete NewFence;
        return nullptr;
    }

    return NewFence;
}

FRHISceneAccelerationStructure* FVulkanRHI::CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(InSceneDesc);
    return nullptr;
}

FRHIGeometryAccelerationStructure* FVulkanRHI::CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
{
    FRHIGeometryAccelerationStructureBuildDesc BuildDesc;
    BuildDesc.VertexBuffer = InGeometryDesc.VertexBuffer;
    BuildDesc.NumVertices  = InGeometryDesc.NumVertices;
    BuildDesc.IndexBuffer  = InGeometryDesc.IndexBuffer;
    BuildDesc.NumIndices   = InGeometryDesc.NumIndices;
    BuildDesc.IndexFormat  = InGeometryDesc.IndexFormat;
    BuildDesc.bUpdate      = false;

    GraphicsCommandContext->StartContext();

    FVulkanGeometryAccelerationStructureRHIRef NewGeometry = new FVulkanGeometryAccelerationStructureRHI(GetDevice(), InGeometryDesc);
    if (!NewGeometry->Build(*GraphicsCommandContext, BuildDesc))
    {
        DEBUG_BREAK();
        NewGeometry.Reset();
    }

    GraphicsCommandContext->FinishContext();
    return NewGeometry.ReleaseOwnership();
}

FRHIShaderResourceView* FVulkanRHI::CreateShaderResourceView(const FRHIShaderResourceViewDesc& InDesc)
{
    FRHIResource* Resource = nullptr;
    if (InDesc.IsBufferSRV())
    {
        Resource = InDesc.BufferSRV.Buffer;
    }
    else if (InDesc.IsTextureSRV())
    {
        Resource = InDesc.TextureSRV.Texture;
    }
    else
    {
        return nullptr;
    }

    CHECK(Resource != nullptr);

    FVulkanShaderResourceViewRHIRef NewShaderResourceView = new FVulkanShaderResourceViewRHI(GetDevice(), Resource);
    if (!NewShaderResourceView->InitializeSRV(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewShaderResourceView.ReleaseOwnership();
    }
}

FRHIUnorderedAccessView* FVulkanRHI::CreateUnorderedAccessView(const FRHIUnorderedAccessViewDesc& InDesc)
{
    FRHIResource* Resource = nullptr;
    if (InDesc.IsBufferUAV())
    {
        Resource = InDesc.BufferUAV.Buffer;
    }
    else if (InDesc.IsTextureUAV())
    {
        Resource = InDesc.TextureUAV.Texture;
    }
    else
    {
        return nullptr;
    }

    CHECK(Resource != nullptr);

    FVulkanUnorderedAccessViewRHIRef NewUnorderedAccessView = new FVulkanUnorderedAccessViewRHI(GetDevice(), Resource);
    if (!NewUnorderedAccessView->InitializeUAV(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewUnorderedAccessView.ReleaseOwnership();
    }
}

FRHIComputeShader* FVulkanRHI::CreateComputeShader(const TArray<uint8>& ShaderCode)
{
    FVulkanComputeShaderRHIRef NewShader = new FVulkanComputeShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIVertexShader* FVulkanRHI::CreateVertexShader(const TArray<uint8>& ShaderCode)
{
    FVulkanVertexShaderRHIRef NewShader = new FVulkanVertexShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIHullShader* FVulkanRHI::CreateHullShader(const TArray<uint8>& ShaderCode)
{
    FVulkanHullShaderRHIRef NewShader = new FVulkanHullShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDomainShader* FVulkanRHI::CreateDomainShader(const TArray<uint8>& ShaderCode)
{
    FVulkanDomainShaderRHIRef NewShader = new FVulkanDomainShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIGeometryShader* FVulkanRHI::CreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    FVulkanGeometryShaderRHIRef NewShader = new FVulkanGeometryShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIMeshShader* FVulkanRHI::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIAmplificationShader* FVulkanRHI::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIPixelShader* FVulkanRHI::CreatePixelShader(const TArray<uint8>& ShaderCode)
{
    FVulkanPixelShaderRHIRef NewShader = new FVulkanPixelShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayGenShader* FVulkanRHI::CreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    FVulkanRayGenShaderRHIRef NewShader = new FVulkanRayGenShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayAnyHitShader* FVulkanRHI::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    FVulkanRayAnyHitShaderRHIRef NewShader = new FVulkanRayAnyHitShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayClosestHitShader* FVulkanRHI::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    FVulkanRayClosestHitShaderRHIRef NewShader = new FVulkanRayClosestHitShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayMissShader* FVulkanRHI::CreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    FVulkanRayMissShaderRHIRef NewShader = new FVulkanRayMissShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FVulkanRHI::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
{
    return new FVulkanDepthStencilStateRHI(InDesc);
}

FRHIRasterizerState* FVulkanRHI::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return new FVulkanRasterizerStateRHI(GetDevice(), InDesc);
}

FRHIBlendState* FVulkanRHI::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return new FVulkanBlendStateRHI(InDesc);
}

FRHIInputLayout* FVulkanRHI::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return new FVulkanInputLayoutRHI(InInputElements);
}

FRHIGraphicsPipelineState* FVulkanRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
{
    FVulkanGraphicsPipelineStateRHIRef NewPipeline = new FVulkanGraphicsPipelineStateRHI(GetDevice());
    if (!NewPipeline->Initialize(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewPipeline.ReleaseOwnership();
    }
}

FRHIComputePipelineState* FVulkanRHI::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
{
    FVulkanComputePipelineStateRHIRef NewPipeline = new FVulkanComputePipelineStateRHI(GetDevice());
    if (!NewPipeline->Initialize(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewPipeline.ReleaseOwnership();
    }
}

FRHIRayTracingPipelineState* FVulkanRHI::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& /*InDesc*/ )
{
    return new FVulkanRayTracingPipelineStateRHI();
}

bool FVulkanRHI::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const 
{
    if (!Device->IsExtensionEnabled(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
    {
        VULKAN_WARNING("[FVulkanRHI] VK_EXT_memory_budget is required to query video-memory information");
        return false;
    }

    VkPhysicalDeviceMemoryProperties2 MemoryProperties2;
    FMemory::Memzero(&MemoryProperties2);
    MemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

    VkPhysicalDeviceMemoryBudgetPropertiesEXT MemoryBudgetProperties;
    FMemory::Memzero(&MemoryBudgetProperties);
    MemoryBudgetProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

    MemoryProperties2.pNext = &MemoryBudgetProperties;

    vkGetPhysicalDeviceMemoryProperties2(PhysicalDevice->GetVkPhysicalDevice(), &MemoryProperties2);

    OutMemoryInfo.MemoryType   = MemoryType;
    OutMemoryInfo.MemoryUsage  = 0;
    OutMemoryInfo.MemoryBudget = 0;

    const VkPhysicalDeviceMemoryProperties& memoryProperties = MemoryProperties2.memoryProperties;
    for (uint32 Index = 0; Index < memoryProperties.memoryHeapCount; Index++)
    {
        if (MemoryType == EVideoMemoryType::Local)
        {
            const VkMemoryHeap& MemoryHeap = memoryProperties.memoryHeaps[Index];
            if (MemoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                OutMemoryInfo.MemoryBudget += MemoryBudgetProperties.heapBudget[Index];
                OutMemoryInfo.MemoryUsage  += MemoryBudgetProperties.heapUsage[Index];
            }
        }
        else
        {
            OutMemoryInfo.MemoryBudget += MemoryBudgetProperties.heapBudget[Index];
            OutMemoryInfo.MemoryUsage  += MemoryBudgetProperties.heapUsage[Index];
        }
    }

    return true;
}

bool FVulkanRHI::QueryUAVFormatSupport(EFormat Format) const
{
    VkFormat VulkanFormat = ConvertFormat(Format);
    if (VulkanFormat != VK_FORMAT_UNDEFINED)
    {
        VkFormatProperties FormatProperties = PhysicalDevice->GetFormatProperties(VulkanFormat);
        if ((FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    
    return false;
}

bool FVulkanRHI::GetQueryResult(FRHIQuery* Query, uint64& OutResult)
{
    FVulkanQueryRHI* VulkanQuery = ResourceCast(Query);
    if (!VulkanQuery)
    {
        return false;
    }

    OutResult = VulkanQuery->Result;
    return true;
}

FString FVulkanRHI::GetAdapterName() const
{
    if (!PhysicalDevice)
    {
        VULKAN_ERROR_CRITICAL("PhysicalDevice is not initialized properly");
        return FString();
    }

    VkPhysicalDeviceProperties DeviceProperties = PhysicalDevice->GetProperties();
    return FString(DeviceProperties.deviceName);
}

IRHICommandContext* FVulkanRHI::ObtainCommandContext()
{
    CHECK(GraphicsCommandContext != nullptr);
    return GraphicsCommandContext;
}

void* FVulkanRHI::GetNativeAdapter()
{
    CHECK(PhysicalDevice != nullptr);
    return reinterpret_cast<void*>(PhysicalDevice->GetVkPhysicalDevice());
}

void* FVulkanRHI::GetNativeDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetVkDevice());
}

void* FVulkanRHI::GetNativeDirectCommandQueue()
{
    CHECK(GraphicsQueue != nullptr);
    return reinterpret_cast<void*>(GraphicsQueue->GetVkQueue());
}

void* FVulkanRHI::GetNativeComputeCommandQueue()
{
    // TODO: Finish
    CHECK(false);
    return nullptr;
}

void* FVulkanRHI::GetNativeCopyCommandQueue()
{
    // TODO: Finish
    CHECK(false);
    return nullptr;
}

void FVulkanRHI::EnqueueResourceDeletion(FRHIResource* Resource)
{
    if (Resource)
    {
        DeferDeletion(Resource);
    }
}

void FVulkanRHI::ProcessPendingCommandSubmissions()
{
    bool bProcess = true;
    while (bProcess)
    {
        FVulkanCommandSubmission* CommandSubmission = nullptr;
        if (PendingSubmissions.Peek(CommandSubmission))
        {
            CHECK(CommandSubmission != nullptr);
            if (!CommandSubmission->IsExecutionFinished())
            {
                bProcess = false;
                break;
            }
            else
            {
                // If we are finished we remove the item from the queue
                PendingSubmissions.Dequeue();
                CommandSubmission->Finish();
            }
        }
        else
        {
            bProcess = false;
        }
    }
}

void FVulkanRHI::SubmitCommands(FVulkanCommandSubmission* CommandSubmission, bool bFlushDeletionQueue)
{
    CHECK(CommandSubmission != nullptr);

    if (!CommandSubmission->IsEmpty())
    {
        if (bFlushDeletionQueue)
        {
            TScopedLock Lock(DeletionQueueCS);
            CommandSubmission->DeletionQueue = Move(DeletionQueue);
        }

        CommandSubmission->Submit();
        
        PendingSubmissions.Enqueue(CommandSubmission);
    }
}
