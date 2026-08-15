#include "Core/Containers/Array.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/CString.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"
#include "RHI/RHI.h"
#include "RHI/RHISamplerState.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanInstance.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanExtensions.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/Platform/VulkanPlatform.h"
#include "VulkanRHI/Generated/ClearBufferUAV_Float.h"
#include "VulkanRHI/Generated/ClearBufferUAV_Uint.h"
#include "VulkanRHI/Generated/ClearBufferUAV_Sint.h"

template <typename FeatureStructType>
static bool CheckRequiredFeaturesHelper(const FeatureStructType& Required, const FeatureStructType& Available, const char* StructName)
{
    UNREFERENCED_VARIABLE(StructName);

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

static String GetQueuePropertiesAsString(const VkQueueFamilyProperties& Properties)
{
    String PropertyString = "QueueCount=" + TTypeToString<int32>::ToString(Properties.queueCount) + ", QueueBits=(";
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

static bool CreateNullBuffer(FVulkanDevice& Device, VkBufferUsageFlags Usage, const CHAR* BufferDebugName, const CHAR* BufferViewDebugName, VkBuffer& OutBuffer, VkBufferView& OutBufferView, FVulkanMemoryLocation& OutLocation)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                 = nullptr;
    BufferCreateInfo.flags                 = 0;
    BufferCreateInfo.pQueueFamilyIndices   = nullptr;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                  = VULKAN_DEFAULT_BUFFER_NUM_BYTES;
    BufferCreateInfo.usage                 = Usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkResult Result = vkCreateBuffer(Device.GetVkDevice(), &BufferCreateInfo, nullptr, &OutBuffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Buffer");
        return false;
    }

    VulkanSetObjectName(Device.GetVkDevice(), BufferDebugName, OutBuffer, VK_OBJECT_TYPE_BUFFER);

    {
        FVulkanMemoryLocation TempLocation(&Device);
        OutLocation.Swap(TempLocation);
    }

    FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, BufferCreateInfo.usage, 0, BufferCreateInfo.size, 256, OutLocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate buffer memory");
        return false;
    }

    VkResult BindResult = vkBindBufferMemory(Device.GetVkDevice(), OutBuffer, OutLocation.GetMemory(), OutLocation.GetMemoryOffset());
    if (VULKAN_FAILED(BindResult))
    {
        VULKAN_ERROR_CRITICAL("Failed to bind NullBuffer memory");
        return false;
    }

    VkBufferViewCreateInfo BufferViewCreateInfo = {};
    BufferViewCreateInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    BufferViewCreateInfo.buffer = OutBuffer;
    BufferViewCreateInfo.format = VK_FORMAT_R32_UINT;
    BufferViewCreateInfo.offset = 0;
    BufferViewCreateInfo.range  = VK_WHOLE_SIZE;

    Result = vkCreateBufferView(Device.GetVkDevice(), &BufferViewCreateInfo, nullptr, &OutBufferView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create NullBufferView");
        return false;
    }

    VulkanSetObjectName(Device.GetVkDevice(), BufferViewDebugName, OutBufferView, VK_OBJECT_TYPE_BUFFER_VIEW);
    return true;
}

