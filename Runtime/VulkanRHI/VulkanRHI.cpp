#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanExtensions.h"
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
#include "VulkanRHI/VulkanStats.h"
#include "RHI/RHIStats.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/Platform/VulkanPlatform.h"

IMPLEMENT_ENGINE_MODULE(FVulkanRHIModule, VulkanRHI);

static TAutoConsoleVariable<int32> CVarMaxDefragMovesPerFrame(
    "VulkanRHI.MaxDefragMovesPerFrame",
    "Maximum number of texture defragmentation moves per frame (0 to disable)",
    4);

static TAutoConsoleVariable<bool> CVarVulkanUseDynamicRendering(
    "VulkanRHI.UseDynamicRendering",
    "Use VK_KHR_dynamic_rendering instead of VkRenderPass and VkFramebuffer",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarVulkanEnableRobustBufferAccess(
    "VulkanRHI.EnableRobustBufferAccess",
    "Enable Vulkan robustBufferAccess feature. Costs more user-data DWORDs per dynamic uniform buffer.",
#if RELEASE_BUILD
    false);
#else
    true);
#endif

#if VULKAN_ENABLE_CRASH_MARKERS
static TAutoConsoleVariable<bool> CVarVulkanEnableCrashMarkers(
    "VulkanRHI.EnableCrashMarkers",
    "Enable GPU crash markers for post-mortem debugging. Requires VK_AMD_buffer_marker or VK_NV_device_diagnostic_checkpoints.",
    true);
#endif

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

FVulkanRHI* FVulkanRHI::GVulkanRHI = nullptr;

FVulkanTextureRHI* FVulkanRHI::ResourceCast(FRHITexture* Texture)
{
    if (Texture)
    {
        return static_cast<FVulkanTextureBase*>(Texture)->GetTextureInterface();
    }

    return nullptr;
}

FVulkanRenderTargetViewRHI* FVulkanRHI::ResourceCast(FRHIRenderTargetView* RenderTargetView)
{
    if (RenderTargetView)
    {
        return static_cast<FVulkanRenderTargetViewBase*>(RenderTargetView)->GetRenderTargetViewInterface();
    }

    return nullptr;
}

FVulkanRHI::FVulkanRHI()
    : FRHI(ERHIType::Vulkan)
    , Instance()
#if VK_EXT_debug_utils
    , DebugMessenger(VK_NULL_HANDLE)
#endif
    , PhysicalDevice(nullptr)
    , Device(nullptr)
    , GraphicsQueue(nullptr)
    , PresentQueue(nullptr)
    , GraphicsCommandContext(nullptr)
#if VULKAN_ENABLE_CRASH_MARKERS
    , CrashMarkers(nullptr)
