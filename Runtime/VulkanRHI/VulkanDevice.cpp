#include "Core/Containers/Array.h"
#include "Core/Templates/CString.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanInstance.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanExtensions.h"
#include "VulkanRHI/Platform/VulkanPlatform.h"

// -------------------------------------------------------------------------------------------
// Vulkan Device Feature Support
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool   GVulkanForceBinding                        = false;
VULKANRHI_API bool   GVulkanAllowNullDescriptors                = true;
VULKANRHI_API bool   GVulkanAllowGeometryShaders                = true;
VULKANRHI_API bool   GVulkanAllowResetCommandBuffers            = false;
VULKANRHI_API bool   GVulkanRobustBufferAccessEnabled           = false;
VULKANRHI_API bool   GVulkanGPUAssistedValidationEnabled        = false;

VULKANRHI_API bool   GVulkanSupportsDepthClip                   = false;
VULKANRHI_API bool   GVulkanSupportsNullDescriptors             = false;
VULKANRHI_API bool   GVulkanSupportsRobustness2                 = false;
VULKANRHI_API bool   GVulkanSupportsConservativeRasterization   = false;
VULKANRHI_API float  GVulkanMaxExtraPrimitiveOverestimationSize = 0.0f;
VULKANRHI_API bool   GVulkanSupportsPipelineCacheControl        = false;
VULKANRHI_API bool   GVulkanSupportsMultiviews                  = false;
VULKANRHI_API bool   GVulkanSupportsBindless                    = false;
VULKANRHI_API bool   GVulkanSupportsDepthBoundsTest             = false;
VULKANRHI_API bool   GVulkanSupportsSparseBinding               = false;
VULKANRHI_API bool   GVulkanSupportsSparseResidency2D           = false;
VULKANRHI_API bool   GVulkanSupportsSparseResidency3D           = false;
VULKANRHI_API bool   GVulkanSupportsSparseResidencyAliased      = false;
VULKANRHI_API bool   GVulkanSupportsGeometryShader              = false;
VULKANRHI_API bool   GVulkanSupportsTessellation                = false;

VULKANRHI_API uint32 GVulkanMaxMultiviewViewCount               = 1;
VULKANRHI_API uint32 GVulkanMaxDrawIndirectCount                = 1;

// -------------------------------------------------------------------------------------------
// Programmable sample positions (VK_EXT_sample_locations)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanSupportsSampleLocations = false;

// -------------------------------------------------------------------------------------------
// Fragment shader interlock (VK_EXT_fragment_shader_interlock)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanSupportsFragmentShaderInterlock = false;

// -------------------------------------------------------------------------------------------
// Ray Tracing (VK_KHR_ray_tracing_pipeline, VK_KHR_ray_query)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanSupportsRayTracingPipeline     = false;
VULKANRHI_API bool GVulkanSupportsRayQuery               = false;
VULKANRHI_API bool GVulkanSupportsAccelerationStructures = false;

// -------------------------------------------------------------------------------------------
// Variable Rate Shading (VK_KHR_fragment_shading_rate)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool   GVulkanSupportsFragmentShadingRate = false;
VULKANRHI_API uint32 GVulkanShadingRateTileSize         = 0;

// -------------------------------------------------------------------------------------------
// Mesh Shaders (VK_EXT_mesh_shader)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool   GVulkanSupportsMeshShaders         = false;
VULKANRHI_API uint32 GVulkanMaxMeshOutputVertices       = 0;
VULKANRHI_API uint32 GVulkanMaxMeshWorkGroupInvocations = 0;
VULKANRHI_API uint32 GVulkanMaxTaskWorkGroupInvocations = 0;

// -------------------------------------------------------------------------------------------
// Dynamic Rendering (VK_KHR_dynamic_rendering / Vulkan 1.3)
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanUseDynamicRendering = true;

// -------------------------------------------------------------------------------------------
// Descriptor Set Management
// -------------------------------------------------------------------------------------------

VULKANRHI_API bool GVulkanUseDescriptorCache = true;

// -------------------------------------------------------------------------------------------
// Descriptor / Heap Limits
// -------------------------------------------------------------------------------------------

VULKANRHI_API uint32 GVulkanMaxDescriptorSetSamplers       = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetSampledImages  = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetStorageImages  = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetUniformBuffers = 0;
VULKANRHI_API uint32 GVulkanMaxDescriptorSetStorageBuffers = 0;

// -------------------------------------------------------------------------------------------
// FVulkanCoreFeatures
// -------------------------------------------------------------------------------------------

template <typename FeatureStructType>
static bool CheckRequiredFeaturesHelper(const FeatureStructType& Required, const FeatureStructType& Available, const char* StructName)
{
    TVulkanFeatureView<const FeatureStructType> RequiredView(Required);
    TVulkanFeatureView<const FeatureStructType> AvailableView(Available);

    for (SIZE_T i = 0; i < RequiredView.Size(); ++i)
    {
        if (RequiredView[i] == VK_TRUE && AvailableView[i] != VK_TRUE)
        {
            VULKAN_WARNING("PhysicalDevice does not support required device-feature %s[%llu]", StructName, static_cast<uint64>(i));
            return false;
        }
    }

    return true;
}

template <typename FeatureStructType>
static void EnableAvailableFeaturesHelper(FeatureStructType& OutEnabled, const FeatureStructType& Desired, const FeatureStructType& Available)
{
    TVulkanFeatureView<FeatureStructType>       EnabledView(OutEnabled);
    TVulkanFeatureView<const FeatureStructType> DesiredView(Desired);
    TVulkanFeatureView<const FeatureStructType> AvailableView(Available);

    for (SIZE_T i = 0; i < DesiredView.Size(); ++i)
    {
        if (DesiredView[i] == VK_TRUE && AvailableView[i] == VK_TRUE)
        {
            EnabledView[i] = VK_TRUE;
        }
    }
}

void FVulkanCoreFeatures::BuildQueryChain(VkPhysicalDeviceFeatures2& Root)
{
    Features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    AddAllToStructChain(Root, Features11, Features12, Features13);
}

bool FVulkanCoreFeatures::CheckRequired(VkPhysicalDevice PhysicalDevice) const
{
    VkPhysicalDeviceFeatures2 DeviceFeatures2 = {};
    DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    FVulkanCoreFeatures Available;

    Available.BuildQueryChain(DeviceFeatures2);

    vkGetPhysicalDeviceFeatures2(PhysicalDevice, &DeviceFeatures2);
    Available.Features10 = DeviceFeatures2.features;

    if (!CheckRequiredFeaturesHelper(Features10, Available.Features10, "VkPhysicalDeviceFeatures"))
    {
        return false;
    }
    
    if (!CheckRequiredFeaturesHelper(Features11, Available.Features11, "VkPhysicalDeviceVulkan11Features"))
    {
        return false;
    }
    
    if (!CheckRequiredFeaturesHelper(Features12, Available.Features12, "VkPhysicalDeviceVulkan12Features"))
    {
        return false;
    }
    
    if (!CheckRequiredFeaturesHelper(Features13, Available.Features13, "VkPhysicalDeviceVulkan13Features"))
    {
        return false;
    }

    return true;
}

void FVulkanCoreFeatures::EnableAvailable(FVulkanCoreFeatures& OutEnabled, const FVulkanCoreFeatures& Available) const
{
    EnableAvailableFeaturesHelper(OutEnabled.Features10, Features10, Available.Features10);
    EnableAvailableFeaturesHelper(OutEnabled.Features11, Features11, Available.Features11);
    EnableAvailableFeaturesHelper(OutEnabled.Features12, Features12, Available.Features12);
    EnableAvailableFeaturesHelper(OutEnabled.Features13, Features13, Available.Features13);
}

void FVulkanCoreFeatures::BuildEnableChain(VkPhysicalDeviceFeatures2& Root)
{
    Features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    AddAllToStructChain(Root, Features11, Features12, Features13);
}

