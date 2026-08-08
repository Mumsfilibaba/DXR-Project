#include "Core/Misc/ConsoleManager.h"
#include "Core/Tasks/Tasks.h"
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
#include "VulkanRHI/RayTracing/VulkanRayTracing.h"
#include "VulkanRHI/VulkanStats.h"
#include "RHI/RHIStats.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/Platform/VulkanPlatform.h"

IMPLEMENT_ENGINE_MODULE(FVulkanModuleRHI, VulkanRHI);

static TAutoConsoleVariable<int32> CVarMaxDefragMovesPerFrame(
    "VulkanRHI.MaxDefragMovesPerFrame",
    "Maximum number of texture defragmentation moves per frame (0 to disable)",
    4);

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
static TAutoConsoleVariable<bool> CVarVulkanUseDynamicRendering(
    "VulkanRHI.UseDynamicRendering",
    "Use VK_KHR_dynamic_rendering instead of VkRenderPass and VkFramebuffer",
    true,
    EConsoleVariableFlags::Default);
#endif

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

FRHIDevice* FVulkanModuleRHI::CreateDevice()
{
    TUniquePtr<FVulkanDeviceRHI> NewRHI = MakeUniquePtr<FVulkanDeviceRHI>();
    if (!NewRHI->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewRHI.Release();
    }
}

ERHIType FVulkanDeviceRHI::GetRHIType() const
{
    return ERHIType::Vulkan;
}

FVulkanDeviceRHI* FVulkanDeviceRHI::VulkanDeviceRHI = nullptr;

FVulkanTextureRHI* FVulkanDeviceRHI::ResourceCast(FRHITexture* Texture)
{
    if (Texture)
    {
        return static_cast<FVulkanTextureBase*>(Texture)->GetTextureInterface();
    }

    return nullptr;
}

FVulkanRenderTargetViewRHI* FVulkanDeviceRHI::ResourceCast(FRHIRenderTargetView* RenderTargetView)
{
    if (RenderTargetView)
    {
        return static_cast<FVulkanRenderTargetViewBase*>(RenderTargetView)->GetRenderTargetViewInterface();
    }

    return nullptr;
}

FVulkanUnorderedAccessViewRHI* FVulkanDeviceRHI::ResourceCast(FRHIUnorderedAccessView* UnorderedAccessView)
{
    if (UnorderedAccessView)
    {
        return static_cast<FVulkanUnorderedAccessViewBase*>(UnorderedAccessView)->GetUnorderedAccessViewInterface();
    }

    return nullptr;
}

FVulkanAccelerationStructure* FVulkanDeviceRHI::ResourceCast(FRHIRayTracingAccelerationStructure* AccelerationStructure)
{
    if (!AccelerationStructure)
    {
        return nullptr;
    }

    switch (AccelerationStructure->GetAccelerationStructureType())
    {
        case ERayTracingAccelerationStructureType::Geometry:
            return static_cast<FVulkanGeometryAccelerationStructureRHI*>(AccelerationStructure);

        case ERayTracingAccelerationStructureType::Scene:
            return static_cast<FVulkanSceneAccelerationStructureRHI*>(AccelerationStructure);

        case ERayTracingAccelerationStructureType::Cluster:
            return static_cast<FVulkanClusterAccelerationStructureRHI*>(AccelerationStructure);
        
        case ERayTracingAccelerationStructureType::PartitionedScene:
            return static_cast<FVulkanPartitionedSceneAccelerationStructureRHI*>(AccelerationStructure);

        default:
            return nullptr;
    }
}

const FVulkanAccelerationStructure* FVulkanDeviceRHI::ResourceCast(const FRHIRayTracingAccelerationStructure* AccelerationStructure)
{
    if (!AccelerationStructure)
    {
        return nullptr;
    }

    switch (AccelerationStructure->GetAccelerationStructureType())
    {
        case ERayTracingAccelerationStructureType::Geometry:
            return static_cast<const FVulkanGeometryAccelerationStructureRHI*>(AccelerationStructure);

        case ERayTracingAccelerationStructureType::Scene:
            return static_cast<const FVulkanSceneAccelerationStructureRHI*>(AccelerationStructure);

        case ERayTracingAccelerationStructureType::Cluster:
            return static_cast<const FVulkanClusterAccelerationStructureRHI*>(AccelerationStructure);

        case ERayTracingAccelerationStructureType::PartitionedScene:
            return static_cast<const FVulkanPartitionedSceneAccelerationStructureRHI*>(AccelerationStructure);

        default:
            return nullptr;
    }
}

const FVulkanTextureRHI* FVulkanDeviceRHI::ResourceCast(const FRHITexture* Texture)
{
    if (Texture)
    {
        return static_cast<const FVulkanTextureBase*>(Texture)->GetTextureInterface();
    }

    return nullptr;
}

const FVulkanRenderTargetViewRHI* FVulkanDeviceRHI::ResourceCast(const FRHIRenderTargetView* RenderTargetView)
{
    if (RenderTargetView)
    {
        return static_cast<const FVulkanRenderTargetViewBase*>(RenderTargetView)->GetRenderTargetViewInterface();
    }

    return nullptr;
}

const FVulkanUnorderedAccessViewRHI* FVulkanDeviceRHI::ResourceCast(const FRHIUnorderedAccessView* UnorderedAccessView)
{
    if (UnorderedAccessView)
    {
        return static_cast<const FVulkanUnorderedAccessViewBase*>(UnorderedAccessView)->GetUnorderedAccessViewInterface();
    }

    return nullptr;
}

FVulkanDeviceRHI::FVulkanDeviceRHI()
    : FRHIDevice()
    , Instance()
#if VK_EXT_debug_utils
    , DebugMessenger(VK_NULL_HANDLE)
#endif
    , PhysicalDevice(nullptr)
    , Device(nullptr)
    , GraphicsCommandContext(nullptr)
    , FrameNumber(0)
    , NumOpenCommandBuffers(0)
#if VULKAN_ENABLE_CRASH_MARKERS
    , CrashMarkers(nullptr)
#endif
{
    if (!VulkanDeviceRHI)
    {
        VulkanDeviceRHI = this;
    }
}