static bool CreateNullImageViews(FVulkanDevice& Device, VkImage Image, VkImageView OutViews[static_cast<uint32>(EVulkanNullImageViewType::Count)])
{
    struct FNullViewDesc
    {
        EVulkanNullImageViewType ViewType;
        VkImageViewType          VkViewType;
        uint32                   LayerCount;
        const CHAR*              DebugName;
    };

    const FNullViewDesc NullViewDescs[] =
    {
        { EVulkanNullImageViewType::Texture2D,        VK_IMAGE_VIEW_TYPE_2D,         1,                                 "NullImageView2D"        },
        { EVulkanNullImageViewType::Texture2DArray,   VK_IMAGE_VIEW_TYPE_2D_ARRAY,   VULKAN_DEFAULT_IMAGE_ARRAY_LAYERS, "NullImageView2DArray"   },
        { EVulkanNullImageViewType::TextureCube,      VK_IMAGE_VIEW_TYPE_CUBE,       VULKAN_DEFAULT_IMAGE_ARRAY_LAYERS, "NullImageViewCube"      },
        { EVulkanNullImageViewType::TextureCubeArray, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, VULKAN_DEFAULT_IMAGE_ARRAY_LAYERS, "NullImageViewCubeArray" },
    };

    static_assert(ARRAY_COUNT(NullViewDescs) == static_cast<uint32>(EVulkanNullImageViewType::Count), "NullViewDescs is out of date");

    VkImageViewCreateInfo ImageViewCreateInfo = {};
    ImageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.flags                           = 0;
    ImageViewCreateInfo.format                          = VK_FORMAT_R8G8B8A8_UNORM;
    ImageViewCreateInfo.image                           = Image;
    ImageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    ImageViewCreateInfo.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

    for (const FNullViewDesc& ViewDesc : NullViewDescs)
    {
        const uint32 ViewIndex = static_cast<uint32>(ViewDesc.ViewType);
        if (ViewDesc.VkViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY && !GVulkanSupportsImageCubeArray)
        {
            OutViews[ViewIndex] = OutViews[static_cast<uint32>(EVulkanNullImageViewType::TextureCube)];
            continue;
        }

        ImageViewCreateInfo.viewType                    = ViewDesc.VkViewType;
        ImageViewCreateInfo.subresourceRange.layerCount = ViewDesc.LayerCount;

        VkResult Result = vkCreateImageView(Device.GetVkDevice(), &ImageViewCreateInfo, nullptr, &OutViews[ViewIndex]);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("vkCreateImageView failed for '%s'", ViewDesc.DebugName);
            return false;
        }

        VulkanSetObjectName(Device.GetVkDevice(), ViewDesc.DebugName, OutViews[ViewIndex], VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    return true;
}

static bool CreateNullImage(FVulkanDevice& Device, VkImageUsageFlags Usage, const CHAR* DebugName, VkImage& OutImage, FVulkanMemoryLocation& OutLocation, VkImageView OutViews[static_cast<uint32>(EVulkanNullImageViewType::Count)])
{
    constexpr VkExtent3D NullExtent =
    {
        VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT,
        VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT,
        1
    };

    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.flags                 = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    ImageCreateInfo.imageType             = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.usage                 = Usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ImageCreateInfo.format                = VK_FORMAT_R8G8B8A8_UNORM;
    ImageCreateInfo.extent                = NullExtent;
    ImageCreateInfo.mipLevels             = 1;
    ImageCreateInfo.pQueueFamilyIndices   = nullptr;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.samples               = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.arrayLayers           = VULKAN_DEFAULT_IMAGE_ARRAY_LAYERS;

    VkResult Result = vkCreateImage(Device.GetVkDevice(), &ImageCreateInfo, nullptr, &OutImage);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create image");
        return false;
    }

    VulkanSetObjectName(Device.GetVkDevice(), DebugName, OutImage, VK_OBJECT_TYPE_IMAGE);

    {
        FVulkanMemoryLocation TempLocation(&Device);
        OutLocation.Swap(TempLocation);
    }

    FVulkanMemoryManager& MemoryManager = Device.GetMemoryManager();
    if (!MemoryManager.AllocateImageMemory(OutImage, ImageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, OutLocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate ImageMemory");
        return false;
    }

    Result = vkBindImageMemory(Device.GetVkDevice(), OutImage, OutLocation.GetMemory(), OutLocation.GetMemoryOffset());
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to bind NullImage memory");
        return false;
    }

    return CreateNullImageViews(Device, OutImage, OutViews);
}

FVulkanCoreFeatures::FVulkanCoreFeatures()
{
    Memory::Memzero(this);
}

void FVulkanCoreFeatures::BuildQueryChain(VkPhysicalDeviceFeatures2& Root)
{
    Features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    AddAllToStructChain(Root, Features11, Features12);
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

    return true;
}

void FVulkanCoreFeatures::EnableAvailable(FVulkanCoreFeatures& OutEnabled, const FVulkanCoreFeatures& Available) const
{
    EnableAvailableFeaturesHelper(OutEnabled.Features10, Features10, Available.Features10);
    EnableAvailableFeaturesHelper(OutEnabled.Features11, Features11, Available.Features11);
    EnableAvailableFeaturesHelper(OutEnabled.Features12, Features12, Available.Features12);
}