// -------------------------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------------------------

static FString GetQueuePropertiesAsString(const VkQueueFamilyProperties& Properties)
{
    FString PropertyString = "QueueCount=" + TTypeToString<int32>::ToString(Properties.queueCount) + ", QueueBits=(";
    if (Properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
        PropertyString += "GRAPHICS | ";
    }
    
    if (Properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
        PropertyString += "COMPUTE | ";
    }

    if (Properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
        PropertyString += "COPY | ";
    }

    PropertyString.Pop();
    PropertyString.Pop();
    PropertyString.Pop();
    PropertyString += ')';

    return PropertyString;
}

FVulkanPhysicalDevice::FVulkanPhysicalDevice(FVulkanInstance* InInstance)
    : Instance(InInstance)
    , PhysicalDevice(VK_NULL_HANDLE)
{
}

FVulkanPhysicalDevice::~FVulkanPhysicalDevice()
{
    Instance       = nullptr;
    PhysicalDevice = VK_NULL_HANDLE;
}

bool FVulkanPhysicalDevice::Initialize(const FVulkanDeviceCreateInfo& InDeviceCreateInfo)
{
    VkResult Result = VK_SUCCESS;

    uint32 AdapterCount = 0;
    Result = vkEnumeratePhysicalDevices(Instance->GetVkInstance(), &AdapterCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve AdapterCount");
        return false;
    }

    TArray<VkPhysicalDevice> Adapters(AdapterCount);
    Result = vkEnumeratePhysicalDevices(Instance->GetVkInstance(), &AdapterCount, Adapters.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve available Adapters");
        return false;
    }

    if (AdapterCount < 1)
    {
        VULKAN_ERROR_CRITICAL("No Adapters available");
        return false;
    }

    IConsoleVariable* CVarVerboseLogging = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.VerboseLogging");

    const bool bVerboseLogging = CVarVerboseLogging && CVarVerboseLogging->GetBool();
    if (bVerboseLogging)
    {
        VULKAN_INFO("Available adapters:");

        for (VkPhysicalDevice CurrentAdapter : Adapters)
        {
            VkPhysicalDeviceProperties AdapterProperties;
            vkGetPhysicalDeviceProperties(CurrentAdapter, &AdapterProperties);
            LOG_INFO("    '%s' Supports Vulkan '%s'", AdapterProperties.deviceName, *GetVersionAsString(AdapterProperties.apiVersion));
        }
    }

    // GPU selection
    TArray<VkPhysicalDevice> AcceptedAdapers;
    AcceptedAdapers.Reserve(Adapters.Size());

    TArray<VkPhysicalDevice> DiscreteAdapers;
    DiscreteAdapers.Reserve(Adapters.Size());

    for (VkPhysicalDevice CurrentAdapter : Adapters)
    {
        VkPhysicalDeviceProperties AdapterProperties;
        vkGetPhysicalDeviceProperties(CurrentAdapter, &AdapterProperties);

        if (AdapterProperties.apiVersion < VK_API_VERSION_1_3)
        {
            VULKAN_INFO("Skipping device '%s' since it's api-version is below Vulkan 1.3 (apiVersion=%s)", AdapterProperties.deviceName, *GetVersionAsString(AdapterProperties.apiVersion));
            continue;
        }

        if (!InDeviceCreateInfo.RequiredFeatures.CheckRequired(CurrentAdapter))
        {
            continue;
        }

        // Find indices for queue-families
        TOptional<FVulkanQueueFamilyIndices> QueueIndices = GetQueueFamilyIndices(CurrentAdapter);
        if (!QueueIndices)
        {
            VULKAN_WARNING("PhysicalDevice '%s' does not support all required QueueFamilies", AdapterProperties.deviceName);
            continue;
        }

        // Check if required extension for device is supported
        uint32 DeviceExtensionCount = 0;
        Result = vkEnumerateDeviceExtensionProperties(CurrentAdapter, nullptr, &DeviceExtensionCount, nullptr);
        
        if (VULKAN_FAILED(Result))
        {
            continue;
        }

        TArray<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionCount);
        Result = vkEnumerateDeviceExtensionProperties(CurrentAdapter, nullptr, &DeviceExtensionCount, AvailableDeviceExtensions.Data());
        
        if (VULKAN_FAILED(Result))
        {
            continue;
        }

        if (bVerboseLogging)
        {
            VULKAN_INFO("The adapter '%s' has the following available extensions (NumExtensions=%d):", AdapterProperties.deviceName, AvailableDeviceExtensions.Size());
            for (const VkExtensionProperties& Extension : AvailableDeviceExtensions)
            {
                LOG_INFO("    '%s'", Extension.extensionName);
            }
        }

        bool bMissingRequiredExtension = false;
        for (const TUniquePtr<FVulkanDeviceExtension>& Extension : InDeviceCreateInfo.Extensions)
        {
            if (!Extension->IsRequired())
            {
                continue;
            }

            bool bFound = false;
            for (const VkExtensionProperties& Property : AvailableDeviceExtensions)
            {
                if (FCString::Strcmp(Extension->GetExtensionName(), Property.extensionName) == 0)
                {
                    bFound = true;
                    break;
                }
            }

            if (!bFound)
            {
                VULKAN_WARNING("Adapter '%s' does not support required extension '%s'", AdapterProperties.deviceName, Extension->GetExtensionName());
                bMissingRequiredExtension = true;
            }
        }

        if (bMissingRequiredExtension)
        {
            continue;
        }

        AcceptedAdapers.Add(CurrentAdapter);
        if (AdapterProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            DiscreteAdapers.Add(CurrentAdapter);
        }
    }

    // Select the first discrete-adapter we found
    if (!DiscreteAdapers.IsEmpty())
    {
        PhysicalDevice = DiscreteAdapers[0];
    }
    
    // If no discrete adapter is found...
    if (PhysicalDevice == VK_NULL_HANDLE)
    {
        if (AcceptedAdapers.IsEmpty())
        {
            // ... we failed to find a suitable adapter
            VULKAN_ERROR_CRITICAL("Failed to find a suitable PhysicalDevice");
            return false;
        }
        else
        {
            // ... select the first accepted one
            PhysicalDevice = AcceptedAdapers[0];
        }
    }
    
    // Retrieve and cache information about the physical-device
    vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);
    vkGetPhysicalDeviceFeatures(PhysicalDevice, &DeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &DeviceMemoryProperties);

    // Get Get Physical Device Properties
    FMemory::Memzero(&DeviceProperties2);
    DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

    vkGetPhysicalDeviceProperties2(PhysicalDevice, &DeviceProperties2);

    // Get Physical Device Feature (For Vulkan 1.1 and Vulkan 1.2 and extensions)
    FMemory::Memzero(&DeviceFeatures2);
    DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    // Vulkan 1.1 features
    FMemory::Memzero(&DeviceFeatures11);
    DeviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

    // Vulkan 1.2 features
    FMemory::Memzero(&DeviceFeatures12);
    DeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    
    // Helper for checking for extensions
    AddAllToStructChain(DeviceFeatures2, DeviceFeatures11, DeviceFeatures12);

    // Get the physical device features
    vkGetPhysicalDeviceFeatures2(PhysicalDevice, &DeviceFeatures2);

    // Get Physical Device Memory Properties
    FMemory::Memzero(&DeviceMemoryProperties2);
    DeviceMemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

    vkGetPhysicalDeviceMemoryProperties2(PhysicalDevice, &DeviceMemoryProperties2);
    
    VULKAN_INFO("Using adapter '%s' Which supports Vulkan '%s'", DeviceProperties.deviceName, *GetVersionAsString(DeviceProperties.apiVersion));
    return true;
}