FVulkanDeviceRHI::~FVulkanDeviceRHI()
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

    // Idle all queues and reclaim completed submissions.
    if (Device)
    {
        Device->WaitForGPU();
    }

    // Flush before submitting since some objects needs the CommandContext
    FlushDeletionQueue();

    if (GraphicsCommandContext)
    {
        GraphicsCommandContext->GetCommandQueue().ReleaseCommandContext(GraphicsCommandContext);
        GraphicsCommandContext = nullptr;
    }

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

    SAFE_DELETE(Device);
    SAFE_DELETE(PhysicalDevice);

#if VK_EXT_debug_utils
    VulkanDestroyDebugMessenger(Instance.GetVkInstance(), DebugMessenger);
#endif
    
    Instance.Release();

    if (VulkanDeviceRHI == this)
    {
        VulkanDeviceRHI = nullptr;
    }
}

bool FVulkanDeviceRHI::Initialize()
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
    DeviceCreateInfo.OptionalFeatures.Features10.depthBounds             = VK_TRUE;

    // Shader Model 6.9 promotes native 16-bit and 64-bit integer shader ops from optional to required.
    DeviceCreateInfo.OptionalFeatures.Features10.shaderInt16 = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features10.shaderInt64 = VK_TRUE;
    
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

    // SM 6.2 defines the 16-bit scalar types as usable with memory operations, not just arithmetic.
    DeviceCreateInfo.OptionalFeatures.Features11.storageBuffer16BitAccess           = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features11.uniformAndStorageBuffer16BitAccess = VK_TRUE;

    // Vulkan 1.2 Required 
    DeviceCreateInfo.RequiredFeatures.Features12.hostQueryReset      = VK_TRUE; 
    DeviceCreateInfo.RequiredFeatures.Features12.bufferDeviceAddress = VK_TRUE; 
    DeviceCreateInfo.RequiredFeatures.Features12.shaderOutputLayer   = VK_TRUE; 
    DeviceCreateInfo.RequiredFeatures.Features12.timelineSemaphore   = VK_TRUE; 
    
    // Vulkan 1.2 Optional
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorIndexing = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.drawIndirectCount  = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.shaderFloat16      = VK_TRUE;

    // SM 6.6 adds 64-bit integer atomics on buffers and groupshared memory.
    DeviceCreateInfo.OptionalFeatures.Features12.shaderBufferInt64Atomics = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.shaderSharedInt64Atomics = VK_TRUE;

    DeviceCreateInfo.OptionalFeatures.Features12.runtimeDescriptorArray                             = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorBindingPartiallyBound                    = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorBindingVariableDescriptorCount           = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorBindingSampledImageUpdateAfterBind       = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorBindingStorageImageUpdateAfterBind       = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorBindingUniformBufferUpdateAfterBind      = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorBindingStorageBufferUpdateAfterBind      = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.descriptorBindingUpdateUnusedWhilePending          = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.scalarBlockLayout                                  = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.shaderSampledImageArrayNonUniformIndexing          = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.shaderStorageImageArrayNonUniformIndexing          = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.shaderStorageBufferArrayNonUniformIndexing         = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.shaderUniformBufferArrayNonUniformIndexing         = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.shaderUniformTexelBufferArrayNonUniformIndexing    = VK_TRUE;
    DeviceCreateInfo.OptionalFeatures.Features12.shaderStorageTexelBufferArrayNonUniformIndexing    = VK_TRUE;

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

    // Initialize Queues (owned by FVulkanDevice)
    if (!Device->CreateGraphicsQueue())
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize VulkanQueue [Graphics]");
        return false;
    }

#if VULKAN_ENABLE_CRASH_MARKERS
    if (Device->IsCrashMarkerExtensionsEnabled() && CVarVulkanEnableCrashMarkers.GetValue())
    {
        CrashMarkers = new FVulkanCrashMarkers(Device);
        if (!CrashMarkers->Initialize(*Device->GetGraphicsQueue()))
        {
            delete CrashMarkers;
            CrashMarkers = nullptr;
        }
    }
#endif

    GraphicsCommandContext = Device->GetGraphicsQueue()->ObtainCommandContext();
    if (!GraphicsCommandContext)
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

void FVulkanDeviceRHI::BeginFrame()
{
    // Update timestamp period, this is necessary on MoltenVK in order to get correct measurements
    {
        VkPhysicalDeviceProperties Properties;
        vkGetPhysicalDeviceProperties(PhysicalDevice->GetVkPhysicalDevice(), &Properties);
        VulkanDeviceLimits::TimestampPeriod = Properties.limits.timestampPeriod;
    }

    FVulkanQueue* GraphicsQueue = Device->GetGraphicsQueue();
    GraphicsQueue->ProcessCommandQueue();

    FrameNumber++;

    GraphicsQueue->PruneCommandContexts(FrameNumber);

#if VULKAN_USE_DESCRIPTOR_CACHE
    Device->GetDescriptorSetCache().EvictStaleDescriptorSets(0);
#else
    Device->GetDescriptorPoolManager().EvictUnusedPools();
#endif

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    if (!GVulkanUseDynamicRendering)
    {
        Device->GetRenderPassCache().EvictStaleFramebuffers();
    }
#endif

    GraphicsQueue->ForEachLiveCommandContext([](FVulkanCommandContext& Context)
    {
        Context.GetContextState().EvictStaleDescriptorStates();
    });

    Device->GetMemoryManager().CleanUpAllocators();

    const int32 MaxDefragMoves = CVarMaxDefragMovesPerFrame.GetValue();
    if (MaxDefragMoves > 0)
    {
        Device->GetMemoryManager().DefragmentAllocations(GraphicsCommandContext, MaxDefragMoves);
    }
}

void FVulkanDeviceRHI::EndFrame()
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