void FVulkanCoreFeatures::BuildEnableChain(VkPhysicalDeviceFeatures2& Root)
{
    Features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    AddAllToStructChain(Root, Features11, Features12);
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

        if (AdapterProperties.apiVersion < VULKAN_TARGET_API_VERSION)
        {
            VULKAN_INFO("Skipping device '%s' since it's api-version is below Vulkan 1.2 (apiVersion=%s)", AdapterProperties.deviceName, *GetVersionAsString(AdapterProperties.apiVersion));
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
                if (CString::Strcmp(Extension->GetExtensionName(), Property.extensionName) == 0)
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
    Memory::Memzero(&DeviceProperties2);
    DeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

#if VK_KHR_ray_tracing_pipeline
    Memory::Memzero(&RayTracingPipelineProperties);
    RayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    bool bAdapterSupportsRayTracingPipeline = false;
    {
        uint32 DeviceExtensionCount = 0;
        if (!VULKAN_FAILED(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionCount, nullptr)) && DeviceExtensionCount > 0)
        {
            TArray<VkExtensionProperties> AvailableDeviceExtensions(DeviceExtensionCount);
            if (!VULKAN_FAILED(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionCount, AvailableDeviceExtensions.Data())))
            {
                for (const VkExtensionProperties& Property : AvailableDeviceExtensions)
                {
                    if (CString::Strcmp(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, Property.extensionName) == 0)
                    {
                        bAdapterSupportsRayTracingPipeline = true;
                        break;
                    }
                }
            }
        }
    }

    if (bAdapterSupportsRayTracingPipeline)
    {
        AddToStructChain(DeviceProperties2, RayTracingPipelineProperties);
    }
#endif

    vkGetPhysicalDeviceProperties2(PhysicalDevice, &DeviceProperties2);

    // Get Physical Device Feature (For Vulkan 1.1 and Vulkan 1.2 and extensions)
    Memory::Memzero(&DeviceFeatures2);
    DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    // Vulkan 1.1 features
    Memory::Memzero(&DeviceFeatures11);
    DeviceFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

    // Vulkan 1.2 features
    Memory::Memzero(&DeviceFeatures12);
    DeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    
    // Helper for checking for extensions
    AddAllToStructChain(DeviceFeatures2, DeviceFeatures11, DeviceFeatures12);

    // Get the physical device features
    vkGetPhysicalDeviceFeatures2(PhysicalDevice, &DeviceFeatures2);

    // Get Physical Device Memory Properties
    Memory::Memzero(&DeviceMemoryProperties2);
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
            const String PropertyString = GetQueuePropertiesAsString(Properties);
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
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    , RenderPassCache(nullptr)
#endif
    , MemoryManager(nullptr)
    , FenceManager(nullptr)
    , FrameFence(nullptr)
    , PipelineLayoutManager(nullptr)
    , PipelineStateManager(nullptr)
#if VULKAN_USE_DESCRIPTOR_CACHE
    , DescriptorSetCache(nullptr)
#else
    , DescriptorPoolManager(nullptr)
#endif
    , TimingQueryPoolManager(nullptr)
    , OcclusionQueryPoolManager(nullptr)
    , PipelineStatsQueryPoolManager(nullptr)
    , GraphicsQueue(nullptr)
    , PresentQueue(nullptr)
#if VULKAN_ENABLE_CRASH_MARKERS
    , bSupportsAMDBufferMarker(false)
    , bSupportsNVDiagnosticCheckpoints(false)
