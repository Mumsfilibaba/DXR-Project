#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanQuery.h"
#include "VulkanRHI/VulkanShader.h"
#include "VulkanRHI/VulkanPipelineState.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanSamplerState.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanDeviceLimits.h"
#include "VulkanRHI/VulkanRayTracing.h"
#include "VulkanRHI/Platform/PlatformVulkan.h"

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
    InstanceDesc.RequiredExtensionNames = FPlatformVulkan::GetRequiredInstanceExtensions();
    InstanceDesc.RequiredLayerNames     = FPlatformVulkan::GetRequiredInstanceLayers();
    InstanceDesc.OptionalExtensionNames = FPlatformVulkan::GetOptionalInstanceExtentions();
    
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
    
    // We always want to add debug utils in order to make markers work
#if VK_EXT_debug_utils
    InstanceDesc.RequiredExtensionNames.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    if (!Instance.Initialize(InstanceDesc))
    {
        VULKAN_ERROR("Failed to initialize VulkanInstance");
        return false;
    }
    
    // Load functions that requires an instance here
    if (!LoadInstanceFunctions(GetInstance()))
    {
        return false;
    }

    FVulkanPhysicalDeviceCreateInfo AdapterCreateInfo;
    AdapterCreateInfo.RequiredExtensionNames = FPlatformVulkan::GetRequiredDeviceExtensions();
    AdapterCreateInfo.OptionalExtensionNames = FPlatformVulkan::GetOptionalDeviceExtentions();
    
    // Enable required features (These are necessary to run)
    AdapterCreateInfo.RequiredFeatures.samplerAnisotropy         = VK_TRUE;
    AdapterCreateInfo.RequiredFeatures.shaderImageGatherExtended = VK_TRUE;
    AdapterCreateInfo.RequiredFeatures.imageCubeArray            = VK_TRUE;
    AdapterCreateInfo.RequiredFeatures.depthBiasClamp            = VK_TRUE;
    AdapterCreateInfo.RequiredFeatures11.shaderDrawParameters    = VK_TRUE;
    AdapterCreateInfo.RequiredFeatures12.hostQueryReset          = VK_TRUE;

    PhysicalDevice = new FVulkanPhysicalDevice(GetInstance());
    if (!PhysicalDevice->Initialize(AdapterCreateInfo))
    {
        VULKAN_ERROR("Failed to initialize VulkanPhyscicalDevice");
        return false;
    }

    FVulkanDeviceCreateInfo DeviceCreateInfo;
    DeviceCreateInfo.RequiredExtensionNames = AdapterCreateInfo.RequiredExtensionNames;
    DeviceCreateInfo.OptionalExtensionNames = AdapterCreateInfo.OptionalExtensionNames;
    DeviceCreateInfo.RequiredFeatures       = AdapterCreateInfo.RequiredFeatures;
    DeviceCreateInfo.RequiredFeatures11     = AdapterCreateInfo.RequiredFeatures11;
    DeviceCreateInfo.RequiredFeatures12     = AdapterCreateInfo.RequiredFeatures12;

    // Enable optional features for Vulkan 1.0
    const VkPhysicalDeviceFeatures& PhysicalDeviceFeatures = PhysicalDevice->GetFeatures();
    
    // Enable geometryShader if the device supports them
    if (PhysicalDeviceFeatures.geometryShader)
    {
        DeviceCreateInfo.RequiredFeatures.geometryShader = VK_TRUE;
    }
    
    // Enable multiDrawIndirect if the device supports them
    if (PhysicalDeviceFeatures.multiDrawIndirect)
    {
        DeviceCreateInfo.RequiredFeatures.multiDrawIndirect = VK_TRUE;
    }
    
    // Enable optional features for Vulkan 1.2
    const VkPhysicalDeviceVulkan12Features& PhysicalDeviceFeatures12 = PhysicalDevice->GetFeaturesVulkan12();

    // Enable shaderOutputLayer if the device supports them
    if (PhysicalDeviceFeatures12.shaderOutputLayer)
    {
        DeviceCreateInfo.RequiredFeatures12.shaderOutputLayer = VK_TRUE;
    }

    Device = new FVulkanDevice(GetInstance(), GetPhysicalDevice());
    if (!Device->Initialize(DeviceCreateInfo))
    {
        VULKAN_ERROR("Failed to initialize VulkanDevice");
        return false;
    }
    
    // Load functions that requires a device here (Order is important)
    if (!LoadDeviceFunctions(Device))
    {
        return false;
    }

    // Initialize parts of the device that require device functions to be present
    if (!Device->PostLoaderInitalize())
    {
        VULKAN_ERROR("Failed to PostLoaderInitalize failed to VulkanDevice");
        return false;
    }

    // Initialize Queues
    GraphicsQueue = new FVulkanQueue(Device, EVulkanCommandQueueType::Graphics);
    if (!GraphicsQueue->Initialize())
    {
        VULKAN_ERROR("Failed to initialize VulkanQueue [Graphics]");
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
        VULKAN_ERROR("Failed to initialize VulkanCommandContext");
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
        FVulkanDeviceLimits::TimestampPeriod = Properties.limits.timestampPeriod;
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

FRHIShaderResourceView* FVulkanRHI::CreateShaderResourceView(const FRHITextureSRVInfo& InInfo)
{
    FVulkanTexture* VulkanTexture = FVulkanTexture::ResourceCast(InInfo.Texture);
    CHECK(VulkanTexture != nullptr);

    FVulkanShaderResourceViewRef NewShaderResourceView = new FVulkanShaderResourceView(GetDevice(), VulkanTexture);
    if (!NewShaderResourceView->InitializeTextureSRV(InInfo))
    {
        return nullptr;
    }
    else
    {
        return NewShaderResourceView.ReleaseOwnership();
    }
}

FRHIShaderResourceView* FVulkanRHI::CreateShaderResourceView(const FRHIBufferSRVInfo& InInfo)
{
    FVulkanBuffer* VulkanBuffer = FVulkanBuffer::ResourceCast(InInfo.Buffer);
    CHECK(VulkanBuffer != nullptr);

    FVulkanShaderResourceViewRef NewShaderResourceView = new FVulkanShaderResourceView(GetDevice(), VulkanBuffer);
    if (!NewShaderResourceView->InitializeBufferSRV(InInfo))
    {
        return nullptr;
    }
    else
    {
        return NewShaderResourceView.ReleaseOwnership();
    }
}

FRHIUnorderedAccessView* FVulkanRHI::CreateUnorderedAccessView(const FRHITextureUAVInfo& InInfo)
{
    FVulkanTexture* VulkanTexture = FVulkanTexture::ResourceCast(InInfo.Texture);
    CHECK(VulkanTexture != nullptr);

    FVulkanUnorderedAccessViewRef NewUnorderedAccessView = new FVulkanUnorderedAccessView(GetDevice(), VulkanTexture);
    if (!NewUnorderedAccessView->InitializeTextureUAV(InInfo))
    {
        return nullptr;
    }
    else
    {
        return NewUnorderedAccessView.ReleaseOwnership();
    }
}

FRHIUnorderedAccessView* FVulkanRHI::CreateUnorderedAccessView(const FRHIBufferUAVInfo& InInfo)
{
    FVulkanBuffer* VulkanBuffer = FVulkanBuffer::ResourceCast(InInfo.Buffer);
    CHECK(VulkanBuffer != nullptr);

    FVulkanUnorderedAccessViewRef NewUnorderedAccessView = new FVulkanUnorderedAccessView(GetDevice(), VulkanBuffer);
    if (!NewUnorderedAccessView->InitializeBufferUAV(InInfo))
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

FRHIDepthStencilState* FVulkanRHI::CreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
{
    return new FVulkanDepthStencilState(InInitializer);
}

FRHIRasterizerState* FVulkanRHI::CreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
{
    return new FVulkanRasterizerState(GetDevice(), InInitializer);
}

FRHIBlendState* FVulkanRHI::CreateBlendState(const FRHIBlendStateInitializer& InInitializer)
{
    return new FVulkanBlendState(InInitializer);
}

FRHIVertexLayout* FVulkanRHI::CreateVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList)
{
    return new FVulkanVertexLayout(InInitializerList);
}

FRHIGraphicsPipelineState* FVulkanRHI::CreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer)
{
    FVulkanGraphicsPipelineStateRef NewPipeline = new FVulkanGraphicsPipelineState(GetDevice());
    if (!NewPipeline->Initialize(InInitializer))
    {
        return nullptr;
    }
    else
    {
        return NewPipeline.ReleaseOwnership();
    }
}