FRHITexture* FVulkanDeviceRHI::CreateTexture(const FRHITextureDesc& InTextureDesc, ERHIResourceState InInitialState, const IRHITextureData* InInitialData)
{
    FVulkanBorrowedCommandContext UploadContext(*GetDevice()->GetGraphicsQueue());

    FVulkanTextureRHIRef NewTexture = new FVulkanTextureRHI(GetDevice(), InTextureDesc);
    if (!NewTexture->Initialize(UploadContext.Get(), InInitialState, InInitialData))
    {
        return nullptr;
    }

#if VULKAN_ENABLE_STATS
    {
        const int64 AllocatedSize = static_cast<int64>(NewTexture->GetMemoryLocation().GetSize());
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

    FlushCompletedSubmissions();
    return NewTexture.ReleaseOwnership();
}

FRHIBuffer* FVulkanDeviceRHI::CreateBuffer(const FRHIBufferDesc& InBufferDesc, ERHIResourceState InInitialState, const void* InInitialData)
{
    FVulkanBorrowedCommandContext UploadContext(*GetDevice()->GetGraphicsQueue());

    FVulkanBufferRHIRef NewBuffer = new FVulkanBufferRHI(GetDevice(), InBufferDesc);
    if (!NewBuffer->Initialize(UploadContext.Get(), InInitialState, InInitialData))
    {
        return nullptr;
    }

#if VULKAN_ENABLE_STATS
    {
        const int64 AllocatedSize = static_cast<int64>(NewBuffer->GetMemoryLocation().GetSize());
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

    FlushCompletedSubmissions();
    return NewBuffer.ReleaseOwnership();
}

FRHISamplerState* FVulkanDeviceRHI::CreateSamplerState(const FRHISamplerStateDesc& InSamplerDesc)
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

FRHISwapChain* FVulkanDeviceRHI::CreateSwapChain(const FRHISwapChainDesc& InSwapChainDesc)
{
    CHECK(InSwapChainDesc.WindowHandle != nullptr);

    if (!Tasks::IsInRHIThread())
    {
        FRHISwapChain* NewSwapChain = nullptr;
        Tasks::LaunchOnRHIThread("VulkanCreateSwapChain", [this, &NewSwapChain, &InSwapChainDesc]()
        {
            NewSwapChain = CreateSwapChain(InSwapChainDesc);
        }).Wait();

        return NewSwapChain;
    }

    FVulkanSwapChainRHIRef NewSwapChain = new FVulkanSwapChainRHI(Device, GraphicsCommandContext, InSwapChainDesc);
    if (!NewSwapChain->Initialize())
    {
        return nullptr;
    }

    Device->EnsurePresentQueue();
    return NewSwapChain.ReleaseOwnership();
}

FRHIQuery* FVulkanDeviceRHI::CreateQuery(EQueryType InQueryType)
{
    return new FVulkanQueryRHI(Device, InQueryType);
}

FRHIFence* FVulkanDeviceRHI::CreateFence()
{
    FVulkanFenceRHI* NewFence = new FVulkanFenceRHI(Device);
    if (!NewFence->Initialize())
    {
        delete NewFence;
        return nullptr;
    }

    return NewFence;
}

FRHISceneAccelerationStructure* FVulkanDeviceRHI::CreateSceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc)
{
    FRHISceneAccelerationStructureBuildDesc BuildDesc;
    BuildDesc.Instances    = InSceneDesc.Instances.Data();
    BuildDesc.NumInstances = InSceneDesc.Instances.Size();
    BuildDesc.bUpdate      = false;

    FVulkanSceneAccelerationStructureRHIRef NewScene = new FVulkanSceneAccelerationStructureRHI(GetDevice(), InSceneDesc);

    {
        FVulkanScopedCommandContext BuildContext(*GetDevice()->GetGraphicsQueue());
        if (!NewScene->Build(*BuildContext, BuildDesc))
        {
            DEBUG_BREAK();
            NewScene.Reset();
        }
    }

    FlushCompletedSubmissions();
    return NewScene.ReleaseOwnership();
}

FRHIGeometryAccelerationStructure* FVulkanDeviceRHI::CreateGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
{
    FRHIGeometryAccelerationStructureBuildDesc BuildDesc;
    BuildDesc.VertexBuffer = InGeometryDesc.VertexBuffer;
    BuildDesc.NumVertices  = InGeometryDesc.NumVertices;
    BuildDesc.IndexBuffer  = InGeometryDesc.IndexBuffer;
    BuildDesc.NumIndices   = InGeometryDesc.NumIndices;
    BuildDesc.IndexFormat  = InGeometryDesc.IndexFormat;
    BuildDesc.bUpdate      = false;

    FVulkanGeometryAccelerationStructureRHIRef NewGeometry = new FVulkanGeometryAccelerationStructureRHI(GetDevice(), InGeometryDesc);

    {
        FVulkanScopedCommandContext BuildContext(*GetDevice()->GetGraphicsQueue());
        if (!NewGeometry->Build(*BuildContext, BuildDesc))
        {
            DEBUG_BREAK();
            NewGeometry.Reset();
        }
    }

    FlushCompletedSubmissions();
    return NewGeometry.ReleaseOwnership();
}

FRHIOpacityMicromap* FVulkanDeviceRHI::CreateOpacityMicromap(const FRHIOpacityMicromapDesc& InDesc)
{
#if VK_EXT_opacity_micromap
    if (GVulkanSupportsOpacityMicromap)
    {
        return new FVulkanOpacityMicromap(GetDevice(), InDesc);
    }
#endif

    UNREFERENCED_VARIABLE(InDesc);
    return nullptr;
}

FRHIClusterAccelerationStructure* FVulkanDeviceRHI::CreateClusterAccelerationStructure(const FRHIClusterAccelerationStructureDesc& InDesc)
{
#if VK_NV_cluster_acceleration_structure
    if (GVulkanSupportsClustersAndPTLAS)
    {
        TSharedRef<FVulkanClusterAccelerationStructureRHI> NewCluster = new FVulkanClusterAccelerationStructureRHI(GetDevice(), InDesc);
        if (NewCluster->Initialize())
        {
            return NewCluster.ReleaseOwnership();
        }
    }
#endif

    UNREFERENCED_VARIABLE(InDesc);
    return nullptr;
}

FRHIClusterTemplate* FVulkanDeviceRHI::CreateClusterTemplate(const FRHIClusterTemplateDesc& InDesc)
{
#if VK_NV_cluster_acceleration_structure
    if (GVulkanSupportsClustersAndPTLAS)
    {
        TSharedRef<FVulkanClusterTemplateRHI> NewTemplate = new FVulkanClusterTemplateRHI(GetDevice(), InDesc);
        if (NewTemplate->Initialize())
        {
            return NewTemplate.ReleaseOwnership();
        }
    }
#endif

    UNREFERENCED_VARIABLE(InDesc);
    return nullptr;
}

FRHIPartitionedSceneAccelerationStructure* FVulkanDeviceRHI::CreatePartitionedSceneAccelerationStructure(const FRHIRayTracingAccelerationStructurePartitionedSceneInputs& InInputs)
{
#if VK_NV_partitioned_acceleration_structure
    if (GVulkanSupportsClustersAndPTLAS)
    {
        TSharedRef<FVulkanPartitionedSceneAccelerationStructureRHI> NewScene = new FVulkanPartitionedSceneAccelerationStructureRHI(GetDevice(), InInputs);
        if (NewScene->Initialize())
        {
            return NewScene.ReleaseOwnership();
        }
    }
#endif

    UNREFERENCED_VARIABLE(InInputs);
    return nullptr;
}

void FVulkanDeviceRHI::GetRayTracingAccelerationStructureOperationPrebuildInfo(const FRHIRayTracingAccelerationStructureOperationInputs& InInputs, FRHIRayTracingAccelerationStructurePrebuildInfo& OutInfo)
{
    OutInfo = FRHIRayTracingAccelerationStructurePrebuildInfo();

#if VK_NV_cluster_acceleration_structure && VK_NV_partitioned_acceleration_structure
    if (!GVulkanSupportsClustersAndPTLAS)
    {
        return;
    }

    VkAccelerationStructureBuildSizesInfoKHR SizesInfo = {};
    SizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    if (InInputs.OperationType == ERayTracingAccelerationStructureOperationType::PartitionedSceneAccelerationStructure)
    {
        if (vkGetPartitionedAccelerationStructuresBuildSizesNV)
        {
            VkPartitionedAccelerationStructureInstancesInputNV InstancesInput = {};
            InstancesInput.sType                             = VK_STRUCTURE_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCES_INPUT_NV;
            InstancesInput.instanceCount                     = InInputs.MaxArgumentCount;
            InstancesInput.maxInstancePerPartitionCount      = InInputs.MaxArgumentCount;
            InstancesInput.partitionCount                    = 1;
            InstancesInput.maxInstanceInGlobalPartitionCount = InInputs.MaxArgumentCount;
            vkGetPartitionedAccelerationStructuresBuildSizesNV(GetDevice()->GetVkDevice(), &InstancesInput, &SizesInfo);
        }
    }
    else if (vkGetClusterAccelerationStructureBuildSizesNV)
    {
        FVulkanClusterInputScratch Scratch = {};
        Scratch.TriangleClusters.sType                         = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_TRIANGLE_CLUSTER_INPUT_NV;
        Scratch.TriangleClusters.vertexFormat                  = VK_FORMAT_R32G32B32_SFLOAT;
        Scratch.TriangleClusters.maxGeometryIndexValue         = InInputs.ClusterLimits.MaxGeometryIndex;
        Scratch.TriangleClusters.maxClusterUniqueGeometryCount = 1;
        Scratch.TriangleClusters.maxClusterTriangleCount       = InInputs.ClusterLimits.MaxTrianglesPerCluster;
        Scratch.TriangleClusters.maxClusterVertexCount         = InInputs.ClusterLimits.MaxVerticesPerCluster;
        Scratch.TriangleClusters.maxTotalTriangleCount         = InInputs.ClusterLimits.MaxTrianglesPerCluster * InInputs.ClusterLimits.MaxClusterCount;
        Scratch.TriangleClusters.maxTotalVertexCount           = InInputs.ClusterLimits.MaxVerticesPerCluster * InInputs.ClusterLimits.MaxClusterCount;
        Scratch.TriangleClusters.minPositionTruncateBitCount   = 0;

        Scratch.ClustersBottomLevel.sType                                   = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_CLUSTERS_BOTTOM_LEVEL_INPUT_NV;
        Scratch.ClustersBottomLevel.maxTotalClusterCount                    = InInputs.ClusterLimits.MaxClusterCount;
        Scratch.ClustersBottomLevel.maxClusterCountPerAccelerationStructure = InInputs.ClusterLimits.MaxClusterCount;

        Scratch.MoveObjects.sType         = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_MOVE_OBJECTS_INPUT_NV;
        Scratch.MoveObjects.type          = VK_CLUSTER_ACCELERATION_STRUCTURE_TYPE_TRIANGLE_CLUSTER_NV;
        Scratch.MoveObjects.noMoveOverlap = VK_FALSE;
        Scratch.MoveObjects.maxMovedBytes = 0;

        VkClusterAccelerationStructureInputInfoNV InputInfo = {};
        InputInfo.sType                         = VK_STRUCTURE_TYPE_CLUSTER_ACCELERATION_STRUCTURE_INPUT_INFO_NV;
        InputInfo.maxAccelerationStructureCount = InInputs.MaxArgumentCount;
        InputInfo.flags                         = ConvertAccelerationStructureBuildFlags(EAccelerationStructureBuildFlags::None);

        switch (InInputs.OperationType)
        {
            case ERayTracingAccelerationStructureOperationType::BuildClusterTemplatesFromTriangles:
                InputInfo.opType                    = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_TRIANGLE_CLUSTER_TEMPLATE_NV;
                InputInfo.opInput.pTriangleClusters = &Scratch.TriangleClusters;
                break;
            case ERayTracingAccelerationStructureOperationType::InstantiateClusterTemplates:
                InputInfo.opType                    = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_INSTANTIATE_TRIANGLE_CLUSTER_NV;
                InputInfo.opInput.pTriangleClusters = &Scratch.TriangleClusters;
                break;
            case ERayTracingAccelerationStructureOperationType::BuildGeometryAccelerationStructureFromClusters:
                InputInfo.opType                       = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_CLUSTERS_BOTTOM_LEVEL_NV;
                InputInfo.opInput.pClustersBottomLevel = &Scratch.ClustersBottomLevel;
                break;
            case ERayTracingAccelerationStructureOperationType::MoveClusterObjects:
                InputInfo.opType               = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_MOVE_OBJECTS_NV;
                InputInfo.opInput.pMoveObjects = &Scratch.MoveObjects;
                break;
            default:
                InputInfo.opType                    = VK_CLUSTER_ACCELERATION_STRUCTURE_OP_TYPE_BUILD_TRIANGLE_CLUSTER_NV;
                InputInfo.opInput.pTriangleClusters = &Scratch.TriangleClusters;
                break;
        }

        vkGetClusterAccelerationStructureBuildSizesNV(GetDevice()->GetVkDevice(), &InputInfo, &SizesInfo);
    }

    OutInfo.ResultSizeInBytes        = SizesInfo.accelerationStructureSize;
    OutInfo.ScratchSizeInBytes       = SizesInfo.buildScratchSize;
    OutInfo.UpdateScratchSizeInBytes = SizesInfo.updateScratchSize;
#else
    UNREFERENCED_VARIABLE(InInputs);
#endif
}

FRHIShaderResourceView* FVulkanDeviceRHI::CreateShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
{
    if (!InResource)
    {
        VULKAN_ERROR_CRITICAL("CreateShaderResourceView: Resource cannot be nullptr");
        return nullptr;
    }

    if (InDesc.IsBufferSRV())
    {
        VULKAN_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Buffer,
            "CreateShaderResourceView: buffer view requires an FRHIBuffer resource");
    }
    else if (InDesc.IsTextureSRV())
    {
        VULKAN_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
            "CreateShaderResourceView: texture view requires an FRHITexture resource");
        CHECK(IsViewDimensionCompatible(static_cast<FRHITexture*>(InResource)->GetDesc().Dimension, InDesc.ViewDimension));
    }
    else if (InDesc.IsAccelerationStructureSRV())
    {
        VULKAN_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::SceneAccelerationStructure,
            "CreateShaderResourceView: AccelerationStructure view requires an FRHISceneAccelerationStructure resource");
    }
    else
    {
        return nullptr;
    }

    FVulkanShaderResourceViewRHIRef NewShaderResourceView = new FVulkanShaderResourceViewRHI(GetDevice(), InResource, InDesc);
    if (!NewShaderResourceView->Initialize(InResource, InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewShaderResourceView.ReleaseOwnership();
    }
}