#endif
{
#if VULKAN_USE_DESCRIPTOR_CACHE
    DescriptorSetCache            = new FVulkanDescriptorSetCache(this);
#else
    DescriptorPoolManager         = new FVulkanDescriptorPoolManager(this);
#endif
    BindlessDescriptorManager     = new FVulkanBindlessDescriptorManager(this);
    TimingQueryPoolManager        = new FVulkanQueryPoolManager(this, VK_QUERY_TYPE_TIMESTAMP, VULKAN_DEFAULT_QUERY_COUNT);
    OcclusionQueryPoolManager     = new FVulkanQueryPoolManager(this, VK_QUERY_TYPE_OCCLUSION, VULKAN_DEFAULT_QUERY_COUNT);
    PipelineStatsQueryPoolManager = new FVulkanQueryPoolManager(this, VK_QUERY_TYPE_PIPELINE_STATISTICS, VULKAN_DEFAULT_QUERY_COUNT);
    PipelineLayoutManager         = new FVulkanPipelineLayoutManager(this);
    FenceManager                  = new FVulkanFenceManager(this);
#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    RenderPassCache               = new FVulkanRenderPassCache(this);
#endif
}

FVulkanDevice::~FVulkanDevice()
{
    if (PresentQueue != GraphicsQueue)
    {
        SAFE_DELETE(PresentQueue);
    }
    else
    {
        PresentQueue = nullptr;
    }

    SAFE_DELETE(GraphicsQueue);

    // Release default resources
    BufferClearPipelines.Release();
    DefaultResources.Release(*this);

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

    FVulkanDeviceRHI::FlushDeferredDeletions();

    if (PipelineStateManager)
    {
        PipelineStateManager->SaveCacheData();
        delete PipelineStateManager;
    }

    SAFE_DELETE(BindlessDescriptorManager);

#if VULKAN_USE_DESCRIPTOR_CACHE
    SAFE_DELETE(DescriptorSetCache);
#else
    SAFE_DELETE(DescriptorPoolManager);
#endif

    SAFE_DELETE(TimingQueryPoolManager);
    SAFE_DELETE(OcclusionQueryPoolManager);
    SAFE_DELETE(PipelineStatsQueryPoolManager);
    SAFE_DELETE(PipelineLayoutManager);

#if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    SAFE_DELETE(RenderPassCache);
#endif

    SAFE_DELETE(FrameFence);
    SAFE_DELETE(FenceManager);
    SAFE_DELETE(MemoryManager);

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
            if (CString::Strcmp(Extension->GetExtensionName(), Property.extensionName) == 0)
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

    VkPhysicalDeviceSubgroupProperties AvailableDeviceSubgroupProperties = {};
    AvailableDeviceSubgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;

    VkPhysicalDeviceVulkan12Properties AvailableDeviceVulkan12Properties = {};
    AvailableDeviceVulkan12Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;

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
        AddAllToStructChain(AvailableDeviceProperties2, AvailableDeviceMultiviewProperties, AvailableDeviceSubgroupProperties, AvailableDeviceVulkan12Properties);

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

    DeriveCoreCapabilities(InDeviceCreateInfo, AvailableFeatures, PhysicalDevice->GetProperties(), AvailableDeviceMultiviewProperties, AvailableDeviceSubgroupProperties, AvailableDeviceVulkan12Properties);

#if !VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
    if (!GVulkanSupportsDynamicRendering)
    {
        VULKAN_ERROR_CRITICAL("Device does not support VK_KHR_dynamic_rendering, which is required");
        return false;
    }
#endif

    if (!GVulkanSupportsSynchronization2)
    {
        VULKAN_ERROR_CRITICAL("Device does not support VK_KHR_synchronization2, which is required");
        return false;
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
                if (CString::Strcmp(RequiredLayer, LayerProp.layerName) == 0)
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
                if (CString::Strcmp(OptionalLayer, LayerProp.layerName) == 0)
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
    
    DeriveEnabledFeatureCapabilities(EnabledFeatures);

    if ((GVulkanSupportsDepthClip || GVulkanSupportsDepthClamp) && AvailableFeatures.Features10.depthClamp)
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

    if (BindlessDescriptorManager && GVulkanSupportsBindless)
    {
        if (!BindlessDescriptorManager->Initialize())
        {
            VULKAN_WARNING("FVulkanDevice: Failed to initialize BindlessDescriptorManager; bindless will be disabled");
            GVulkanSupportsBindless = false;
        }
    }

    RHI::bSupportsBindless = BindlessDescriptorManager && BindlessDescriptorManager->IsEnabled();
    return true;
}

// -------------------------------------------------------------------------------------------
// Initialize default resources that are just for null bindings
// -------------------------------------------------------------------------------------------

bool FVulkanDevice::InitializeDefaultResources(FVulkanCommandContext& CommandContext)
{
    if (!DefaultResources.Initialize(*this))
    {
        VULKAN_ERROR_CRITICAL("Failed to create DefaultResources");
        return false;
    }

    CommandContext.ObtainCommandBuffer();

    if (VULKAN_CHECK_HANDLE(DefaultResources.NullReadBuffer))
    {
        CommandContext.GetCommandBuffer()->FillBuffer(DefaultResources.NullReadBuffer, 0, VULKAN_DEFAULT_BUFFER_NUM_BYTES, 0);
    }

    if (VULKAN_CHECK_HANDLE(DefaultResources.NullWriteBuffer))
    {
        CommandContext.GetCommandBuffer()->FillBuffer(DefaultResources.NullWriteBuffer, 0, VULKAN_DEFAULT_BUFFER_NUM_BYTES, 0);
    }

    if (GVulkanSupportsNullDescriptors)
    {
        CommandContext.FinishCommandBuffer(true);
        return true;
    }

    const auto InitializeNullImageContent = [&](VkBuffer SrcBuffer, VkImage DstImage, VkImageLayout FinalLayout, VkAccessFlags2 FinalAccess)
    {
        VkImageMemoryBarrier2KHR ImageBarrier = {};
        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
        ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = DstImage;
        ImageBarrier.srcAccessMask                   = VK_ACCESS_2_NONE_KHR;
        ImageBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
        ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        ImageBarrier.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(VK_FORMAT_R8G8B8A8_UNORM);
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        CommandContext.GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);

        VkBufferMemoryBarrier2KHR BufferBarrier = {};
        BufferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
        BufferBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_CLEAR_BIT_KHR;
        BufferBarrier.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
        BufferBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
        BufferBarrier.dstAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT_KHR;
        BufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        BufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        BufferBarrier.buffer              = SrcBuffer;
        BufferBarrier.offset              = 0;
        BufferBarrier.size                = VK_WHOLE_SIZE;

        CommandContext.GetBarrierBatcher().AddBufferMemoryBarrier(0, BufferBarrier);

        VkBufferImageCopy BufferImageCopy = {};
        BufferImageCopy.bufferOffset                    = 0;
        BufferImageCopy.bufferRowLength                 = 0;
        BufferImageCopy.bufferImageHeight               = 0;
        BufferImageCopy.imageSubresource.aspectMask     = ImageBarrier.subresourceRange.aspectMask;
        BufferImageCopy.imageSubresource.mipLevel       = 0;
        BufferImageCopy.imageSubresource.baseArrayLayer = 0;
        BufferImageCopy.imageSubresource.layerCount     = VULKAN_DEFAULT_IMAGE_ARRAY_LAYERS;
        BufferImageCopy.imageOffset                     = { 0, 0, 0 };
        BufferImageCopy.imageExtent                     = { VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, VULKAN_DEFAULT_IMAGE_WIDTH_AND_HEIGHT, 1 };

        CommandContext.GetBarrierBatcher().FlushBarriers(CommandContext.GetCommandBuffer());
        CommandContext.GetCommandBuffer()->CopyBufferToImage(SrcBuffer, DstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);

        ImageBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ImageBarrier.newLayout     = FinalLayout;
        ImageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
        ImageBarrier.dstAccessMask = FinalAccess;
        ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        CommandContext.GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);
    };

    InitializeNullImageContent(DefaultResources.NullReadBuffer, DefaultResources.NullReadImage, 
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_2_SHADER_READ_BIT_KHR);

    InitializeNullImageContent(DefaultResources.NullWriteBuffer, DefaultResources.NullWriteImage, 
        VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR);

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
        const String DebugName = String::Printf("Sampler %d", SamplerMap.Size());
        VulkanSetObjectName(GetVkDevice(), DebugName.Data(), OutSampler, VK_OBJECT_TYPE_SAMPLER);
    }

    SamplerMap.Add(HashableCreateInfo, OutSampler);
    return true;
}

