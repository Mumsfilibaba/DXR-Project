#include "VulkanRHI.h"
#include "VulkanLoader.h"
#include "VulkanQuery.h"
#include "VulkanShader.h"
#include "VulkanPipelineState.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanResourceViews.h"
#include "VulkanSamplerState.h"
#include "VulkanViewport.h"
#include "VulkanDeviceLimits.h"
#include "VulkanRayTracing.h"
#include "Platform/PlatformVulkan.h"
#include "Core/Misc/ConsoleManager.h"

IMPLEMENT_ENGINE_MODULE(FVulkanRHIModule, VulkanRHI);

FRHI* FVulkanRHIModule::CreateRHI()
{
    return new FVulkanRHI();
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
    // Delete the Default Context before we flush the submission queue..
    SAFE_DELETE(GraphicsCommandContext);

    // Then delete all samplers
    {
        TScopedLock Lock(SamplerStateMapCS);
        SamplerStateMap.Clear();
    }

    //.. since the context will put objects into the Deferred Deletion Queue
    while (!PendingSubmissions.IsEmpty())
    {
        ProcessPendingCommands();
    }
    
    // Delete all remaining resources
    TArray<FVulkanDeletionQueue::FDeferredResource> Resources;
    DeletionQueue.Dequeue(Resources);
    
    for (FVulkanDeletionQueue::FDeferredResource& Object : Resources)
    {
        Object.Release();
    }
    
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
    
    // Turn on the debuglayer
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

    FVulkanPhysicalDeviceCreateInfo AdapterDesc;
    AdapterDesc.RequiredExtensionNames                     = FPlatformVulkan::GetRequiredDeviceExtensions();
    AdapterDesc.OptionalExtensionNames                     = FPlatformVulkan::GetOptionalDeviceExtentions();
    AdapterDesc.RequiredFeatures.samplerAnisotropy         = VK_TRUE;
    AdapterDesc.RequiredFeatures.shaderImageGatherExtended = VK_TRUE;
    AdapterDesc.RequiredFeatures.imageCubeArray            = VK_TRUE;
    AdapterDesc.RequiredFeatures11.shaderDrawParameters    = VK_TRUE;
    AdapterDesc.RequiredFeatures12.hostQueryReset          = VK_TRUE;

    PhysicalDevice = new FVulkanPhysicalDevice(GetInstance());
    if (!PhysicalDevice->Initialize(AdapterDesc))
    {
        VULKAN_ERROR("Failed to initialize VulkanPhyscicalDevice");
        return false;
    }

    FVulkanDeviceCreateInfo DeviceDesc;
    DeviceDesc.RequiredExtensionNames = AdapterDesc.RequiredExtensionNames;
    DeviceDesc.OptionalExtensionNames = AdapterDesc.OptionalExtensionNames;
    DeviceDesc.RequiredFeatures       = AdapterDesc.RequiredFeatures;
    DeviceDesc.RequiredFeatures11     = AdapterDesc.RequiredFeatures11;
    DeviceDesc.RequiredFeatures12     = AdapterDesc.RequiredFeatures12;
    
    Device = new FVulkanDevice(GetInstance(), GetAdapter());
    if (!Device->Initialize(DeviceDesc))
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

void FVulkanRHI::RHIBeginFrame()
{
    // Update timestamp period, this is necessary on MoltenVK in order to get correct measurements
    {
        VkPhysicalDeviceProperties Properties;
        vkGetPhysicalDeviceProperties(PhysicalDevice->GetVkPhysicalDevice(), &Properties);
        FVulkanDeviceLimits::TimestampPeriod = Properties.limits.timestampPeriod;
    }
}

void FVulkanRHI::RHIEndFrame()
{
    // NOTE: Empty for now
}

FRHITexture* FVulkanRHI::RHICreateTexture(const FRHITextureDesc& InDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData)
{
    FVulkanTextureRef NewTexture = new FVulkanTexture(GetDevice(), InDesc);
    if (!NewTexture->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewTexture.ReleaseOwnership();
    }
}

FRHIBuffer* FVulkanRHI::RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData)
{
    FVulkanBufferRef NewBuffer = new FVulkanBuffer(GetDevice(), InDesc);
    if (!NewBuffer->Initialize(InInitialState, InInitialData))
    {
        return nullptr;
    }
    else
    {
        return NewBuffer.ReleaseOwnership();
    }
}