FRHIUnorderedAccessView* FVulkanDeviceRHI::CreateUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
{
    if (!InResource)
    {
        VULKAN_ERROR_CRITICAL("CreateUnorderedAccessView: Resource cannot be nullptr");
        return nullptr;
    }

    if (InDesc.IsBufferUAV())
    {
        VULKAN_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Buffer,
            "CreateUnorderedAccessView: buffer view requires an FRHIBuffer resource");
    }
    else if (InDesc.IsTextureUAV())
    {
        VULKAN_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
            "CreateUnorderedAccessView: texture view requires an FRHITexture resource");
        CHECK(IsViewDimensionCompatible(static_cast<FRHITexture*>(InResource)->GetDesc().Dimension, InDesc.ViewDimension));
    }
    else
    {
        return nullptr;
    }

    FVulkanUnorderedAccessViewRHIRef NewUnorderedAccessView = new FVulkanUnorderedAccessViewRHI(GetDevice(), InResource, InDesc);
    if (!NewUnorderedAccessView->Initialize(InResource, InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewUnorderedAccessView.ReleaseOwnership();
    }
}

FRHIUnorderedAccessView* FVulkanDeviceRHI::CreateSamplerFeedbackUnorderedAccessView(FRHITexture* /* InFeedbackTexture */, FRHITexture* /* InTargetedTexture */)
{
    VULKAN_ERROR("CreateSamplerFeedbackUnorderedAccessView: sampler feedback is not supported by the Vulkan backend");
    return nullptr;
}