TOptional<FVulkanQueueFamilyIndices> FVulkanPhysicalDevice::GetQueueFamilyIndices(VkPhysicalDevice PhysicalDevice)
{
    const auto GetQueueFamilyIndex = [](VkQueueFlagBits QueueFlag, const TArray<VkQueueFamilyProperties>& QueueFamilies) -> uint32
    {
        if (QueueFlag == VK_QUEUE_COMPUTE_BIT)
        {
            for (uint32 QueueIndex = 0; QueueIndex < uint32(QueueFamilies.Size()); ++QueueIndex)
            {
                const VkQueueFamilyProperties& Properties = QueueFamilies[QueueIndex];
                if (Properties.queueCount > 0)
                {
                    const uint32 CurrentQueueFlags = Properties.queueFlags;
                    if ((CurrentQueueFlags & QueueFlag) && ((CurrentQueueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                    {
                        return QueueIndex;
                    }
                }
            }
        }

        if (QueueFlag == VK_QUEUE_TRANSFER_BIT)
        {
            for (uint32 QueueIndex = 0; QueueIndex < uint32(QueueFamilies.Size()); ++QueueIndex)
            {
                const VkQueueFamilyProperties& Properties = QueueFamilies[QueueIndex];
                if (Properties.queueCount > 0)
                {
                    const uint32 CurrentQueueFlags = Properties.queueFlags;
                    if ((CurrentQueueFlags & QueueFlag) && ((CurrentQueueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0))
                    {
                        return QueueIndex;
                    }
                }
            }
        }

        for (uint32 QueueIndex = 0; QueueIndex < uint32(QueueFamilies.Size()); ++QueueIndex)
        {
            const VkQueueFamilyProperties& Properties = QueueFamilies[QueueIndex];
            if (Properties.queueCount > 0)
            {
                if (Properties.queueFlags & QueueFlag)
                {
                    return QueueIndex;
                }
            }
        }

        return uint32(~0u);
    };

    // Retrieve the queue indices for the adapter
    TOptional<FVulkanQueueFamilyIndices> QueueIndicies;

    uint32 QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, nullptr);

    TArray<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies.Data());
    
    IConsoleVariable* VerboseVulkan = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.VerboseLogging");
    if (VerboseVulkan && VerboseVulkan->GetBool())
    {
        uint32 Index = 0;
        for (const VkQueueFamilyProperties& Properties : QueueFamilies)
        {
            const FString PropertyString = GetQueuePropertiesAsString(Properties);
            VULKAN_INFO("Queue[%d]: %s", Index, *PropertyString);
            Index++;
        }
    }
    
    const uint32 GraphicsIndex = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, QueueFamilies);
    if (GraphicsIndex == (~0u))
    {
        return QueueIndicies;
    }

    QueueFamilies[GraphicsIndex].queueCount--;
    
    const uint32 CopyIndex = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, QueueFamilies);
    if (CopyIndex == (~0u))
    {
        return QueueIndicies;
    }
    
    QueueFamilies[CopyIndex].queueCount--;
    
    const uint32 ComputeIndex = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT , QueueFamilies);
    if (ComputeIndex == (~0u))
    {
        return QueueIndicies;
    }
    
    QueueFamilies[ComputeIndex].queueCount--;

    QueueIndicies.Emplace(GraphicsIndex, CopyIndex, ComputeIndex);
    return QueueIndicies;
}

uint32 FVulkanPhysicalDevice::FindMemoryTypeIndex(uint32 TypeFilter, VkMemoryPropertyFlags Properties)
{
    VkPhysicalDeviceMemoryProperties MemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

    for (uint32 MemoryIndex = 0; MemoryIndex < MemoryProperties.memoryTypeCount; ++MemoryIndex)
    {
        if ((TypeFilter & (1 << MemoryIndex)) && (MemoryProperties.memoryTypes[MemoryIndex].propertyFlags & Properties) == Properties)
        {
            return MemoryIndex;
        }
    }

    return TNumericLimits<int32>::Max();
}

VkFormatProperties FVulkanPhysicalDevice::GetFormatProperties(VkFormat Format) const
{
    VkFormatProperties FormatProperties = {};
    vkGetPhysicalDeviceFormatProperties(PhysicalDevice, Format, &FormatProperties);
    return FormatProperties;
}

FVulkanDevice::FVulkanDevice(FVulkanInstance* InInstance, FVulkanPhysicalDevice* InAdapter)
    : Instance(InInstance)
    , PhysicalDevice(InAdapter)
    , Device(VK_NULL_HANDLE)
    , RenderPassCache(nullptr)
    , MemoryManager(nullptr)
    , FenceManager(nullptr)
    , FrameFence(nullptr)
    , PipelineLayoutManager(nullptr)
    , PipelineStateManager(nullptr)
    , DescriptorSetCache(nullptr)
    , DescriptorPoolManager(nullptr)
    , TimingQueryPoolManager(nullptr)
    , OcclusionQueryPoolManager(nullptr)
#if VULKAN_ENABLE_CRASH_MARKERS
    , bSupportsAMDBufferMarker(false)
    , bSupportsNVDiagnosticCheckpoints(false)