#endif
{
    if (!GVulkanRHI)
    {
        GVulkanRHI = this;
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
        while (!DeferredObjects.IsEmpty())
        {
            TArray<FVulkanDeferredObject> Items;
            {
                TScopedLock Lock(DeferredObjectsCS);
                Items = Move(DeferredObjects);
            }

            FVulkanDeferredObject::ProcessItems(Device, Items);

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

    if (GraphicsQueue)
    {
        GraphicsQueue->ProcessCommandQueue();
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

#if VULKAN_ENABLE_CRASH_MARKERS
    SAFE_DELETE(CrashMarkers);
#endif

    if (PresentQueue != GraphicsQueue)
    {
        SAFE_DELETE(PresentQueue);
    }
    else
    {
        PresentQueue = nullptr;
    }

    SAFE_DELETE(GraphicsQueue);
    SAFE_DELETE(Device);
    SAFE_DELETE(PhysicalDevice);

#if VK_EXT_debug_utils
    VulkanDestroyDebugMessenger(Instance.GetVkInstance(), DebugMessenger);
#endif
    
    Instance.Release();

    if (GVulkanRHI == this)
    {
        GVulkanRHI = nullptr;
    }
}

bool FVulkanRHI::Initialize()
{
    // -------------------------------------------------------------------------------------------
    // Build instance create info
    // -------------------------------------------------------------------------------------------

    FVulkanInstanceCreateInfo InstanceCreateInfo;
    InstanceCreateInfo.RequiredLayerNames = VulkanPlatform::GetRequiredInstanceLayers();
    InstanceCreateInfo.OptionalLayerNames = VulkanPlatform::GetOptionalInstanceLayers();

    // Retrieve platform specific extensions
    VulkanPlatform::RetrieveInstanceExtensions(InstanceCreateInfo.Extensions);
    
    // Register extensions that the engine wants to use
    FVulkanInstanceExtension::RegisterExtensions(InstanceCreateInfo.Extensions);
    
    bool bEnableDebugLayer = false;
    if (IConsoleVariable* CVarEnableDebugLayer = FConsoleManager::Get().FindConsoleVariable("RHI.EnableDebugLayer"))
    {
        bEnableDebugLayer = CVarEnableDebugLayer->GetBool();
    }
    
    if (bEnableDebugLayer)
    {
        InstanceCreateInfo.RequiredLayerNames.Add(VULKAN_VALIDATION_LAYER_NAME);
    }

    if (!Instance.Initialize(InstanceCreateInfo))
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize VulkanInstance");
        return false;
    }
    
    if (!VulkanLoader::LoadInstanceFunctions(&Instance))
    {
        return false;
    }

#if VK_EXT_debug_utils
    GVulkanSupportsDebugUtils = Instance.IsExtensionEnabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    VulkanCreateDebugMessenger(Instance.GetVkInstance(), DebugMessenger);
#endif

    // -------------------------------------------------------------------------------------------
    // Build device create info
    // -------------------------------------------------------------------------------------------

    FVulkanDeviceCreateInfo DeviceCreateInfo;
    DeviceCreateInfo.RequiredLayerNames = VulkanPlatform::GetRequiredDeviceLayers();
    DeviceCreateInfo.OptionalLayerNames = VulkanPlatform::GetOptionalDeviceLayers();

    // Retrieve platform specific extensions
    VulkanPlatform::RetrieveDeviceExtensions(DeviceCreateInfo.Extensions);
    
    // Register extensions that the engine wants to use
    FVulkanDeviceExtension::RegisterExtensions(DeviceCreateInfo.Extensions);

	// -------------------------------------------------------------------------------------------
    // Enable required features (These are necessary to run)
    // -------------------------------------------------------------------------------------------

    // Vulkan 1.0 Required
    DeviceCreateInfo.RequiredFeatures.Features10.samplerAnisotropy                    = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.Features10.shaderImageGatherExtended            = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.Features10.imageCubeArray                       = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.Features10.depthBiasClamp                       = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.Features10.shaderStorageImageWriteWithoutFormat = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.Features10.shaderStorageImageReadWithoutFormat  = VK_TRUE;
    
    // Vulkan 1.0 Optional
    DeviceCreateInfo.OptionalFeatures.Features10.geometryShader          = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features10.tessellationShader      = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features10.multiDrawIndirect       = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features10.pipelineStatisticsQuery = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features10.depthClamp              = VK_TRUE;
    
#ifndef RELEASE_BUILD
    if (CVarVulkanEnableRobustBufferAccess.GetValue())
    {
        DeviceCreateInfo.OptionalFeatures.Features10.robustBufferAccess = VK_TRUE;
    }
#else
    {
        DeviceCreateInfo.OptionalFeatures.Features10.robustBufferAccess = VK_FALSE;
    }
#endif

    // Vulkan 1.1 Required
    DeviceCreateInfo.RequiredFeatures.Features11.shaderDrawParameters = VK_TRUE;
    
    // Vulkan 1.1 Optional
    DeviceCreateInfo.OptionalFeatures.Features11.multiview = VK_TRUE;

    // Vulkan 1.2 Required 
    DeviceCreateInfo.RequiredFeatures.Features12.hostQueryReset      = VK_TRUE; 
    DeviceCreateInfo.RequiredFeatures.Features12.bufferDeviceAddress = VK_TRUE; 
    DeviceCreateInfo.RequiredFeatures.Features12.shaderOutputLayer   = VK_TRUE; 
    DeviceCreateInfo.RequiredFeatures.Features12.timelineSemaphore   = VK_TRUE; 
    
    // Vulkan 1.2 Optional 
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorIndexing  = VK_TRUE; 

    // Vulkan 1.3 Required
    DeviceCreateInfo.RequiredFeatures.Features13.dynamicRendering = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.Features13.synchronization2 = VK_TRUE;
    DeviceCreateInfo.RequiredFeatures.Features13.maintenance4     = VK_TRUE;

    // Vulkan 1.3 Optional
    DeviceCreateInfo.OptionalFeatures.Features13.pipelineCreationCacheControl = VK_TRUE;

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

#if VULKAN_ENABLE_CRASH_MARKERS
    if (Device->IsCrashMarkerExtensionsEnabled() && CVarVulkanEnableCrashMarkers.GetValue())
    {
        CrashMarkers = new FVulkanCrashMarkers(Device);
        if (!CrashMarkers->Initialize(*GraphicsQueue))
        {
            delete CrashMarkers;
            CrashMarkers = nullptr;
        }
    }
#endif

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

    GraphicsQueue->ProcessCommandQueue();

#if VULKAN_USE_DESCRIPTOR_CACHE
    Device->GetDescriptorSetCache().EvictStaleDescriptorSets(0);
#else
    Device->GetDescriptorPoolManager().EvictUnusedPools();
#endif

    if (!GVulkanUseDynamicRendering)
    {
        Device->GetRenderPassCache().EvictStaleFramebuffers();
    }

    // NOTE: Currently only GraphicsCommandContext exists. When additional contexts
    // are added (async compute, copy), iterate all contexts here.
    GraphicsCommandContext->GetContextState().EvictStaleDescriptorStates();

    Device->GetMemoryManager().CleanUpAllocators();

    const int32 MaxDefragMoves = CVarMaxDefragMovesPerFrame.GetValue();
    if (MaxDefragMoves > 0)
    {
        Device->GetMemoryManager().DefragmentAllocations(GraphicsCommandContext, MaxDefragMoves);
    }

    Device->GetFrameFence().Signal(*GraphicsQueue);
}

void FVulkanRHI::EndFrame()
{
    if (Device)
    {
        Device->GetPipelineStateManager().SaveCacheDataAsync();

#if VULKAN_ENABLE_STATS
        Device->GetMemoryManager().UpdateMemoryStats();

        {
            FRHIVideoMemoryInfo LocalMemory;
            if (QueryVideoMemoryInfo(EVideoMemoryType::Local, LocalMemory))
            {
                STAT_SET(STAT_RHI_LocalMemoryBudget, LocalMemory.MemoryBudget);
                STAT_SET(STAT_RHI_LocalMemoryUsage,  LocalMemory.MemoryUsage);
            }

            FRHIVideoMemoryInfo NonLocalMemory;
            if (QueryVideoMemoryInfo(EVideoMemoryType::NonLocal, NonLocalMemory))
            {
                STAT_SET(STAT_RHI_NonLocalMemoryBudget, NonLocalMemory.MemoryBudget);
                STAT_SET(STAT_RHI_NonLocalMemoryUsage,  NonLocalMemory.MemoryUsage);
            }
        }
#endif
    }
}

FRHITexture* FVulkanRHI::CreateTexture(const FRHITextureDesc& InTextureDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FVulkanTextureRHIRef NewTexture = new FVulkanTextureRHI(GetDevice(), InTextureDesc);
    if (!NewTexture->Initialize(GraphicsCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }

#if VULKAN_ENABLE_STATS
    {
        const int64 AllocatedSize = static_cast<int64>(NewTexture->GetMemoryStorage().GetSize());
        if (InTextureDesc.IsRenderTarget() || InTextureDesc.IsDepthStencil())
        {
            STAT_ADD(STAT_RHI_RenderTargetMemory, AllocatedSize);
        }
        else
        {
            STAT_ADD(STAT_RHI_TextureMemory, AllocatedSize);
        }
    }
#endif

    TickCoreProgression();
    return NewTexture.ReleaseOwnership();
}

FRHIBuffer* FVulkanRHI::CreateBuffer(const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState, const void* InInitialData)
{
    FVulkanBufferRHIRef NewBuffer = new FVulkanBufferRHI(GetDevice(), InBufferDesc);
    if (!NewBuffer->Initialize(GraphicsCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }

#if VULKAN_ENABLE_STATS
    {
        const int64 AllocatedSize = static_cast<int64>(NewBuffer->GetMemoryStorage().GetSize());
        if (InBufferDesc.IsVertexBuffer())
        {
            STAT_ADD(STAT_RHI_VertexBufferMemory, AllocatedSize);
        }
        else if (InBufferDesc.IsIndexBuffer())
        {
            STAT_ADD(STAT_RHI_IndexBufferMemory, AllocatedSize);
        }
        else if (InBufferDesc.IsConstantBuffer())
        {
            STAT_ADD(STAT_RHI_ConstantBufferMemory, AllocatedSize);
        }
        else if (InBufferDesc.IsShaderResourceBuffer() || InBufferDesc.IsUnorderedAccessBuffer())
        {
            STAT_ADD(STAT_RHI_StructuredBufferMemory, AllocatedSize);
        }
        else
        {
            STAT_ADD(STAT_RHI_MiscBufferMemory, AllocatedSize);
        }

        if (InBufferDesc.IsReadBack())
        {
            STAT_ADD(STAT_RHI_ReadbackMemory, AllocatedSize);
        }
        if (InBufferDesc.IsDynamic() || InBufferDesc.IsTransient())
        {
            STAT_ADD(STAT_RHI_UploadMemory, AllocatedSize);
        }
    }
#endif

    TickCoreProgression();
    return NewBuffer.ReleaseOwnership();
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

    FVulkanSwapChainRHIRef NewSwapChain = new FVulkanSwapChainRHI(Device, GraphicsCommandContext, InSwapChainDesc);
    if (!NewSwapChain->Initialize())
    {
        return nullptr;
    }

    EnsurePresentQueue();
    return NewSwapChain.ReleaseOwnership();
}

bool FVulkanRHI::EnsurePresentQueue()
{
    if (PresentQueue)
    {
        return true;
    }

    TOptional<FVulkanQueueFamilyIndices> QueueIndices = Device->GetQueueIndicies();
    if (!QueueIndices || QueueIndices->PresentQueueIndex == uint32(~0))
    {
        return false;
    }

    if (!QueueIndices->HasSeparatePresentQueue())
    {
        PresentQueue = GraphicsQueue;
        return true;
    }

    PresentQueue = new FVulkanQueue(Device, EVulkanCommandQueueType::Present);
    if (!PresentQueue->Initialize())
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize present queue");
        SAFE_DELETE(PresentQueue);
        return false;
    }

    PresentQueue->SetDebugName("Present Queue");
    VULKAN_INFO("Created separate present queue (family=%u)", QueueIndices->PresentQueueIndex);
    return true;
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

    TickCoreProgression();
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
    if (!NewShaderResourceView->Initialize(InDesc))
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
    if (!NewUnorderedAccessView->Initialize(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewUnorderedAccessView.ReleaseOwnership();
    }
}

FRHIRenderTargetView* FVulkanRHI::CreateRenderTargetView(const FRHIRenderTargetViewDesc& InDesc)
{
    if (!InDesc.Texture)
    {
        VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
        return nullptr;
    }

    FVulkanRenderTargetViewRHIRef NewRenderTargetView = new FVulkanRenderTargetViewRHI(GetDevice(), InDesc.Texture);
    if (!NewRenderTargetView->Initialize(InDesc))
    {
        return nullptr;
    }

    return NewRenderTargetView.ReleaseOwnership();
}

FRHIDepthStencilView* FVulkanRHI::CreateDepthStencilView(const FRHIDepthStencilViewDesc& InDesc)
{
    if (!InDesc.Texture)
    {
        VULKAN_ERROR_CRITICAL("Texture cannot be nullptr");
        return nullptr;
    }

    FVulkanDepthStencilViewRHIRef NewDepthStencilView = new FVulkanDepthStencilViewRHI(GetDevice(), InDesc.Texture);
    if (!NewDepthStencilView->Initialize(InDesc))
    {
        return nullptr;
    }

    return NewDepthStencilView.ReleaseOwnership();
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
        const VkMemoryHeap& MemoryHeap  = memoryProperties.memoryHeaps[Index];
        const bool          bDeviceLocal = (MemoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0;

        if ((MemoryType == EVideoMemoryType::Local) == bDeviceLocal)
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

bool FVulkanRHI::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
{
    FVulkanQueryRHI* VulkanQuery = FVulkanRHI::ResourceCast(Query);
    if (!VulkanQuery)
    {
        return false;
    }

    if (Mode == EQueryResultMode::Wait)
    {
        if (!VulkanQuery->SyncFence)
        {
            return false;
        }

        VulkanQuery->SyncFence->Wait(UINT64_MAX);
    }

    OutResult = *VulkanQuery->QueryResult;
    return true;
}

bool FVulkanRHI::GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode)
{
    FVulkanQueryRHI* VulkanQuery = FVulkanRHI::ResourceCast(Query);
    if (!VulkanQuery)
    {
        return false;
    }

    if (Mode == EQueryResultMode::Wait)
    {
        if (!VulkanQuery->SyncFence)
        {
            return false;
        }
        
        VulkanQuery->SyncFence->Wait(UINT64_MAX);
    }

    OutResult = *reinterpret_cast<const FRHIPipelineStatistics*>(VulkanQuery->QueryResult);
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

void* FVulkanRHI::GetRHINativeAdapter()
{
    CHECK(PhysicalDevice != nullptr);
    return reinterpret_cast<void*>(PhysicalDevice->GetVkPhysicalDevice());
}

void* FVulkanRHI::GetRHINativeDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetVkDevice());
}

void* FVulkanRHI::GetRHINativeDirectCommandQueue()
{
    CHECK(GraphicsQueue != nullptr);
    return reinterpret_cast<void*>(GraphicsQueue->GetVkQueue());
}

void* FVulkanRHI::GetRHINativeComputeCommandQueue()
{
    // TODO: Finish
    CHECK(false);
    return nullptr;
}

void* FVulkanRHI::GetRHINativeCopyCommandQueue()
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

void FVulkanRHI::TickCoreProgression()
{
    GraphicsQueue->ProcessCommandQueue();
    Device->GetMemoryManager().CleanUpAllocators();
}

void FVulkanRHI::FlushCompletedSubmissions()
{
    GraphicsQueue->ProcessCommandQueue();
}

void FVulkanRHI::FlushDeletionQueue(FVulkanCommands* Commands)
{
    CHECK(Commands != nullptr);
    if (Commands->IsEmpty())
    {
        return;
    }

    TScopedLock Lock(DeferredObjectsCS);
    Commands->DeferredObjects = Move(DeferredObjects);
}

VkPipelineStageFlags2 FVulkanRHI::ResourceStateToPipelineStageFlags(EResourceAccess ResourceState)
{
    VkPipelineStageFlags2 AllShaderBits =
        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    VkPipelineStageFlags2 AllNonPixelShaderBits =
        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    if (GVulkanSupportsGeometryShader)
    {
        AllShaderBits         |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;
        AllNonPixelShaderBits |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT;
    }
    
    if (GVulkanSupportsTessellation)
    {
        AllShaderBits         |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;
        AllNonPixelShaderBits |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT;
    }

    switch (ResourceState)
    {
        case EResourceAccess::Common:                 return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        case EResourceAccess::CopyDest:               return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        case EResourceAccess::CopySource:             return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        case EResourceAccess::DepthRead:              return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        case EResourceAccess::DepthWrite:             return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        case EResourceAccess::IndexBuffer:            return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
        case EResourceAccess::VertexBuffer:           return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
        case EResourceAccess::NonPixelShaderResource: return AllNonPixelShaderBits;
        case EResourceAccess::PixelShaderResource:    return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        case EResourceAccess::Present:                return VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        case EResourceAccess::RenderTarget:           return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        case EResourceAccess::ResolveDest:            return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        case EResourceAccess::ResolveSource:          return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        case EResourceAccess::ShadingRateSource:      return GVulkanSupportsFragmentShadingRate ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR : VK_PIPELINE_STAGE_2_NONE;
        case EResourceAccess::UnorderedAccess:        return AllShaderBits;
        case EResourceAccess::ConstantBuffer:         return AllShaderBits;
        case EResourceAccess::GenericRead:            return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        default:                                      return VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    }
}

VkAccessFlags2 FVulkanRHI::ResourceStateToAccessFlags(EResourceAccess ResourceState)
{
    switch (ResourceState)
    {
        case EResourceAccess::Common:                 return VK_ACCESS_2_NONE;
        case EResourceAccess::CopyDest:               return VK_ACCESS_2_TRANSFER_WRITE_BIT;
        case EResourceAccess::CopySource:             return VK_ACCESS_2_TRANSFER_READ_BIT;
        case EResourceAccess::DepthRead:              return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        case EResourceAccess::DepthWrite:             return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case EResourceAccess::IndexBuffer:            return VK_ACCESS_2_INDEX_READ_BIT;
        case EResourceAccess::VertexBuffer:           return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
        case EResourceAccess::NonPixelShaderResource: return VK_ACCESS_2_SHADER_READ_BIT;
        case EResourceAccess::PixelShaderResource:    return VK_ACCESS_2_SHADER_READ_BIT;
        case EResourceAccess::Present:                return VK_ACCESS_2_MEMORY_READ_BIT;
        case EResourceAccess::RenderTarget:           return VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        case EResourceAccess::ResolveDest:            return VK_ACCESS_2_TRANSFER_WRITE_BIT;
        case EResourceAccess::ResolveSource:          return VK_ACCESS_2_TRANSFER_READ_BIT;
        case EResourceAccess::ShadingRateSource:      return GVulkanSupportsFragmentShadingRate ? VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR : VK_ACCESS_2_NONE;
        case EResourceAccess::UnorderedAccess:        return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
        case EResourceAccess::ConstantBuffer:         return VK_ACCESS_2_UNIFORM_READ_BIT;
        case EResourceAccess::GenericRead:            return VK_ACCESS_2_MEMORY_READ_BIT;
        default:                                      return VK_ACCESS_2_NONE;
    }
}

VkImageLayout FVulkanRHI::ResourceStateToImageLayout(EResourceAccess ResourceState)
{
    switch (ResourceState)
    {
        case EResourceAccess::Common:                 return VK_IMAGE_LAYOUT_GENERAL;
        case EResourceAccess::CopyDest:               return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case EResourceAccess::CopySource:             return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case EResourceAccess::DepthRead:              return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case EResourceAccess::DepthWrite:             return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case EResourceAccess::IndexBuffer:            return VK_IMAGE_LAYOUT_UNDEFINED;
        case EResourceAccess::VertexBuffer:           return VK_IMAGE_LAYOUT_UNDEFINED;
        case EResourceAccess::NonPixelShaderResource: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case EResourceAccess::PixelShaderResource:    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case EResourceAccess::Present:                return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case EResourceAccess::RenderTarget:           return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case EResourceAccess::ResolveDest:            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case EResourceAccess::ResolveSource:          return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case EResourceAccess::ShadingRateSource:      return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        case EResourceAccess::UnorderedAccess:        return VK_IMAGE_LAYOUT_GENERAL;
        case EResourceAccess::ConstantBuffer:         return VK_IMAGE_LAYOUT_UNDEFINED;
        case EResourceAccess::GenericRead:            return VK_IMAGE_LAYOUT_UNDEFINED;
        default:                                      return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}