FRHIRenderTargetView* FVulkanDeviceRHI::CreateRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
{
    if (!InResource)
    {
        VULKAN_ERROR_CRITICAL("CreateRenderTargetView: Resource cannot be nullptr");
        return nullptr;
    }

    VULKAN_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
        "CreateRenderTargetView: requires an FRHITexture resource");
    CHECK(IsViewDimensionCompatible(static_cast<FRHITexture*>(InResource)->GetDesc().Dimension, InDesc.ViewDimension));

    FVulkanRenderTargetViewRHIRef NewRenderTargetView = new FVulkanRenderTargetViewRHI(GetDevice(), InResource, InDesc);
    if (!NewRenderTargetView->Initialize(static_cast<FRHITexture*>(InResource), InDesc))
    {
        return nullptr;
    }

    return NewRenderTargetView.ReleaseOwnership();
}

FRHIDepthStencilView* FVulkanDeviceRHI::CreateDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc)
{
    if (!InResource)
    {
        VULKAN_ERROR_CRITICAL("CreateDepthStencilView: Resource cannot be nullptr");
        return nullptr;
    }

    VULKAN_ERROR_COND(InResource->GetResourceType() == ERHIResourceType::Texture,
        "CreateDepthStencilView: requires an FRHITexture resource");
    CHECK(IsViewDimensionCompatible(static_cast<FRHITexture*>(InResource)->GetDesc().Dimension, InDesc.ViewDimension));

    FVulkanDepthStencilViewRHIRef NewDepthStencilView = new FVulkanDepthStencilViewRHI(GetDevice(), InResource, InDesc);
    if (!NewDepthStencilView->Initialize(static_cast<FRHITexture*>(InResource), InDesc))
    {
        return nullptr;
    }

    return NewDepthStencilView.ReleaseOwnership();
}

FRHIComputeShader* FVulkanDeviceRHI::CreateComputeShader(const TArray<uint8>& ShaderCode)
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

FRHIVertexShader* FVulkanDeviceRHI::CreateVertexShader(const TArray<uint8>& ShaderCode)
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

FRHIHullShader* FVulkanDeviceRHI::CreateHullShader(const TArray<uint8>& ShaderCode)
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

FRHIDomainShader* FVulkanDeviceRHI::CreateDomainShader(const TArray<uint8>& ShaderCode)
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

FRHIGeometryShader* FVulkanDeviceRHI::CreateGeometryShader(const TArray<uint8>& ShaderCode)
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