FRHISamplerState* FVulkanRHI::RHICreateSamplerState(const FRHISamplerStateDesc& InDesc)
{
    TScopedLock Lock(SamplerStateMapCS);

    TSharedRef<FVulkanSamplerState> Result;

    // Check if there already is an existing sampler state with this description
    if (TSharedRef<FVulkanSamplerState>* ExistingSamplerState = SamplerStateMap.Find(InDesc))
    {
        Result = *ExistingSamplerState;
    }
    else
    {
        Result = new FVulkanSamplerState(GetDevice(), InDesc);
        if (!Result->Initialize())
        {
            return nullptr;
        }
        else
        {
            SamplerStateMap.Add(InDesc, Result);
        }
    }

    return Result.ReleaseOwnership();
}

FRHIViewport* FVulkanRHI::RHICreateViewport(const FRHIViewportDesc& InDesc)
{
    FVulkanViewportRef NewViewport = new FVulkanViewport(Device, GraphicsCommandContext, InDesc);
    if (!NewViewport->Initialize())
    {
        return nullptr;
    }
    else
    {
        return NewViewport.ReleaseOwnership();
    }
}

FRHIQuery* FVulkanRHI::RHICreateQuery()
{
    return new FVulkanQuery(Device);
}

FRHIRayTracingScene* FVulkanRHI::RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& InDesc)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(InDesc);
    return nullptr;
}

FRHIRayTracingGeometry* FVulkanRHI::RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc)
{
    FRayTracingGeometryBuildInfo BuildInfo;
    BuildInfo.VertexBuffer = InDesc.VertexBuffer;
    BuildInfo.NumVertices  = InDesc.NumVertices;
    BuildInfo.IndexBuffer  = InDesc.IndexBuffer;
    BuildInfo.NumIndices   = InDesc.NumIndices;
    BuildInfo.IndexFormat  = InDesc.IndexFormat;
    BuildInfo.bUpdate      = false;

    GraphicsCommandContext->RHIStartContext();

    FVulkanRayTracingGeometryRef NewGeometry = new FVulkanRayTracingGeometry(GetDevice(), InDesc);
    if (!NewGeometry->Build(*GraphicsCommandContext, BuildInfo))
    {
        DEBUG_BREAK();
        NewGeometry.Reset();
    }

    GraphicsCommandContext->RHIFinishContext();
    return NewGeometry.ReleaseOwnership();
}

FRHIShaderResourceView* FVulkanRHI::RHICreateShaderResourceView(const FRHITextureSRVDesc& InDesc)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(InDesc.Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR("Texture cannot be nullptr");
        return nullptr;
    }
    
    FVulkanShaderResourceViewRef NewShaderResourceView = new FVulkanShaderResourceView(GetDevice(), VulkanTexture);
    if (!NewShaderResourceView->CreateTextureView(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewShaderResourceView.ReleaseOwnership();
    }
}

FRHIShaderResourceView* FVulkanRHI::RHICreateShaderResourceView(const FRHIBufferSRVDesc& InDesc)
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(InDesc.Buffer);
    if (!VulkanBuffer)
    {
        VULKAN_ERROR("Buffer cannot be nullptr");
        return nullptr;
    }
    
    FVulkanShaderResourceViewRef NewShaderResourceView = new FVulkanShaderResourceView(GetDevice(), VulkanBuffer);
    if (!NewShaderResourceView->CreateBufferView(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewShaderResourceView.ReleaseOwnership();
    }
}

FRHIUnorderedAccessView* FVulkanRHI::RHICreateUnorderedAccessView(const FRHITextureUAVDesc& InDesc)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(InDesc.Texture);
    if (!VulkanTexture)
    {
        VULKAN_ERROR("Texture cannot be nullptr");
        return nullptr;
    }
    
    FVulkanUnorderedAccessViewRef NewUnorderedAccessView = new FVulkanUnorderedAccessView(GetDevice(), VulkanTexture);
    if (!NewUnorderedAccessView->CreateTextureView(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewUnorderedAccessView.ReleaseOwnership();
    }
}