bool FVulkanDevice::FindOrCreateSampler(const FRHISamplerStateDesc& SamplerDesc, VkSampler& OutSampler)
{
    VkSamplerCreateInfo CreateInfo = {};
    CreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    CreateInfo.magFilter               = ConvertSamplerFilterToMagFilter(SamplerDesc.Filter);
    CreateInfo.minFilter               = ConvertSamplerFilterToMinFilter(SamplerDesc.Filter);
    CreateInfo.mipmapMode              = ConvertSamplerFilterToMipmapMode(SamplerDesc.Filter);
    CreateInfo.addressModeU            = ConvertSamplerMode(SamplerDesc.AddressU);
    CreateInfo.addressModeV            = ConvertSamplerMode(SamplerDesc.AddressV);
    CreateInfo.addressModeW            = ConvertSamplerMode(SamplerDesc.AddressW);
    CreateInfo.mipLodBias              = SamplerDesc.MipLODBias;
    CreateInfo.anisotropyEnable        = IsAnisotropySampler(SamplerDesc.Filter);
    CreateInfo.maxAnisotropy           = SamplerDesc.MaxAnisotropy;
    CreateInfo.compareEnable           = IsComparisonSampler(SamplerDesc.Filter);
    CreateInfo.compareOp               = ConvertComparisonFunc(SamplerDesc.ComparisonFunc);
    CreateInfo.minLod                  = SamplerDesc.MinLOD;
    CreateInfo.maxLod                  = SamplerDesc.MaxLOD;
    CreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    CreateInfo.unnormalizedCoordinates = false;

    if (!CreateInfo.anisotropyEnable)
    {
        CreateInfo.maxAnisotropy = 1.0f;
    }
    else
    {
        CreateInfo.maxAnisotropy = Math::Max(1.0f, CreateInfo.maxAnisotropy);
    }

    if (CreateInfo.maxLod < CreateInfo.minLod)
    {
        Math::Swap(CreateInfo.minLod, CreateInfo.maxLod);
    }

    return FindOrCreateSampler(CreateInfo, OutSampler);
}