FRHIMeshShader* FVulkanDeviceRHI::CreateMeshShader(const TArray<uint8>& ShaderCode)
{
    if (!GVulkanSupportsMeshShaders)
    {
        return nullptr;
    }

    FVulkanMeshShaderRHIRef NewShader = new FVulkanMeshShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIAmplificationShader* FVulkanDeviceRHI::CreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    if (!GVulkanSupportsTaskShaders)
    {
        VULKAN_WARNING("CreateAmplificationShader called but task (amplification) shaders are not supported on this device");
        return nullptr;
    }

    FVulkanAmplificationShaderRHIRef NewShader = new FVulkanAmplificationShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIPixelShader* FVulkanDeviceRHI::CreatePixelShader(const TArray<uint8>& ShaderCode)
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

FRHIRayGenShader* FVulkanDeviceRHI::CreateRayGenShader(const TArray<uint8>& ShaderCode)
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

FRHIRayAnyHitShader* FVulkanDeviceRHI::CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
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

FRHIRayClosestHitShader* FVulkanDeviceRHI::CreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
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

FRHIRayMissShader* FVulkanDeviceRHI::CreateRayMissShader(const TArray<uint8>& ShaderCode)
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

FRHIRayIntersectionShader* FVulkanDeviceRHI::CreateRayIntersectionShader(const TArray<uint8>& ShaderCode)
{
    FVulkanRayIntersectionShaderRHIRef NewShader = new FVulkanRayIntersectionShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIRayCallableShader* FVulkanDeviceRHI::CreateRayCallableShader(const TArray<uint8>& ShaderCode)
{
    FVulkanRayCallableShaderRHIRef NewShader = new FVulkanRayCallableShaderRHI(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FVulkanDeviceRHI::CreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
{
    return new FVulkanDepthStencilStateRHI(InDesc);
}

FRHIRasterizerState* FVulkanDeviceRHI::CreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)
{
    return new FVulkanRasterizerStateRHI(GetDevice(), InDesc);
}

FRHIBlendState* FVulkanDeviceRHI::CreateBlendState(const FRHIBlendStateDesc& InDesc)
{
    return new FVulkanBlendStateRHI(InDesc);
}

FRHIInputLayout* FVulkanDeviceRHI::CreateInputLayout(const TArray<FRHIInputElementDesc>& InInputElements)
{
    return new FVulkanInputLayoutRHI(InInputElements);
}

FRHIGraphicsPipelineState* FVulkanDeviceRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)
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

FRHIComputePipelineState* FVulkanDeviceRHI::CreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)
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

FRHIMeshletPipelineState* FVulkanDeviceRHI::CreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& InDesc)
{
    if (!GVulkanSupportsMeshShaders)
    {
        return nullptr;
    }

    FVulkanMeshletPipelineStateRHIRef NewPipeline = new FVulkanMeshletPipelineStateRHI(GetDevice());
    if (!NewPipeline->Initialize(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewPipeline.ReleaseOwnership();
    }
}

FRHIRayTracingPipelineState* FVulkanDeviceRHI::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc)
{
    FVulkanRayTracingPipelineStateRHIRef NewPipeline = new FVulkanRayTracingPipelineStateRHI(GetDevice());
    if (!NewPipeline->Initialize(InDesc))
    {
        DEBUG_BREAK();
        return nullptr;
    }

    return NewPipeline.ReleaseOwnership();
}

FRHIShaderBindingTable* FVulkanDeviceRHI::CreateShaderBindingTable(const FRHIShaderBindingTableDesc& InDesc)
{
    FVulkanShaderBindingTableRef NewTable = new FVulkanShaderBindingTable(GetDevice(), InDesc);
    if (!NewTable->Initialize())
    {
        return nullptr;
    }

    return NewTable.ReleaseOwnership();
}

FRHIRayTracingShaderIdentifier FVulkanDeviceRHI::GetRayTracingShaderIdentifier(FRHIRayTracingPipelineState* InPipeline, const String& InExportName)
{
    FRHIRayTracingShaderIdentifier Identifier;
    if (FVulkanRayTracingPipelineStateRHI* VulkanPipeline = FVulkanDeviceRHI::ResourceCast(InPipeline))
    {
        if (const uint8* GroupHandle = VulkanPipeline->GetShaderGroupHandle(InExportName))
        {
            const uint32 HandleSize = Math::Min<uint32>(VulkanPipeline->GetShaderGroupHandleSize(), FRHIRayTracingShaderIdentifier::MAX_SIZE_IN_BYTES);
            Memory::Memcpy(Identifier.Data, GroupHandle, HandleSize);
            Identifier.SizeInBytes = HandleSize;
        }
    }

    return Identifier;
}

bool FVulkanDeviceRHI::IsAccelerationStructureSerializationHeaderValid(const FRHIAccelerationStructureSerializationHeader& InHeader)
{
    if (InHeader.RHIType != ERHIType::Vulkan)
    {
        return false;
    }

#if VK_KHR_acceleration_structure
    if (!vkGetDeviceAccelerationStructureCompatibilityKHR)
    {
        return false;
    }

    static_assert(FRHIAccelerationStructureSerializationHeader::DRIVER_MATCHING_IDENTIFIER_SIZE >= (2 * VK_UUID_SIZE),
        "Serialization header driver-matching identifier is too small for Vulkan");

    VkAccelerationStructureVersionInfoKHR VersionInfo = {};
    VersionInfo.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_VERSION_INFO_KHR;
    VersionInfo.pVersionData = InHeader.DriverMatchingIdentifier;

    VkAccelerationStructureCompatibilityKHR Compatibility = VK_ACCELERATION_STRUCTURE_COMPATIBILITY_INCOMPATIBLE_KHR;
    vkGetDeviceAccelerationStructureCompatibilityKHR(GetDevice()->GetVkDevice(), &VersionInfo, &Compatibility);

    return Compatibility == VK_ACCELERATION_STRUCTURE_COMPATIBILITY_COMPATIBLE_KHR;
#else
    return false;
#endif
}