#endif
{
    if (IConsoleVariable* UseDynamicRenderingVar = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.UseDynamicRendering"))
    {
        GVulkanUseDynamicRendering = UseDynamicRenderingVar->GetBool();
    }

    if (IConsoleVariable* UseDescriptorCacheVar = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.UseDescriptorCache"))
    {
        GVulkanUseDescriptorCache = UseDescriptorCacheVar->GetBool();
    }

    DescriptorSetCache        = new FVulkanDescriptorSetCache(this);
    DescriptorPoolManager     = GVulkanUseDescriptorCache ? nullptr : new FVulkanDescriptorPoolManager(this);
    TimingQueryPoolManager    = new FVulkanQueryPoolManager(this, EQueryType::Timestamp);
    OcclusionQueryPoolManager = new FVulkanQueryPoolManager(this, EQueryType::Occlusion);
    PipelineLayoutManager     = new FVulkanPipelineLayoutManager(this);
    FenceManager              = new FVulkanFenceManager(this);
    RenderPassCache           = new FVulkanRenderPassCache(this);
}

FVulkanDevice::~FVulkanDevice()
{
    // Release default resources
    DefaultResources.Release(*this);

    // Release all samplers
    {
        TScopedLock Lock(SamplerMapCS);

        for (const auto SamplerPair : SamplerMap)
        {
            if (VULKAN_CHECK_HANDLE(SamplerPair.Second))
            {
                vkDestroySampler(GetVkDevice(), SamplerPair.Second, nullptr);
            }
        }

        SamplerMap.Clear();
    }

    // Release the PipelineCache
    if (PipelineStateManager)
    {
        PipelineStateManager->SaveCacheData();
        delete PipelineStateManager;
    }
    
    SAFE_DELETE(DescriptorPoolManager);
    SAFE_DELETE(DescriptorSetCache);
    SAFE_DELETE(TimingQueryPoolManager);
    SAFE_DELETE(OcclusionQueryPoolManager);
    SAFE_DELETE(PipelineLayoutManager);
    SAFE_DELETE(RenderPassCache);
    SAFE_DELETE(FrameFence);
    SAFE_DELETE(FenceManager);
    SAFE_DELETE(MemoryManager);
    
    // Destroy the device here
    if (VULKAN_CHECK_HANDLE(Device))
    {
        vkDestroyDevice(Device, nullptr);
    }
}

bool FVulkanDevice::Initialize(FVulkanDeviceCreateInfo& InDeviceCreateInfo)
{
    if (!PhysicalDevice)
    {
        VULKAN_ERROR_CRITICAL("PhysicalDevice is not initalized correctly");
        return false;
    }

    VkResult Result = VK_SUCCESS;

    // -------------------------------------------------------------------------------------------
    // Enumerate and resolve extensions
    // -------------------------------------------------------------------------------------------
    
    uint32 DeviceExtensionCount = 0;
    Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve the device extension count");
        return false;
    }

    TArray<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionCount);
    Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice->GetVkPhysicalDevice(), nullptr, &DeviceExtensionCount, AvailableDeviceExtensions.Data());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve the device extensions");
        return false;
    }

    TArray<const CHAR*> EnabledExtensionNames;
    for (const TUniquePtr<FVulkanDeviceExtension>& Extension : InDeviceCreateInfo.Extensions)
    {
        Extension->SetEnabled(false);

        if (!Extension->ShouldEnable())
        {
            continue;
        }

        for (const VkExtensionProperties& Property : AvailableDeviceExtensions)
        {
            if (FCString::Strcmp(Extension->GetExtensionName(), Property.extensionName) == 0)
            {
                Extension->SetEnabled(true);
                EnabledExtensionNames.Add(Property.extensionName);
                ExtensionNames.Emplace(Property.extensionName);
                break;
            }
        }

        if (!Extension->IsEnabled() && Extension->IsRequired())
        {
            VULKAN_ERROR_CRITICAL("Required device extension '%s' is not available", Extension->GetExtensionName());
            return false;
        }
    }

    if (IConsoleVariable* VerboseVulkan = FConsoleManager::Get().FindConsoleVariable("VulkanRHI.VerboseLogging"))
    {
        if (VerboseVulkan->GetBool() && !EnabledExtensionNames.IsEmpty())
        {
            VULKAN_INFO("Enabled Device Extensions:");
            for (const CHAR* ExtensionName : EnabledExtensionNames)
            {
                LOG_INFO("    %s", ExtensionName);
            }
        }
    }

    // -------------------------------------------------------------------------------------------
    // Queues
    // -------------------------------------------------------------------------------------------

    QueueIndicies = FVulkanPhysicalDevice::GetQueueFamilyIndices(PhysicalDevice->GetVkPhysicalDevice());
    if (!QueueIndicies)
    {
        VULKAN_ERROR_CRITICAL("Failed to query queue indices");
        return false;
    }

    VULKAN_INFO("QueueIndicies: Graphics=%d, Compute=%d, Copy=%d, Present=%d", 
        QueueIndicies->GraphicsQueueIndex, QueueIndicies->ComputeQueueIndex, QueueIndicies->CopyQueueIndex, QueueIndicies->PresentQueueIndex);

    TSet<uint32> UniqueQueueIndices = 
    { 
        QueueIndicies->GraphicsQueueIndex, 
        QueueIndicies->CopyQueueIndex, 
        QueueIndicies->ComputeQueueIndex
    };

    if (QueueIndicies->PresentQueueIndex != uint32(~0) && QueueIndicies->HasSeparatePresentQueue())
    {
        UniqueQueueIndices.Add(QueueIndicies->PresentQueueIndex);
    }

    const float DefaultQueuePriority = 0.0f;

    TArray<VkDeviceQueueCreateInfo> QueueCreateInfos;
    for (uint32 QueueFamilyIndex : UniqueQueueIndices)
    {
        VkDeviceQueueCreateInfo QueueCreateInfo = {};
        QueueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
        QueueCreateInfo.queueCount       = 1;
        QueueCreateInfo.pQueuePriorities = &DefaultQueuePriority;
        QueueCreateInfos.Add(QueueCreateInfo);
    }

    // -------------------------------------------------------------------------------------------
    // Collect core features/properties + extension feature/property structs
    // -------------------------------------------------------------------------------------------

    VkPhysicalDeviceFeatures2 AvailableDeviceFeatures2 = {};
    AvailableDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    FVulkanCoreFeatures AvailableFeatures;

    VkPhysicalDeviceProperties2 AvailableDeviceProperties2 = {};
    AvailableDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

    VkPhysicalDeviceMultiviewProperties AvailableDeviceMultiviewProperties = {};
    AvailableDeviceMultiviewProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;

    {
        AvailableFeatures.BuildQueryChain(AvailableDeviceFeatures2);

        for (const TUniquePtr<FVulkanDeviceExtension>& Extension : InDeviceCreateInfo.Extensions)
        {
            if (Extension->IsEnabled())
            {
                Extension->PrepareDeviceFeatures(AvailableDeviceFeatures2);
            }
        }

        vkGetPhysicalDeviceFeatures2(PhysicalDevice->GetVkPhysicalDevice(), &AvailableDeviceFeatures2);
        AvailableFeatures.Features10 = AvailableDeviceFeatures2.features;
    }

    {
        AddToStructChain(AvailableDeviceProperties2, AvailableDeviceMultiviewProperties);

        for (const TUniquePtr<FVulkanDeviceExtension>& Extension : InDeviceCreateInfo.Extensions)
        {
            if (Extension->IsEnabled())
            {
                Extension->PrepareDeviceProperties(AvailableDeviceProperties2);
            }
        }

        vkGetPhysicalDeviceProperties2(PhysicalDevice->GetVkPhysicalDevice(), &AvailableDeviceProperties2);
    }

    // -------------------------------------------------------------------------------------------
    // Set core GVulkan* globals from collected caps
    // -------------------------------------------------------------------------------------------

    const VkPhysicalDeviceFeatures&   CoreDeviceFeatures10   = AvailableFeatures.Features10;
    const VkPhysicalDeviceProperties& CoreDeviceProperties10 = PhysicalDevice->GetProperties();

    GVulkanSupportsDepthBoundsTest        = (CoreDeviceFeatures10.depthBounds == VK_TRUE);
    GVulkanSupportsSparseBinding          = (CoreDeviceFeatures10.sparseBinding == VK_TRUE);
    GVulkanSupportsSparseResidency2D      = (CoreDeviceFeatures10.sparseResidencyImage2D == VK_TRUE);
    GVulkanSupportsSparseResidency3D      = (CoreDeviceFeatures10.sparseResidencyImage3D == VK_TRUE);
    GVulkanSupportsSparseResidencyAliased = (CoreDeviceFeatures10.sparseResidencyAliased == VK_TRUE);
    GVulkanSupportsGeometryShader         = (GVulkanAllowGeometryShaders && CoreDeviceFeatures10.geometryShader == VK_TRUE);
    GVulkanSupportsTessellation           = (CoreDeviceFeatures10.tessellationShader == VK_TRUE);

    if (AvailableFeatures.Features11.multiview)
    {
        GVulkanSupportsMultiviews    = true;
        GVulkanMaxMultiviewViewCount = Math::Max<uint32>(1u, AvailableDeviceMultiviewProperties.maxMultiviewViewCount);
    }
    else
    {
        GVulkanSupportsMultiviews    = false;
        GVulkanMaxMultiviewViewCount = 1u;
    }

    if (AvailableFeatures.Features13.pipelineCreationCacheControl)
    {
        GVulkanSupportsPipelineCacheControl = true;
    }

    if (AvailableFeatures.Features12.descriptorIndexing)
    {
        GVulkanSupportsBindless = true;
    }

#if VULKAN_ENABLE_CRASH_MARKERS
#if VK_AMD_buffer_marker
    bSupportsAMDBufferMarker = IsExtensionEnabled(VK_AMD_BUFFER_MARKER_EXTENSION_NAME);