FRHIComputePipelineState* FVulkanRHI::CreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer)
{
    FVulkanComputePipelineStateRef NewPipeline = new FVulkanComputePipelineState(GetDevice());
    if (!NewPipeline->Initialize(InInitializer))
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

    // Query memory properties
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
        VULKAN_ERROR("PhysicalDevice is not initialized properly");
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
        FVulkanCommandPayload* CommandPayload = nullptr;
        if (PendingSubmissions.Peek(CommandPayload))
        {
            CHECK(CommandPayload != nullptr);
            if (!CommandPayload->IsExecutionFinished())
            {
                bProcess = false;
                break;
            }
            else
            {
                // If we are finished we remove the item from the queue
                PendingSubmissions.Dequeue();
                CommandPayload->Finish();
            }
        }
        else
        {
            bProcess = false;
        }
    }
}

void FVulkanRHI::SubmitCommands(FVulkanCommandPayload* CommandPayload, bool bFlushDeletionQueue)
{
    CHECK(CommandPayload != nullptr);

    if (!CommandPayload->IsEmpty())
    {
        if (bFlushDeletionQueue)
        {
            TScopedLock Lock(DeletionQueueCS);
            CommandPayload->DeletionQueue = Move(DeletionQueue);
        }

        CommandPayload->Submit();
        
        PendingSubmissions.Enqueue(CommandPayload);
    }
}