bool FVulkanDevice::FindOrCreateSampler(const FRHIStaticSamplerInfo& StaticSamplerInfo, VkSampler& OutSampler)
{
    return FindOrCreateSampler(StaticSamplerInfo.GetSamplerStateDesc(), OutSampler);
}

FVulkanQueryPoolManager* FVulkanDevice::GetQueryPoolManager(VkQueryType QueryType)
{
    switch (QueryType)
    {
    case VK_QUERY_TYPE_TIMESTAMP:
        CHECK(TimingQueryPoolManager != nullptr);
        return TimingQueryPoolManager;

    case VK_QUERY_TYPE_OCCLUSION:
        CHECK(OcclusionQueryPoolManager != nullptr);
        return OcclusionQueryPoolManager;

    case VK_QUERY_TYPE_PIPELINE_STATISTICS:
        CHECK(PipelineStatsQueryPoolManager != nullptr);
        return PipelineStatsQueryPoolManager;

    default:
        DEBUG_BREAK();
        return nullptr;
    }
}

FVulkanQueryPool* FVulkanDevice::ObtainQueryPool(VkQueryType QueryType)
{
    FVulkanQueryPoolManager* Manager = GetQueryPoolManager(QueryType);
    CHECK(Manager != nullptr);
    return Manager->ObtainPool();
}

void FVulkanDevice::RecycleQueryPool(FVulkanQueryPool* Pool)
{
    CHECK(Pool != nullptr);
    FVulkanQueryPoolManager* Manager = GetQueryPoolManager(Pool->QueryType);
    CHECK(Manager != nullptr);
    Manager->RecyclePool(Pool);
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

bool FVulkanDevice::CreateGraphicsQueue()
{
    CHECK(GraphicsQueue == nullptr);

    GraphicsQueue = new FVulkanQueue(this, EVulkanCommandQueueType::Graphics);
    if (!GraphicsQueue->Initialize())
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize VulkanQueue [Graphics]");
        SAFE_DELETE(GraphicsQueue);
        return false;
    }

    GraphicsQueue->SetDebugName("Graphics Queue");
    return true;
}