#endif
#if VK_NV_device_diagnostic_checkpoints
    bSupportsNVDiagnosticCheckpoints = IsExtensionEnabled(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
#endif
#endif

    GVulkanMaxDrawIndirectCount           = CoreDeviceProperties10.limits.maxDrawIndirectCount;
    GVulkanMaxDescriptorSetSamplers       = CoreDeviceProperties10.limits.maxDescriptorSetSamplers;
    GVulkanMaxDescriptorSetSampledImages  = CoreDeviceProperties10.limits.maxDescriptorSetSampledImages;
    GVulkanMaxDescriptorSetStorageImages  = CoreDeviceProperties10.limits.maxDescriptorSetStorageImages;
    GVulkanMaxDescriptorSetUniformBuffers = CoreDeviceProperties10.limits.maxDescriptorSetUniformBuffers;
    GVulkanMaxDescriptorSetStorageBuffers = CoreDeviceProperties10.limits.maxDescriptorSetStorageBuffers;

    for (const TUniquePtr<FVulkanDeviceExtension>& Extension : InDeviceCreateInfo.Extensions)
    {
        if (Extension->IsEnabled())
        {
            Extension->ProcessQueriedFeatures();
        }
    }

    if (GVulkanSupportsDepthClip && !CoreDeviceFeatures10.depthClamp)
    {
        GVulkanSupportsDepthClip = false;
    }

    // -------------------------------------------------------------------------------------------
    // Resolve device layers
    // -------------------------------------------------------------------------------------------

    TArray<const CHAR*> EnabledDeviceLayerNames;
    if (!InDeviceCreateInfo.RequiredLayerNames.IsEmpty() || !InDeviceCreateInfo.OptionalLayerNames.IsEmpty())
    {
        uint32 DeviceLayerCount = 0;
        vkEnumerateDeviceLayerProperties(PhysicalDevice->GetVkPhysicalDevice(), &DeviceLayerCount, nullptr);

        TArray<VkLayerProperties> AvailableDeviceLayers(DeviceLayerCount);
        vkEnumerateDeviceLayerProperties(PhysicalDevice->GetVkPhysicalDevice(), &DeviceLayerCount, AvailableDeviceLayers.Data());

        for (const CHAR* RequiredLayer : InDeviceCreateInfo.RequiredLayerNames)
        {
            bool bFound = false;
            for (const VkLayerProperties& LayerProp : AvailableDeviceLayers)
            {
                if (FCString::Strcmp(RequiredLayer, LayerProp.layerName) == 0)
                {
                    EnabledDeviceLayerNames.Add(LayerProp.layerName);
                    LayerNames.Emplace(LayerProp.layerName);
                    bFound = true;
                    break;
                }
            }

            if (!bFound)
            {
                VULKAN_ERROR_CRITICAL("Required device layer '%s' is not available", RequiredLayer);
                return false;
            }
        }

        for (const CHAR* OptionalLayer : InDeviceCreateInfo.OptionalLayerNames)
        {
            for (const VkLayerProperties& LayerProp : AvailableDeviceLayers)
            {
                if (FCString::Strcmp(OptionalLayer, LayerProp.layerName) == 0)
                {
                    EnabledDeviceLayerNames.Add(LayerProp.layerName);
                    LayerNames.Emplace(LayerProp.layerName);
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------------------------------------
    // Build VkDeviceCreateInfo with feature enable chains
    // -------------------------------------------------------------------------------------------

    VkDeviceCreateInfo DeviceCreateInfo = {};
    DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.enabledLayerCount       = EnabledDeviceLayerNames.Size();
    DeviceCreateInfo.ppEnabledLayerNames     = EnabledDeviceLayerNames.Data();
    DeviceCreateInfo.enabledExtensionCount   = EnabledExtensionNames.Size();
    DeviceCreateInfo.ppEnabledExtensionNames = EnabledExtensionNames.Data();
    DeviceCreateInfo.queueCreateInfoCount    = QueueCreateInfos.Size();
    DeviceCreateInfo.pQueueCreateInfos       = QueueCreateInfos.Data();

    FVulkanCoreFeatures EnabledFeatures = InDeviceCreateInfo.RequiredFeatures;
    InDeviceCreateInfo.OptionalFeatures.EnableAvailable(EnabledFeatures, AvailableFeatures);
    
    GVulkanRobustBufferAccessEnabled = (EnabledFeatures.Features10.robustBufferAccess == VK_TRUE);

    if (GVulkanSupportsDepthClip && AvailableFeatures.Features10.depthClamp)
    {
        EnabledFeatures.Features10.depthClamp = VK_TRUE;
    }

    VkPhysicalDeviceFeatures2 EnableDeviceFeatures2 = {};
    EnableDeviceFeatures2.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    EnableDeviceFeatures2.features = EnabledFeatures.Features10;

    AddToStructChain(DeviceCreateInfo, EnableDeviceFeatures2);
    EnabledFeatures.BuildEnableChain(EnableDeviceFeatures2);

    for (const TUniquePtr<FVulkanDeviceExtension>& Extension : InDeviceCreateInfo.Extensions)
    {
        if (Extension->IsEnabled())
        {
            Extension->PrepareDeviceCreateInfo(DeviceCreateInfo);
        }
    }

    Result = vkCreateDevice(PhysicalDevice->GetVkPhysicalDevice(), &DeviceCreateInfo, nullptr, &Device);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Device");
        return false;
    }

    return true;
}

bool FVulkanDevice::PostLoaderInitalize()
{
    // Initialize PipelineStateManager
    PipelineStateManager = new FVulkanPipelineStateManager(this);
    if (!PipelineStateManager->Initialize())
    {
        return false;
    }

    // Initialize the device feature support
    if (!InitializeDeviceFeatureSupport())
    {
        return false;
    }

    MemoryManager = new FVulkanMemoryManager(this);
    if (!MemoryManager->Initialize())
    {
        VULKAN_ERROR_CRITICAL("FVulkanDevice: Failed to initialize MemoryManager");
        return false;
    }

    FrameFence = new FVulkanTimelineFence(this);
    if (!FrameFence->Initialize())
    {
        VULKAN_ERROR_CRITICAL("FVulkanDevice: Failed to initialize FrameFence");
        return false;
    }

    FrameFence->SetDebugName("FrameFence");

    return true;
}

bool FVulkanDevice::InitializeDeviceFeatureSupport()
{
    // -------------------------------------------------------------------------------------------
    // Baseline defaults
    // -------------------------------------------------------------------------------------------

    RHIDeviceFeatureSupport::bSupportsGeometryShaders                       = false;
    RHIDeviceFeatureSupport::bSupportRenderTargetArrayIndexFromVertexShader = false;

    RHIDeviceFeatureSupport::bSupportsViewInstancing     = false;
    RHIDeviceFeatureSupport::MaxViewInstanceCount        = 1;

    RHIDeviceFeatureSupport::bSupportsRayTracing         = false;
    RHIDeviceFeatureSupport::RayTracingTier              = ERayTracingTier::NotSupported;
    RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = 0;

    RHIDeviceFeatureSupport::bSupportsVRS                = false;
    RHIDeviceFeatureSupport::ShadingRateTier             = EShadingRateTier::NotSupported;
    RHIDeviceFeatureSupport::ShadingRateImageTileSize    = 0;

    RHIDeviceFeatureSupport::bSupportDrawIndirect        = true;
    RHIDeviceFeatureSupport::bSupportMultiDrawIndirect   = false;
    RHIDeviceFeatureSupport::MaxDrawIndirectCount        = 1;

    RHIDeviceFeatureSupport::MaxTexture1DSize            = 0;
    RHIDeviceFeatureSupport::MaxTexture1DArrayLayers     = 0;
    RHIDeviceFeatureSupport::MaxTexture2DSize            = 0;
    RHIDeviceFeatureSupport::MaxTexture2DArrayLayers     = 0;
    RHIDeviceFeatureSupport::MaxTexture3DWidth           = 0;
    RHIDeviceFeatureSupport::MaxTexture3DHeight          = 0;
    RHIDeviceFeatureSupport::MaxTexture3DDepth           = 0;
    RHIDeviceFeatureSupport::MaxCubeTextureSize          = 0;
    RHIDeviceFeatureSupport::MaxCubeArrayCount           = 0;

    RHIDeviceFeatureSupport::MaxBufferSize               = 0;
    RHIDeviceFeatureSupport::MaxConstantBufferSize       = 0;
    RHIDeviceFeatureSupport::MaxStorageBufferSize        = 0;
    RHIDeviceFeatureSupport::StructuredBufferMinStride   = 4;
    RHIDeviceFeatureSupport::StructuredBufferMaxStride   = 2048;
    RHIDeviceFeatureSupport::RawBufferRequiredAlignment  = 4;

    // -------------------------------------------------------------------------------------------
    // Core features/properties
    // -------------------------------------------------------------------------------------------

    VkPhysicalDevice PhysicalDeviceHandle = GetPhysicalDevice()->GetVkPhysicalDevice();

    // Core features
    const VkPhysicalDeviceFeatures& PhysicalDeviceFeatures = PhysicalDevice->GetFeatures();
    if (GVulkanAllowGeometryShaders && PhysicalDeviceFeatures.geometryShader)
    {
        RHIDeviceFeatureSupport::bSupportsGeometryShaders = true;
    }

    // Core properties
    const VkPhysicalDeviceProperties& PhysicalDeviceProperties = PhysicalDevice->GetProperties();

    {
        // DrawIndirect + MultiDrawIndirect
        if (PhysicalDeviceFeatures.multiDrawIndirect)
        {
            RHIDeviceFeatureSupport::bSupportMultiDrawIndirect = true;
            RHIDeviceFeatureSupport::MaxDrawIndirectCount      = PhysicalDeviceProperties.limits.maxDrawIndirectCount;
        }
        else
        {
            RHIDeviceFeatureSupport::bSupportMultiDrawIndirect = false;
            RHIDeviceFeatureSupport::MaxDrawIndirectCount      = 1;
        }

        // Texture / Image limits
        RHIDeviceFeatureSupport::MaxTexture1DSize        = PhysicalDeviceProperties.limits.maxImageDimension1D;
        RHIDeviceFeatureSupport::MaxTexture2DSize        = PhysicalDeviceProperties.limits.maxImageDimension2D;
        RHIDeviceFeatureSupport::MaxTexture3DWidth       = PhysicalDeviceProperties.limits.maxImageDimension3D;
        RHIDeviceFeatureSupport::MaxTexture3DHeight      = PhysicalDeviceProperties.limits.maxImageDimension3D;
        RHIDeviceFeatureSupport::MaxTexture3DDepth       = PhysicalDeviceProperties.limits.maxImageDimension3D;
        RHIDeviceFeatureSupport::MaxCubeTextureSize      = PhysicalDeviceProperties.limits.maxImageDimensionCube;
        RHIDeviceFeatureSupport::MaxTexture1DArrayLayers = PhysicalDeviceProperties.limits.maxImageArrayLayers;
        RHIDeviceFeatureSupport::MaxTexture2DArrayLayers = PhysicalDeviceProperties.limits.maxImageArrayLayers;
        RHIDeviceFeatureSupport::MaxCubeArrayCount       = PhysicalDeviceProperties.limits.maxImageArrayLayers / RHI_NUM_CUBE_FACES;

        // Buffer / Memory Limits 
        const uint32 MinBufferStride = sizeof(uint32); 
        RHIDeviceFeatureSupport::MaxConstantBufferSize      = PhysicalDeviceProperties.limits.maxUniformBufferRange; 
        RHIDeviceFeatureSupport::MaxStorageBufferSize       = PhysicalDeviceProperties.limits.maxStorageBufferRange; 
        RHIDeviceFeatureSupport::MaxBufferSize              = uint64(~0); 
        RHIDeviceFeatureSupport::StructuredBufferMinStride  = MinBufferStride; 
        RHIDeviceFeatureSupport::StructuredBufferMaxStride  = uint32(~0); 
        RHIDeviceFeatureSupport::RawBufferRequiredAlignment = MinBufferStride; 
    } 

    // -------------------------------------------------------------------------------------------
    // SV_RenderTargetArrayIndex from VS (shaderOutputLayer in Vulkan 1.2)
    // -------------------------------------------------------------------------------------------

    const VkPhysicalDeviceVulkan12Features& PhysicalDeviceFeatures12 = PhysicalDevice->GetFeaturesVulkan12();
    RHIDeviceFeatureSupport::bSupportRenderTargetArrayIndexFromVertexShader = PhysicalDeviceFeatures12.shaderOutputLayer ? true : false;

    // -------------------------------------------------------------------------------------------
    // View Instancing (multiview)
    // -------------------------------------------------------------------------------------------

    if (GVulkanSupportsMultiviews)
    {
        RHIDeviceFeatureSupport::MaxViewInstanceCount    = GVulkanMaxMultiviewViewCount;
        RHIDeviceFeatureSupport::bSupportsViewInstancing = RHIDeviceFeatureSupport::MaxViewInstanceCount > 1;
    }
    else
    {
        RHIDeviceFeatureSupport::MaxViewInstanceCount    = 1;
        RHIDeviceFeatureSupport::bSupportsViewInstancing = false;
    }

    // -------------------------------------------------------------------------------------------
    // Ray Tracing
    // Tier1_1: Only if VK_KHR_ray_query is available
    // Tier1: Pipeline ray tracing without ray query
    // Supports ray tracing if acceleration structures + (pipeline OR ray query)
    // -------------------------------------------------------------------------------------------

    const bool bHasRayTracingPipeline     = IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    const bool bHasRayQuery               = IsExtensionEnabled(VK_KHR_RAY_QUERY_EXTENSION_NAME);
    const bool bHasAccelerationStructures = IsExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

    if (bHasAccelerationStructures && bHasRayTracingPipeline)
    {
        // Query pipeline RT properties for recursion depth
        VkPhysicalDeviceProperties2 DeviceProperties2 = {};
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR DeviceRayTracingPipelineProperties = {};
        DeviceRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        AddToStructChain(DeviceProperties2, DeviceRayTracingPipelineProperties);

        vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

        RHIDeviceFeatureSupport::bSupportsRayTracing         = true;
        RHIDeviceFeatureSupport::RayTracingTier              = bHasRayQuery ? ERayTracingTier::Tier1_1 : ERayTracingTier::Tier1;
        RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = DeviceRayTracingPipelineProperties.maxRayRecursionDepth;
    }
    else
    {
        RHIDeviceFeatureSupport::bSupportsRayTracing         = false;
        RHIDeviceFeatureSupport::RayTracingTier              = ERayTracingTier::NotSupported;
        RHIDeviceFeatureSupport::RayTracingMaxRecursionDepth = 0;
    }

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading (fragment shading rate)
    // Tier2: If attachmentFragmentShadingRate (image-based) is supported
    // Tier1: If pipeline/primitive shading rate is supported
    // -------------------------------------------------------------------------------------------
    
    if (IsExtensionEnabled(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
    {
        // Query features
        VkPhysicalDeviceFeatures2 DeviceFeatures2 = {};
        DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceFragmentShadingRateFeaturesKHR DeviceFragmentShadingRateFeatures = {};
        DeviceFragmentShadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;

        AddToStructChain(DeviceFeatures2, DeviceFragmentShadingRateFeatures);

        vkGetPhysicalDeviceFeatures2(PhysicalDeviceHandle, &DeviceFeatures2);

        // Query properties (tile size)
        VkPhysicalDeviceProperties2 DeviceProperties2 = {};
        DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceFragmentShadingRatePropertiesKHR DeviceFragmentShadingRateProperties = {};
        DeviceFragmentShadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;

        AddToStructChain(DeviceProperties2, DeviceFragmentShadingRateProperties);

        vkGetPhysicalDeviceProperties2(PhysicalDeviceHandle, &DeviceProperties2);

        if (DeviceFragmentShadingRateFeatures.attachmentFragmentShadingRate)
        {
            RHIDeviceFeatureSupport::ShadingRateTier = EShadingRateTier::Tier2; // image-based
        }
        else if (DeviceFragmentShadingRateFeatures.pipelineFragmentShadingRate || DeviceFragmentShadingRateFeatures.primitiveFragmentShadingRate)
        {
            RHIDeviceFeatureSupport::ShadingRateTier = EShadingRateTier::Tier1; // per-draw/per-primitive
        }
        else
        {
            RHIDeviceFeatureSupport::ShadingRateTier = EShadingRateTier::NotSupported;
        }

        RHIDeviceFeatureSupport::ShadingRateImageTileSize = Math::Max<uint32>(1u, DeviceFragmentShadingRateProperties.minFragmentShadingRateAttachmentTexelSize.width);
        RHIDeviceFeatureSupport::bSupportsVRS             = RHIDeviceFeatureSupport::ShadingRateTier != EShadingRateTier::NotSupported;
    }
    else
    {
        RHIDeviceFeatureSupport::bSupportsVRS             = false;
        RHIDeviceFeatureSupport::ShadingRateTier          = EShadingRateTier::NotSupported;
        RHIDeviceFeatureSupport::ShadingRateImageTileSize = 0;
    }

    return true;
}

// -------------------------------------------------------------------------------------------
// Initialize default resources that are just for null bindings
// -------------------------------------------------------------------------------------------

bool FVulkanDevice::InitializeDefaultResources(FVulkanCommandContext& CommandContext)
{
    // Create the resources
    if (!DefaultResources.Initialize(*this))
    {
        VULKAN_ERROR_CRITICAL("Failed to create DefaultResources");
        return false;
    }

    CommandContext.ObtainCommandBuffer();

    if (VULKAN_CHECK_HANDLE(DefaultResources.NullBuffer))
    {
        CommandContext.GetCommandBuffer()->FillBuffer(DefaultResources.NullBuffer, 0, VULKAN_DEFAULT_BUFFER_NUM_BYTES, 0);
    }

    if (GVulkanSupportsNullDescriptors)
    {
        return true;
    }

    VkBuffer DefaultBuffer = DefaultResources.NullBuffer;
    VkImage  DefaultImage  = DefaultResources.NullImage;

    VkImageMemoryBarrier2 ImageBarrier = {};
    ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    ImageBarrier.image                           = DefaultImage;
    ImageBarrier.srcAccessMask                   = VK_ACCESS_2_NONE;
    ImageBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(VK_FORMAT_R8G8B8A8_UNORM);
    ImageBarrier.subresourceRange.baseArrayLayer = 0;
    ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageBarrier.subresourceRange.baseMipLevel   = 0;
    ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    CommandContext.GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);

    VkBufferImageCopy BufferImageCopy = {};
    BufferImageCopy.bufferOffset                    = 0;
    BufferImageCopy.bufferRowLength                 = 0;
    BufferImageCopy.bufferImageHeight               = 0;
    BufferImageCopy.imageSubresource.aspectMask     = ImageBarrier.subresourceRange.aspectMask;
    BufferImageCopy.imageSubresource.mipLevel       = 0;
    BufferImageCopy.imageSubresource.baseArrayLayer = 0;
    BufferImageCopy.imageSubresource.layerCount     = 1;
    BufferImageCopy.imageOffset                     = { 0, 0, 0 };
    BufferImageCopy.imageExtent                     = { VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 1 };

    CommandContext.GetBarrierBatcher().FlushBarriers(CommandContext.GetCommandBuffer());
    CommandContext.GetCommandBuffer()->CopyBufferToImage(DefaultBuffer, DefaultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);

    ImageBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImageBarrier.newLayout     = VK_IMAGE_LAYOUT_GENERAL;
    ImageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    ImageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    CommandContext.GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);
    CommandContext.FinishCommandBuffer(true);
    return true;
}

bool FVulkanDevice::FindOrCreateSampler(const VkSamplerCreateInfo& SamplerCreateInfo, VkSampler& OutSampler)
{
    CHECK(SamplerCreateInfo.pNext == nullptr);

    TScopedLock Lock(SamplerMapCS);

    FVulkanHashableSamplerCreateInfo HashableCreateInfo;
    HashableCreateInfo.Flags                   = SamplerCreateInfo.flags;
    HashableCreateInfo.MagFilter               = SamplerCreateInfo.magFilter;
    HashableCreateInfo.MinFilter               = SamplerCreateInfo.minFilter;
    HashableCreateInfo.MipmapMode              = SamplerCreateInfo.mipmapMode;
    HashableCreateInfo.AddressModeU            = SamplerCreateInfo.addressModeU;
    HashableCreateInfo.AddressModeV            = SamplerCreateInfo.addressModeV;
    HashableCreateInfo.AddressModeW            = SamplerCreateInfo.addressModeW;
    HashableCreateInfo.MipLodBias              = SamplerCreateInfo.mipLodBias;
    HashableCreateInfo.AnisotropyEnable        = SamplerCreateInfo.anisotropyEnable;
    HashableCreateInfo.MaxAnisotropy           = SamplerCreateInfo.maxAnisotropy;
    HashableCreateInfo.CompareEnable           = SamplerCreateInfo.compareEnable;
    HashableCreateInfo.CompareOp               = SamplerCreateInfo.compareOp;
    HashableCreateInfo.MinLod                  = SamplerCreateInfo.minLod;
    HashableCreateInfo.MaxLod                  = SamplerCreateInfo.maxLod;
    HashableCreateInfo.BorderColor             = SamplerCreateInfo.borderColor;
    HashableCreateInfo.UnnormalizedCoordinates = SamplerCreateInfo.unnormalizedCoordinates;

    if (VkSampler* Sampler = SamplerMap.Find(HashableCreateInfo))
    {
        OutSampler = *Sampler;
        return true;
    }

    VkResult Result = vkCreateSampler(GetVkDevice(), &SamplerCreateInfo, nullptr, &OutSampler);
    if (VULKAN_FAILED(Result))
    {
        OutSampler = VK_NULL_HANDLE;
        VULKAN_ERROR_CRITICAL("Failed to create sampler");
        return false;
    }
    else
    {
        const FString DebugName = FString::CreateFormatted("Sampler %d", SamplerMap.Size());
        VulkanSetObjectName(GetVkDevice(), DebugName.Data(), OutSampler, VK_OBJECT_TYPE_SAMPLER);
    }

    SamplerMap.Add(HashableCreateInfo, OutSampler);
    return true;
}

FVulkanQueryPoolManager* FVulkanDevice::GetQueryPoolManager(EQueryType QueryType)
{
    if (QueryType == EQueryType::Timestamp)
    {
        CHECK(TimingQueryPoolManager != nullptr);
        return TimingQueryPoolManager;
    }
    else if (QueryType == EQueryType::Occlusion)
    {
        CHECK(OcclusionQueryPoolManager != nullptr);
        return OcclusionQueryPoolManager;
    }
    else
    {
        DEBUG_BREAK();
        return nullptr;
    }
}

uint32 FVulkanDevice::GetQueueIndexFromType(EVulkanCommandQueueType Type) const
{
    if (Type == EVulkanCommandQueueType::Graphics)
    {
        return QueueIndicies->GraphicsQueueIndex;
    }
    else if (Type == EVulkanCommandQueueType::Compute)
    {
        return QueueIndicies->ComputeQueueIndex;
    }
    else if (Type == EVulkanCommandQueueType::Copy)
    {
        return QueueIndicies->CopyQueueIndex;
    }
    else if (Type == EVulkanCommandQueueType::Present)
    {
        return QueueIndicies->PresentQueueIndex;
    }
    else
    {
        VULKAN_ERROR_CRITICAL("Invalid CommandQueueType");
        return (~0U);
    }
}

bool FVulkanDevice::InitializePresentQueueFamily(VkSurfaceKHR Surface)
{
    CHECK(QueueIndicies.HasValue());

    VkPhysicalDevice GPU = PhysicalDevice->GetVkPhysicalDevice();

    uint32 QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(GPU, &QueueFamilyCount, nullptr);

    // Prefer the graphics family if it also supports present (common case)
    VkBool32 GraphicsSupportsPresent = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(GPU, QueueIndicies->GraphicsQueueIndex, Surface, &GraphicsSupportsPresent);
    if (GraphicsSupportsPresent)
    {
        QueueIndicies->PresentQueueIndex = QueueIndicies->GraphicsQueueIndex;
        return true;
    }

    // Graphics queue doesn't support present - find another family that does,
    // preferring one we already have a queue for (compute or copy)
    const uint32 PreferredFamilies[] = { QueueIndicies->ComputeQueueIndex, QueueIndicies->CopyQueueIndex };
    for (uint32 FamilyIndex : PreferredFamilies)
    {
        VkBool32 Supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(GPU, FamilyIndex, Surface, &Supported);
        if (Supported)
        {
            QueueIndicies->PresentQueueIndex = FamilyIndex;
            VULKAN_INFO("Using queue family %u (shared with existing queue) for present", FamilyIndex);
            return true;
        }
    }

    // Fall back to first family that supports present
    for (uint32 i = 0; i < QueueFamilyCount; i++)
    {
        VkBool32 Supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(GPU, i, Surface, &Supported);
        if (Supported)
        {
            QueueIndicies->PresentQueueIndex = i;
            VULKAN_WARNING("Present queue family %u differs from graphics (%u) and is not a pre-existing queue family", i, QueueIndicies->GraphicsQueueIndex);
            return true;
        }
    }

    VULKAN_ERROR_CRITICAL("No queue family supports presentation for this surface");
    return false;
}

bool FVulkanDefaultResources::Initialize(FVulkanDevice& Device)
{
    if (!GVulkanSupportsNullDescriptors)
    {
        if (!InitializeNullBufferAndImage(Device))
        {
            return false;
        }
    }
#if VULKAN_ENABLE_DYNAMIC_UNIFORM_BUFFERS
    else
    {
        if (!InitializeNullBuffer(Device))
        {
            return false;
        }
    }
#endif

    // Create a NullSampler
    VkSamplerCreateInfo SamplerCreateInfo = {};
    SamplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.magFilter               = VK_FILTER_LINEAR;
    SamplerCreateInfo.minFilter               = VK_FILTER_LINEAR;
    SamplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerCreateInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerCreateInfo.mipLodBias              = 0.0f;
    SamplerCreateInfo.anisotropyEnable        = VK_FALSE;
    SamplerCreateInfo.maxAnisotropy           = 1.0f;
    SamplerCreateInfo.compareEnable           = VK_FALSE;
    SamplerCreateInfo.compareOp               = VK_COMPARE_OP_NEVER;
    SamplerCreateInfo.minLod                  = 0.0f;
    SamplerCreateInfo.maxLod                  = VK_LOD_CLAMP_NONE;
    SamplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    SamplerCreateInfo.unnormalizedCoordinates = false;

    if (!Device.FindOrCreateSampler(SamplerCreateInfo, NullSampler))
    {
        VULKAN_ERROR_CRITICAL("vkCreateSampler failed");
        return false;
    }
    else
    {
        VulkanSetObjectName(Device.GetVkDevice(), "NullSampler", NullSampler, VK_OBJECT_TYPE_SAMPLER);
    }

    return true;
}

bool FVulkanDefaultResources::InitializeNullBuffer(FVulkanDevice& Device)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                 = nullptr;
    BufferCreateInfo.flags                 = 0;
    BufferCreateInfo.pQueueFamilyIndices   = nullptr;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                  = VULKAN_DEFAULT_BUFFER_NUM_BYTES;
	BufferCreateInfo.usage =
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VkResult Result = vkCreateBuffer(Device.GetVkDevice(), &BufferCreateInfo, nullptr, &NullBuffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Buffer");
        return false;
    }
    else
    {
        VulkanSetObjectName(Device.GetVkDevice(), "NullBuffer", NullBuffer, VK_OBJECT_TYPE_BUFFER);
    }

    {
        FVulkanMemoryStorage TempStorage(&Device);
        NullBufferStorage.Swap(TempStorage);
    }

    FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, BufferCreateInfo.usage, 0, BufferCreateInfo.size, 256, NullBufferStorage))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate buffer memory");
        return false;
    }

    VkResult BindResult = vkBindBufferMemory(Device.GetVkDevice(), NullBuffer, NullBufferStorage.GetMemory(), NullBufferStorage.GetMemoryOffset());
    if (VULKAN_FAILED(BindResult))
    {
        VULKAN_ERROR_CRITICAL("Failed to bind NullBuffer memory");
        return false;
    }

    return true;
}