bool FVulkanDeviceRHI::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryInfo) const 
{
    if (!Device->IsExtensionEnabled(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
    {
        VULKAN_WARNING("[FVulkanDeviceRHI] VK_EXT_memory_budget is required to query video-memory information");
        return false;
    }

    VkPhysicalDeviceMemoryProperties2 MemoryProperties2;
    Memory::Memzero(&MemoryProperties2);
    MemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

    VkPhysicalDeviceMemoryBudgetPropertiesEXT MemoryBudgetProperties;
    Memory::Memzero(&MemoryBudgetProperties);
    MemoryBudgetProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

    MemoryProperties2.pNext = &MemoryBudgetProperties;

    vkGetPhysicalDeviceMemoryProperties2(PhysicalDevice->GetVkPhysicalDevice(), &MemoryProperties2);

    OutMemoryInfo.MemoryType   = MemoryType;
    OutMemoryInfo.MemoryUsage  = 0;
    OutMemoryInfo.MemoryBudget = 0;

    const VkPhysicalDeviceMemoryProperties& memoryProperties = MemoryProperties2.memoryProperties;
    for (uint32 Index = 0; Index < memoryProperties.memoryHeapCount; Index++)
    {
        const VkMemoryHeap& MemoryHeap = memoryProperties.memoryHeaps[Index];
        
        const bool bDeviceLocal = (MemoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0;
        if ((MemoryType == EVideoMemoryType::Local) == bDeviceLocal)
        {
            OutMemoryInfo.MemoryBudget += MemoryBudgetProperties.heapBudget[Index];
            OutMemoryInfo.MemoryUsage  += MemoryBudgetProperties.heapUsage[Index];
        }
    }

    return true;
}

bool FVulkanDeviceRHI::QueryUAVFormatSupport(EFormat Format) const
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

bool FVulkanDeviceRHI::QuerySupportedSampleCounts(EFormat Format, uint32& OutSampleCounts) const
{
    OutSampleCounts = 0;

    const VkFormat VulkanFormat = ConvertFormat(Format);
    if (VulkanFormat == VK_FORMAT_UNDEFINED)
    {
        return false;
    }

    const VkImageAspectFlags AspectFlags     = GetImageAspectFlagsFromFormat(VulkanFormat);
    const bool               bIsDepthStencil = (AspectFlags & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0;

    VkImageFormatProperties ImageFormatProperties = {};
    const VkResult Result = vkGetPhysicalDeviceImageFormatProperties(
        PhysicalDevice->GetVkPhysicalDevice(),
        VulkanFormat,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_TILING_OPTIMAL,
        bIsDepthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        0,
        &ImageFormatProperties);

    if (Result != VK_SUCCESS)
    {
        return false;
    }

    OutSampleCounts = ImageFormatProperties.sampleCounts;
    return OutSampleCounts != 0;
}

bool FVulkanDeviceRHI::GetQueryResult(FRHIQuery* Query, uint64& OutResult, EQueryResultMode Mode)
{
    FVulkanQueryRHI* VulkanQuery = FVulkanDeviceRHI::ResourceCast(Query);
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

bool FVulkanDeviceRHI::GetPipelineStatisticsResult(FRHIQuery* Query, FRHIPipelineStatistics& OutResult, EQueryResultMode Mode)
{
    FVulkanQueryRHI* VulkanQuery = FVulkanDeviceRHI::ResourceCast(Query);
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

String FVulkanDeviceRHI::GetAdapterName() const
{
    if (!PhysicalDevice)
    {
        VULKAN_ERROR_CRITICAL("PhysicalDevice is not initialized properly");
        return String();
    }

    VkPhysicalDeviceProperties DeviceProperties = PhysicalDevice->GetProperties();
    return String(DeviceProperties.deviceName);
}

IRHICommandContext* FVulkanDeviceRHI::ObtainCommandContext()
{
    CHECK(GraphicsCommandContext != nullptr);
    return GraphicsCommandContext;
}

void* FVulkanDeviceRHI::GetRHINativeAdapter()
{
    CHECK(PhysicalDevice != nullptr);
    return reinterpret_cast<void*>(PhysicalDevice->GetVkPhysicalDevice());
}

void* FVulkanDeviceRHI::GetRHINativeDevice()
{
    CHECK(Device != nullptr);
    return reinterpret_cast<void*>(Device->GetVkDevice());
}

void* FVulkanDeviceRHI::GetRHINativeDirectCommandQueue()
{
    CHECK(Device != nullptr && Device->GetGraphicsQueue() != nullptr);
    return reinterpret_cast<void*>(Device->GetGraphicsQueue()->GetVkQueue());
}

void* FVulkanDeviceRHI::GetRHINativeComputeCommandQueue()
{
    // TODO: Finish
    CHECK(false);
    return nullptr;
}

void* FVulkanDeviceRHI::GetRHINativeCopyCommandQueue()
{
    // TODO: Finish
    CHECK(false);
    return nullptr;
}

void FVulkanDeviceRHI::EnqueueResourceDeletion(FRHIResource* Resource)
{
    if (Resource)
    {
        DeferDeletion(Resource);
    }
}

void FVulkanDeviceRHI::FlushCompletedSubmissions()
{
    Device->GetGraphicsQueue()->ProcessCommandQueue();
}

void FVulkanDeviceRHI::NotifyCommandBufferOpened()
{
    TScopedLock Lock(DeferredObjectsCS);
    NumOpenCommandBuffers++;
}

void FVulkanDeviceRHI::NotifyCommandBufferRetired(FVulkanCommands* Commands)
{
    TArray<FVulkanDeferredObject> ObjectsWithoutBatch;

    {
        TScopedLock Lock(DeferredObjectsCS);

        CHECK(NumOpenCommandBuffers > 0);
        NumOpenCommandBuffers--;

        // ---------------------------------------------------------------------------------------
        // A command buffer holds references to everything it recorded until it is submitted, so
        // these objects may only be destroyed by a batch the GPU is guaranteed to reach last. That
        // is only true of this batch once no other buffer is outstanding: while one is, destroying
        // now would pull a resource out from under a buffer that has not even been ended yet.
        // Leave the queue for a later retire.
        // ---------------------------------------------------------------------------------------

        if (NumOpenCommandBuffers > 0)
        {
            return;
        }

        // ---------------------------------------------------------------------------------------
        // A discarded buffer passes no batch, and an empty batch is never submitted, so in both
        // cases nothing would ever run PostExecute to process the objects. Hand them to the queue
        // instead of leaving them parked here waiting for a retire that may not come until
        // shutdown; the queue destroys them once it has no submission left in flight.
        // ---------------------------------------------------------------------------------------

        if (Commands && !Commands->IsEmpty())
        {
            Commands->DeferredObjects = Move(DeferredObjects);
        }
        else
        {
            ObjectsWithoutBatch = Move(DeferredObjects);
        }
    }

    if (!ObjectsWithoutBatch.IsEmpty())
    {
        Device->GetGraphicsQueue()->RetireDeferredObjects(Move(ObjectsWithoutBatch));
    }
}

VkPipelineStageFlags2KHR FVulkanDeviceRHI::ResourceStateToPipelineStageFlags(ERHIResourceState ResourceState)
{
    VkPipelineStageFlags2KHR AllShaderBits =
        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR |
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR |
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;

    VkPipelineStageFlags2KHR AllNonPixelShaderBits =
        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR |
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;

    if (GVulkanSupportsGeometryShader)
    {
        AllShaderBits         |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT_KHR;
        AllNonPixelShaderBits |= VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT_KHR;
    }
    
    if (GVulkanSupportsTessellation)
    {
        AllShaderBits         |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT_KHR | VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT_KHR;
        AllNonPixelShaderBits |= VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT_KHR | VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT_KHR;
    }

    if (ResourceState == ERHIResourceState::Common || IsEnumFlagSet(ResourceState, ERHIResourceState::GenericRead))
    {
        return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
    }

    VkPipelineStageFlags2KHR Stages = VK_PIPELINE_STAGE_2_NONE_KHR;
    if (IsEnumFlagSet(ResourceState, ERHIResourceState::CopyDest))
    {
        Stages |= VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::CopySource))
    {
        Stages |= VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::DepthRead))
    {
        Stages |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::DepthWrite))
    {
        Stages |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::IndexBuffer))
    {
        Stages |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::VertexBuffer))
    {
        Stages |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::NonPixelShaderResource))
    {
        Stages |= AllNonPixelShaderBits;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::PixelShaderResource))
    {
        Stages |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::Present))
    {
        Stages |= VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::RenderTarget))
    {
        Stages |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ResolveDest))
    {
        Stages |= VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ResolveSource))
    {
        Stages |= VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ShadingRateSource))
    {
        Stages |= GVulkanSupportsFragmentShadingRate ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR : VK_PIPELINE_STAGE_2_NONE_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::UnorderedAccess))
    {
        Stages |= AllShaderBits;
    }
    
    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ConstantBuffer))
    {
        Stages |= AllShaderBits;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::IndirectArgument))
    {
        Stages |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT_KHR;
    }