bool FVulkanDevice::EnsurePresentQueue()
{
    if (PresentQueue)
    {
        return true;
    }

    if (!QueueIndicies || QueueIndicies->PresentQueueIndex == uint32(~0))
    {
        return false;
    }

    // The present queue commonly aliases the graphics queue when they share a family
    if (!QueueIndicies->HasSeparatePresentQueue())
    {
        PresentQueue = GraphicsQueue;
        return true;
    }

    PresentQueue = new FVulkanQueue(this, EVulkanCommandQueueType::Present);
    if (!PresentQueue->Initialize())
    {
        VULKAN_ERROR_CRITICAL("Failed to initialize present queue");
        SAFE_DELETE(PresentQueue);
        return false;
    }

    PresentQueue->SetDebugName("Present Queue");
    VULKAN_INFO("Created separate present queue (family=%u)", QueueIndicies->PresentQueueIndex);
    return true;
}

FVulkanQueue* FVulkanDevice::GetQueue(EVulkanCommandQueueType Type) const
{
    switch (Type)
    {
        case EVulkanCommandQueueType::Graphics: return GraphicsQueue;
        case EVulkanCommandQueueType::Present:  return PresentQueue;
        // Compute/Copy queues are not created as separate objects in this backend yet.
        default:                                return nullptr;
    }
}

void FVulkanDevice::WaitForGPU()
{
    // Idle ALL queues (graphics + present + any future compute/copy).
    if (VULKAN_CHECK_HANDLE(Device))
    {
        vkDeviceWaitIdle(Device);
    }

    // Reclaim completed submissions so per-submission deferred objects are released.
    if (GraphicsQueue)
    {
        GraphicsQueue->ProcessCommandQueue();
    }
}

