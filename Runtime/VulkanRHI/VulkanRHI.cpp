#include "Core/Misc/ConsoleManager.h"
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

FVulkanRHI* FVulkanRHI::GVulkanRHI = nullptr;

FVulkanRHI::FVulkanRHI()
    : FRHI(ERHIType::Vulkan)
    , Instance()
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
        ProcessPendingCommands();
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

    if (GVulkanRHI == this)
    {
        GVulkanRHI = nullptr;
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
    // Vulkan 1.0 Optional
    DeviceCreateInfo.OptionalFeatures.geometryShader                       = VK_TRUE;
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

FRHITexture* FVulkanRHI::CreateTexture(const FRHITextureInfo& InTextureInfo, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FVulkanTextureRef NewTexture = new FVulkanTexture(GetDevice(), InTextureInfo);
    if (!NewTexture->Initialize(GraphicsCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewTexture.ReleaseOwnership();
    }
}

FRHIBuffer* FVulkanRHI::CreateBuffer(const FRHIBufferInfo& InBufferInfo, EResourceAccess InInitialState, const void* InInitialData)
{
    FVulkanBufferRef NewBuffer = new FVulkanBuffer(GetDevice(), InBufferInfo);
    if (!NewBuffer->Initialize(GraphicsCommandContext, InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FVulkanRHI::CreateSamplerState(const FRHISamplerStateInfo& InSamplerInfo)
{
    TScopedLock Lock(SamplerStateMapCS);

    TSharedRef<FVulkanSamplerState> Result;

    // Check if there already is an existing sampler state with this description
    if (TSharedRef<FVulkanSamplerState>* ExistingSamplerState = SamplerStateMap.Find(InSamplerInfo))
    {
        Result = *ExistingSamplerState;
    }
    else
    {
        Result = new FVulkanSamplerState(GetDevice(), InSamplerInfo);
        if (!Result->Initialize())
        {
            return nullptr;
        }
        else
        {
            SamplerStateMap.Add(InSamplerInfo, Result);
        }
    }

    return Result.ReleaseOwnership();
}

FRHISwapChain* FVulkanRHI::CreateSwapChain(const FRHISwapChainInfo& InSwapChainInfo)
{
    CHECK(InSwapChainInfo.WindowHandle != nullptr);

    FVulkanSwapChainRef NewSwapChain = new FVulkanSwapChain(Device, InSwapChainInfo);
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
    return new FVulkanQuery(Device, InQueryType);
}

FRHIGpuFence* FVulkanRHI::CreateFence()
{
    FVulkanGpuFence* NewFence = new FVulkanGpuFence(Device);
    if (!NewFence->Initialize())
    {
        delete NewFence;
        return nullptr;
    }

    return NewFence;
}

FRHIRayTracingScene* FVulkanRHI::CreateRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(InSceneInfo);
    return nullptr;
}

FRHIRayTracingGeometry* FVulkanRHI::CreateRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo)
{
    FRayTracingGeometryBuildInfo BuildInfo;
    BuildInfo.VertexBuffer = InGeometryInfo.VertexBuffer;
    BuildInfo.NumVertices  = InGeometryInfo.NumVertices;
    BuildInfo.IndexBuffer  = InGeometryInfo.IndexBuffer;
    BuildInfo.NumIndices   = InGeometryInfo.NumIndices;
    BuildInfo.IndexFormat  = InGeometryInfo.IndexFormat;
    BuildInfo.bUpdate      = false;

    GraphicsCommandContext->StartContext();

    FVulkanRayTracingGeometryRef NewGeometry = new FVulkanRayTracingGeometry(GetDevice(), InGeometryInfo);
    if (!NewGeometry->Build(*GraphicsCommandContext, BuildInfo))
    {
        DEBUG_BREAK();
        NewGeometry.Reset();
    }

    GraphicsCommandContext->FinishContext();
    return NewGeometry.ReleaseOwnership();
}

FRHIShaderResourceView* FVulkanRHI::CreateShaderResourceView(const FRHIShaderResourceViewInfo& InInfo)
{
    FRHIResource* Resource = nullptr;
    if (InInfo.IsBufferSRV())
    {
        Resource = InInfo.BufferSRV.Buffer;
    }
    else if (InInfo.IsTextureSRV())
    {
        Resource = InInfo.TextureSRV.Texture;
    }
    else
    {
        return nullptr;
    }

	CHECK(Resource != nullptr);

    FVulkanShaderResourceViewRef NewShaderResourceView = new FVulkanShaderResourceView(GetDevice(), Resource);
    if (!NewShaderResourceView->InitializeSRV(InInfo))
    {
        return nullptr;
    }
    else
    {
        return NewShaderResourceView.ReleaseOwnership();
    }
}

FRHIUnorderedAccessView* FVulkanRHI::CreateUnorderedAccessView(const FRHIUnorderedAccessViewInfo& InInfo)
{
	FRHIResource* Resource = nullptr;
	if (InInfo.IsBufferUAV())
	{
		Resource = InInfo.BufferUAV.Buffer;
	}
	else if (InInfo.IsTextureUAV())
	{
		Resource = InInfo.TextureUAV.Texture;
	}
	else
	{
		return nullptr;
	}

	CHECK(Resource != nullptr);

    FVulkanUnorderedAccessViewRef NewUnorderedAccessView = new FVulkanUnorderedAccessView(GetDevice(), Resource);
    if (!NewUnorderedAccessView->InitializeUAV(InInfo))
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
    FVulkanComputeShaderRef NewShader = new FVulkanComputeShader(GetDevice());
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
    FVulkanVertexShaderRef NewShader = new FVulkanVertexShader(GetDevice());
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
    FVulkanHullShaderRef NewShader = new FVulkanHullShader(GetDevice());
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
    FVulkanDomainShaderRef NewShader = new FVulkanDomainShader(GetDevice());
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
    FVulkanGeometryShaderRef NewShader = new FVulkanGeometryShader(GetDevice());
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
    FVulkanPixelShaderRef NewShader = new FVulkanPixelShader(GetDevice());
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
    FVulkanRayGenShaderRef NewShader = new FVulkanRayGenShader(GetDevice());
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
    FVulkanRayAnyHitShaderRef NewShader = new FVulkanRayAnyHitShader(GetDevice());
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
    FVulkanRayClosestHitShaderRef NewShader = new FVulkanRayClosestHitShader(GetDevice());
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
    FVulkanRayMissShaderRef NewShader = new FVulkanRayMissShader(GetDevice());
    if (!NewShader->Initialize(ShaderCode))
    {
        return nullptr;
    }
    else
    {
        return NewShader.ReleaseOwnership();
    }
}

FRHIDepthStencilState* FVulkanRHI::CreateDepthStencilState(const FRHIDepthStencilStateInfo& InInfo)
{
    return new FVulkanDepthStencilState(InInfo);
}

FRHIRasterizerState* FVulkanRHI::CreateRasterizerState(const FRHIRasterizerStateInfo& InInfo)
{
    return new FVulkanRasterizerState(GetDevice(), InInfo);
}

FRHIBlendState* FVulkanRHI::CreateBlendState(const FRHIBlendStateInfo& InInfo)
{
    return new FVulkanBlendState(InInfo);
}

FRHIInputLayout* FVulkanRHI::CreateInputLayout(const TArray<FRHIInputElementInfo>& InInputElements)
{
    return new FVulkanInputLayout(InInputElements);
}

FRHIGraphicsPipelineState* FVulkanRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInfo& InInfo)
{
    FVulkanGraphicsPipelineStateRef NewPipeline = new FVulkanGraphicsPipelineState(GetDevice());
    if (!NewPipeline->Initialize(InInfo))
    {
        return nullptr;
    }
    else
    {
        return NewPipeline.ReleaseOwnership();
    }
}

FRHIComputePipelineState* FVulkanRHI::CreateComputePipelineState(const FRHIComputePipelineStateInfo& InInfo)
{
    FVulkanComputePipelineStateRef NewPipeline = new FVulkanComputePipelineState(GetDevice());
    if (!NewPipeline->Initialize(InInfo))
    {
        return nullptr;
    }
    else
    {
        return NewPipeline.ReleaseOwnership();
    }
}

FRHIRayTracingPipelineState* FVulkanRHI::CreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& /*InInitializer*/ )
{
    return new FVulkanRayTracingPipelineState();
}

bool FVulkanRHI::QueryVideoMemoryInfo(EVideoMemoryType MemoryType, FRHIVideoMemoryInfo& OutMemoryStats) const 
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

    OutMemoryStats.MemoryType   = MemoryType;
    OutMemoryStats.MemoryUsage  = 0;
    OutMemoryStats.MemoryBudget = 0;

    const VkPhysicalDeviceMemoryProperties& memoryProperties = MemoryProperties2.memoryProperties;
    for (uint32 Index = 0; Index < memoryProperties.memoryHeapCount; Index++)
    {
        if (MemoryType == EVideoMemoryType::Local)
        {
            const VkMemoryHeap& MemoryHeap = memoryProperties.memoryHeaps[Index];
            if (MemoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                OutMemoryStats.MemoryBudget += MemoryBudgetProperties.heapBudget[Index];
                OutMemoryStats.MemoryUsage  += MemoryBudgetProperties.heapUsage[Index];
            }
        }
        else
        {
            OutMemoryStats.MemoryBudget += MemoryBudgetProperties.heapBudget[Index];
            OutMemoryStats.MemoryUsage  += MemoryBudgetProperties.heapUsage[Index];
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
    FVulkanQuery* VulkanQuery = static_cast<FVulkanQuery*>(Query);
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

void FVulkanRHI::ProcessPendingCommands()
{
    bool bProcess = true;
    while (bProcess)
    {
        FVulkanCommands* Commands = nullptr;
        if (PendingSubmissions.Peek(Commands))
        {
            CHECK(Commands != nullptr);
            if (!Commands->IsExecutionFinished())
            {
                bProcess = false;
                break;
            }
            else
            {
                // If we are finished we remove the item from the queue
                PendingSubmissions.Dequeue();
                Commands->Finish();
            }
        }
        else
        {
            bProcess = false;
        }
    }
}

void FVulkanRHI::SubmitCommands(FVulkanCommands* Commands, bool bFlushDeletionQueue)
{
    CHECK(Commands != nullptr);

    if (!Commands->IsEmpty())
    {
        TScopedLock SubmitLock(SubmissionCS);

        if (bFlushDeletionQueue)
        {
            TScopedLock Lock(DeletionQueueCS);
            Commands->DeletionQueue = Move(DeletionQueue);
        }

        Commands->PreExecute();
        Commands->Execute();

        PendingSubmissions.Enqueue(Commands);
    }
}