FRHIUnorderedAccessView* FVulkanRHI::RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& InDesc)
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(InDesc.Buffer);
    if (!VulkanBuffer)
    {
        VULKAN_ERROR("Buffer cannot be nullptr");
        return nullptr;
    }
    
    FVulkanUnorderedAccessViewRef NewUnorderedAccessView = new FVulkanUnorderedAccessView(GetDevice(), VulkanBuffer);
    if (!NewUnorderedAccessView->CreateBufferView(InDesc))
    {
        return nullptr;
    }
    else
    {
        return NewUnorderedAccessView.ReleaseOwnership();
    }
}

FRHIComputeShader* FVulkanRHI::RHICreateComputeShader(const TArray<uint8>& ShaderCode)
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

FRHIVertexShader* FVulkanRHI::RHICreateVertexShader(const TArray<uint8>& ShaderCode)
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

FRHIHullShader* FVulkanRHI::RHICreateHullShader(const TArray<uint8>& ShaderCode)
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

FRHIDomainShader* FVulkanRHI::RHICreateDomainShader(const TArray<uint8>& ShaderCode)
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

FRHIGeometryShader* FVulkanRHI::RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
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

FRHIMeshShader* FVulkanRHI::RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIAmplificationShader* FVulkanRHI::RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    // TODO: Finish this
    UNREFERENCED_VARIABLE(ShaderCode);
    return nullptr;
}

FRHIPixelShader* FVulkanRHI::RHICreatePixelShader(const TArray<uint8>& ShaderCode)
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

FRHIRayGenShader* FVulkanRHI::RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
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

FRHIRayAnyHitShader* FVulkanRHI::RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
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

FRHIRayClosestHitShader* FVulkanRHI::RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
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

FRHIRayMissShader* FVulkanRHI::RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
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

FRHIDepthStencilState* FVulkanRHI::RHICreateDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
{
    return new FVulkanDepthStencilState(InInitializer);
}

FRHIRasterizerState* FVulkanRHI::RHICreateRasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
{
    return new FVulkanRasterizerState(GetDevice(), InInitializer);
}

FRHIBlendState* FVulkanRHI::RHICreateBlendState(const FRHIBlendStateInitializer& InInitializer)
{
    return new FVulkanBlendState(InInitializer);
}

FRHIVertexInputLayout* FVulkanRHI::RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& InInitializer)
{
    return new FVulkanVertexInputLayout(InInitializer);
}

FRHIGraphicsPipelineState* FVulkanRHI::RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& InInitializer)
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

FRHIComputePipelineState* FVulkanRHI::RHICreateComputePipelineState(const FRHIComputePipelineStateInitializer& InInitializer)
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

FRHIRayTracingPipelineState* FVulkanRHI::RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc)
{
    return new FVulkanRayTracingPipelineState();
}

bool FVulkanRHI::RHIQueryUAVFormatSupport(EFormat Format) const
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

FString FVulkanRHI::RHIGetAdapterName() const
{
    VULKAN_ERROR_COND(PhysicalDevice != nullptr, "PhysicalDevice is not initialized properly");
    VkPhysicalDeviceProperties DeviceProperties = PhysicalDevice->GetProperties();
    return FString(DeviceProperties.deviceName);
}

void FVulkanRHI::EnqueueResourceDeletion(FRHIResource* Resource)
{
    if (Resource)
    {
        DeletionQueue.Emplace(Resource);
    }
}

void FVulkanRHI::ProcessPendingCommands()
{
    bool bProcess = true;
    while (bProcess)
    {
        FVulkanCommandPacket* CommandPacket = nullptr;
        if (PendingSubmissions.Peek(CommandPacket))
        {
            CHECK(CommandPacket != nullptr);
            if (!CommandPacket->IsExecutionFinished())
            {
                bProcess = false;
                break;
            }
            else
            {
                // If we are finished we remove the item from the queue
                PendingSubmissions.Dequeue();
                CommandPacket->HandleSubmitFinished();
            }
        }
        else
        {
            bProcess = false;
        }
    }
}

void FVulkanRHI::SubmitCommands(FVulkanCommandPacket* CommandPacket)
{
    CHECK(CommandPacket != nullptr);

    if (!CommandPacket->IsEmpty())
    {
        DeletionQueue.Dequeue(CommandPacket->Resources);
        CommandPacket->Submit();
        PendingSubmissions.Enqueue(CommandPacket);
    }
}