bool FVulkanDefaultResources::InitializeNullBufferAndImage(FVulkanDevice& Device)
{
    if (!InitializeNullBuffer(Device))
    {
        return false;
    }

    FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();

    // Create a NullImage
    constexpr VkExtent3D NullExtent = 
    { 
        VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 
        VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT,
        1 
    };

    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType             = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ImageCreateInfo.format                = VK_FORMAT_R8G8B8A8_UNORM;
    ImageCreateInfo.extent                = NullExtent;
    ImageCreateInfo.mipLevels             = 1;
    ImageCreateInfo.pQueueFamilyIndices   = nullptr;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples               = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.arrayLayers           = 1;

    VkResult Result = vkCreateImage(Device.GetVkDevice(), &ImageCreateInfo, nullptr, &NullImage);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create image");
        return false;
    }
    else
    {
        VulkanSetObjectName(Device.GetVkDevice(), "NullImage", NullImage, VK_OBJECT_TYPE_IMAGE);
    }

    {
        FVulkanMemoryStorage TempStorage(&Device);
        NullImageStorage.Swap(TempStorage);
    }

    if (!MemoryManager.AllocateImageMemory(NullImage, ImageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, NullImageStorage))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate ImageMemory");
        return false;
    }

    VkResult BindImageResult = vkBindImageMemory(Device.GetVkDevice(), NullImage, NullImageStorage.GetMemory(), NullImageStorage.GetMemoryOffset());
    if (VULKAN_FAILED(BindImageResult))
    {
        VULKAN_ERROR_CRITICAL("Failed to bind NullImage memory");
        return false;
    }

    // Create NullImageView
    VkImageViewCreateInfo ImageViewCreateInfo = {};
    ImageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.flags                           = 0;
    ImageViewCreateInfo.format                          = ImageCreateInfo.format;
    ImageViewCreateInfo.image                           = NullImage;
    ImageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    ImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    ImageViewCreateInfo.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    Result = vkCreateImageView(Device.GetVkDevice(), &ImageViewCreateInfo, nullptr, &NullImageView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkCreateImageView failed");
        return false;
    }
    else
    {
        VulkanSetObjectName(Device.GetVkDevice(), "NullImageView", NullImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    return true;
}

void FVulkanDefaultResources::Release(FVulkanDevice& Device)
{
    VkDevice VulkanDevice = Device.GetVkDevice();
    if (VULKAN_CHECK_HANDLE(NullBuffer))
    {
        vkDestroyBuffer(VulkanDevice, NullBuffer, nullptr);
        NullBuffer = VK_NULL_HANDLE;
        NullBufferStorage.ReleaseMemory();
    }

    if (VULKAN_CHECK_HANDLE(NullImageView))
    {
        vkDestroyImageView(VulkanDevice, NullImageView, nullptr);
        NullImageView = VK_NULL_HANDLE;
    }

    if (VULKAN_CHECK_HANDLE(NullImage))
    {
        vkDestroyImage(VulkanDevice, NullImage, nullptr);
        NullImage = VK_NULL_HANDLE;
        NullImageStorage.ReleaseMemory();
    }

    NullSampler = VK_NULL_HANDLE;
}