#if VK_EXT_transform_feedback
    if (IsEnumFlagSet(ResourceState, ERHIResourceState::StreamOutput))
    {
        Stages |= GVulkanSupportsTransformFeedback ? VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT : VK_PIPELINE_STAGE_2_NONE_KHR;
    }
#endif

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::RayTracingAccelerationStructure))
    {
        if (GVulkanSupportsAccelerationStructures)
        {
            Stages |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        }

        if (GVulkanSupportsRayTracingPipeline)
        {
            Stages |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        }
    }

    return Stages != VK_PIPELINE_STAGE_2_NONE_KHR ? Stages : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR;
}

VkAccessFlags2KHR FVulkanDeviceRHI::ResourceStateToAccessFlags(ERHIResourceState ResourceState)
{
    if (ResourceState == ERHIResourceState::Common)
    {
        return VK_ACCESS_2_NONE_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::GenericRead))
    {
        return VK_ACCESS_2_MEMORY_READ_BIT_KHR;
    }

    VkAccessFlags2KHR Access = VK_ACCESS_2_NONE_KHR;
    if (IsEnumFlagSet(ResourceState, ERHIResourceState::CopyDest))
    {
        Access |= VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::CopySource))
    {
        Access |= VK_ACCESS_2_TRANSFER_READ_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::DepthRead))
    {
        Access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::DepthWrite))
    {
        Access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::IndexBuffer))
    {
        Access |= VK_ACCESS_2_INDEX_READ_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::VertexBuffer))
    {
        Access |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::NonPixelShaderResource))
    {
        Access |= VK_ACCESS_2_SHADER_READ_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::PixelShaderResource))
    {
        Access |= VK_ACCESS_2_SHADER_READ_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::Present))
    {
        Access |= VK_ACCESS_2_MEMORY_READ_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::RenderTarget))
    {
        Access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ResolveDest))
    {
        Access |= VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ResolveSource))
    {
        Access |= VK_ACCESS_2_TRANSFER_READ_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ShadingRateSource))
    {
        Access |= GVulkanSupportsFragmentShadingRate ? VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR : VK_ACCESS_2_NONE_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::UnorderedAccess))
    {
        Access |= VK_ACCESS_2_SHADER_READ_BIT_KHR | VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
    }
    
    if (IsEnumFlagSet(ResourceState, ERHIResourceState::ConstantBuffer))
    {
        Access |= VK_ACCESS_2_UNIFORM_READ_BIT_KHR;
    }

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::IndirectArgument))
    {
        Access |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT_KHR;
    }

#if VK_EXT_transform_feedback
    if (IsEnumFlagSet(ResourceState, ERHIResourceState::StreamOutput))
    {
        Access |= GVulkanSupportsTransformFeedback ? VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT : VK_ACCESS_2_NONE_KHR;
    }
#endif

    if (IsEnumFlagSet(ResourceState, ERHIResourceState::RayTracingAccelerationStructure) && GVulkanSupportsAccelerationStructures)
    {
        Access |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    }

    return Access;
}

VkImageLayout FVulkanDeviceRHI::ResourceStateToImageLayout(ERHIResourceState ResourceState)
{
    if (IsEnumFlagSet(ResourceState, ERHIResourceState::PixelShaderResource) ||
        IsEnumFlagSet(ResourceState, ERHIResourceState::NonPixelShaderResource))
    {
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    switch (ResourceState)
    {
        case ERHIResourceState::Common:                 return VK_IMAGE_LAYOUT_GENERAL;
        case ERHIResourceState::CopyDest:               return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case ERHIResourceState::CopySource:             return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case ERHIResourceState::DepthRead:              return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case ERHIResourceState::DepthWrite:             return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case ERHIResourceState::IndexBuffer:            return VK_IMAGE_LAYOUT_UNDEFINED;
        case ERHIResourceState::VertexBuffer:           return VK_IMAGE_LAYOUT_UNDEFINED;
        case ERHIResourceState::NonPixelShaderResource: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ERHIResourceState::PixelShaderResource:    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ERHIResourceState::Present:                return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case ERHIResourceState::RenderTarget:           return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ERHIResourceState::ResolveDest:            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case ERHIResourceState::ResolveSource:          return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case ERHIResourceState::ShadingRateSource:      return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        case ERHIResourceState::UnorderedAccess:        return VK_IMAGE_LAYOUT_GENERAL;
        case ERHIResourceState::ConstantBuffer:         return VK_IMAGE_LAYOUT_UNDEFINED;
        case ERHIResourceState::GenericRead:            return VK_IMAGE_LAYOUT_UNDEFINED;
        default:                                      return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}