bool FVulkanDefaultResources::Initialize(FVulkanDevice& Device)
{
    if (!GVulkanSupportsNullDescriptors)
    {
        if (!CreateNullBuffer(Device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
                "NullReadBuffer", "NullReadBufferView", NullReadBuffer, NullReadBufferView, NullReadBufferLocation) || 
            !CreateNullBuffer(Device, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
                "NullWriteBuffer", "NullWriteBufferView", NullWriteBuffer, NullWriteBufferView, NullWriteBufferLocation) || 
            !CreateNullImage(Device, VK_IMAGE_USAGE_SAMPLED_BIT, "NullReadImage", NullReadImage, NullReadImageLocation, NullReadImageViews) || 
            !CreateNullImage(Device, VK_IMAGE_USAGE_STORAGE_BIT, "NullWriteImage", NullWriteImage, NullWriteImageLocation, NullWriteImageViews))
        {
            return false;
        }
    }
#if VULKAN_ENABLE_DYNAMIC_UNIFORM_BUFFERS
    else
    {
        if (!CreateNullBuffer(Device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
                "NullReadBuffer", "NullReadBufferView", NullReadBuffer, NullReadBufferView, NullReadBufferLocation))
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

void FVulkanDefaultResources::Release(FVulkanDevice& Device)
{
    VkDevice VulkanDevice = Device.GetVkDevice();

    auto ReleaseBuffer = [&](VkBufferView& BufferView, VkBuffer& Buffer, FVulkanMemoryLocation& Location)
        {
            if (VULKAN_CHECK_HANDLE(BufferView))
            {
                vkDestroyBufferView(VulkanDevice, BufferView, nullptr);
                BufferView = VK_NULL_HANDLE;
            }

            if (VULKAN_CHECK_HANDLE(Buffer))
            {
                vkDestroyBuffer(VulkanDevice, Buffer, nullptr);
                Buffer = VK_NULL_HANDLE;
                Location.ReleaseMemory();
            }
        };

    ReleaseBuffer(NullReadBufferView, NullReadBuffer, NullReadBufferLocation);
    ReleaseBuffer(NullWriteBufferView, NullWriteBuffer, NullWriteBufferLocation);

    auto ReleaseImageViews = [&](VkImageView Views[static_cast<uint32>(EVulkanNullImageViewType::Count)])
        {
            for (uint32 ViewIndex = 0; ViewIndex < static_cast<uint32>(EVulkanNullImageViewType::Count); ViewIndex++)
            {
                VkImageView& NullImageView = Views[ViewIndex];
                if (!VULKAN_CHECK_HANDLE(NullImageView))
                {
                    continue;
                }

                bool bIsAlias = false;
                for (uint32 PreviousIndex = 0; PreviousIndex < ViewIndex; PreviousIndex++)
                {
                    bIsAlias |= (Views[PreviousIndex] == NullImageView);
                }

                if (!bIsAlias)
                {
                    vkDestroyImageView(VulkanDevice, NullImageView, nullptr);
                }

                NullImageView = VK_NULL_HANDLE;
            }
        };

    ReleaseImageViews(NullReadImageViews);
    ReleaseImageViews(NullWriteImageViews);

    if (VULKAN_CHECK_HANDLE(NullReadImage))
    {
        vkDestroyImage(VulkanDevice, NullReadImage, nullptr);
        NullReadImage = VK_NULL_HANDLE;
        NullReadImageLocation.ReleaseMemory();
    }

    if (VULKAN_CHECK_HANDLE(NullWriteImage))
    {
        vkDestroyImage(VulkanDevice, NullWriteImage, nullptr);
        NullWriteImage = VK_NULL_HANDLE;
        NullWriteImageLocation.ReleaseMemory();
    }

    NullSampler = VK_NULL_HANDLE;
}

FVulkanComputePipelineStateRHI* FVulkanBufferClearPipelines::GetOrCreatePipeline(FVulkanDevice& Device, EVulkanBufferClearType ClearType)
{
    const uint32 ClearTypeIndex = static_cast<uint32>(ClearType);
    CHECK(ClearTypeIndex < NumClearTypes);

    TScopedLock Lock(CriticalSection);

    if (Pipelines[ClearTypeIndex])
    {
        return Pipelines[ClearTypeIndex].Get();
    }

    // Creation that failed once will fail again, and the clear can be called every frame.
    if (bCreationFailed[ClearTypeIndex])
    {
        return nullptr;
    }

    bCreationFailed[ClearTypeIndex] = true;

    const uint8* EmbeddedCode = nullptr;
    uint32       EmbeddedSize = 0;

    switch (ClearType)
    {
        case EVulkanBufferClearType::Uint:
        {
            EmbeddedCode = GVulkanClearBufferUAV_Uint;
            EmbeddedSize = ARRAY_COUNT(GVulkanClearBufferUAV_Uint);
            break;
        }

        case EVulkanBufferClearType::Sint:
        {
            EmbeddedCode = GVulkanClearBufferUAV_Sint;
            EmbeddedSize = ARRAY_COUNT(GVulkanClearBufferUAV_Sint);
            break;
        }

        default:
        {
            EmbeddedCode = GVulkanClearBufferUAV_Float;
            EmbeddedSize = ARRAY_COUNT(GVulkanClearBufferUAV_Float);
            break;
        }
    }

    TArray<uint8> ShaderCode(EmbeddedCode, static_cast<int32>(EmbeddedSize));

    FVulkanComputeShaderRHIRef ClearShader = new FVulkanComputeShaderRHI(&Device);
    if (!ClearShader->Initialize(ShaderCode))
    {
        VULKAN_ERROR("Failed to create the internal buffer-clear shader");
        return nullptr;
    }

    FRHIComputePipelineStateDesc PipelineDesc;
    PipelineDesc.Shader = ClearShader.Get();

    FVulkanComputePipelineStateRHIRef ClearPipeline = new FVulkanComputePipelineStateRHI(&Device);
    if (!ClearPipeline->Initialize(PipelineDesc))
    {
        VULKAN_ERROR("Failed to create the internal buffer-clear pipeline");
        return nullptr;
    }

    ClearPipeline->SetDebugName("Internal BufferUAV Clear");

    Shaders[ClearTypeIndex]         = ClearShader;
    Pipelines[ClearTypeIndex]       = ClearPipeline;
    bCreationFailed[ClearTypeIndex] = false;
    
    return Pipelines[ClearTypeIndex].Get();
}

void FVulkanBufferClearPipelines::Release()
{
    TScopedLock Lock(CriticalSection);

    for (uint32 Index = 0; Index < NumClearTypes; Index++)
    {
        Pipelines[Index].Reset();
        Shaders[Index].Reset();
    }
}
