#include "VulkanPhysicalDevice.h"
#include "VulkanLoader.h"
#include "VulkanDriverInstance.h"

#include "Core/Containers/Array.h"
#include "Core/Templates/StringUtils.h"
#include "Core/Debug/Console/ConsoleManager.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanPhysicalDevice

TSharedRef<CVulkanPhysicalDevice> CVulkanPhysicalDevice::QueryAdapter(CVulkanDriverInstance* InInstance, const SVulkanPhysicalDeviceDesc& AdapterDesc)
{
	TSharedRef<CVulkanPhysicalDevice> NewPhyscialDevice = dbg_new CVulkanPhysicalDevice(InInstance);
	if (NewPhyscialDevice && NewPhyscialDevice->Initialize(AdapterDesc))
	{
		return NewPhyscialDevice;
	}
	else
	{
		return nullptr;
	}
}

CVulkanPhysicalDevice::CVulkanPhysicalDevice(CVulkanDriverInstance* InInstance)
	: Instance(InInstance)
	, PhysicalDevice(VK_NULL_HANDLE)
{
}

CVulkanPhysicalDevice::~CVulkanPhysicalDevice()
{
}

bool CVulkanPhysicalDevice::Initialize(const SVulkanPhysicalDeviceDesc& AdapterDesc)
{
	VkResult Result = VK_SUCCESS;

	uint32 AdapterCount = 0;
	Result = vkEnumeratePhysicalDevices(Instance->GetVkInstance(), &AdapterCount, nullptr);
	VULKAN_CHECK_RESULT(Result, "Failed to retrive AdapterCount");

	TArray<VkPhysicalDevice> Adapters(AdapterCount);
	Result = vkEnumeratePhysicalDevices(Instance->GetVkInstance(), &AdapterCount, Adapters.Data());
	VULKAN_CHECK_RESULT(Result, "Failed to retrive available Adapters");

	if (AdapterCount < 1)
	{
		VULKAN_ERROR_ALWAYS("No Adapters available");
		return false;
	}

	IConsoleVariable* VerboseVulkan = CConsoleManager::Get().FindVariable("vulkan.VerboseLogging");

	const bool bVerboseLogging = VerboseVulkan && VerboseVulkan->GetBool();
	if (bVerboseLogging)
	{
		VULKAN_INFO("Available adapters:");

		for (VkPhysicalDevice CurrentAdapter : Adapters)
		{
			VkPhysicalDeviceProperties AdapterProperties;
			vkGetPhysicalDeviceProperties(CurrentAdapter, &AdapterProperties);
			
			LOG_INFO(String("    ") + AdapterProperties.deviceName + " Supports Vulkan " + GetVersionAsString(AdapterProperties.apiVersion));
		}
	}

	// GPU selection
	TArray<VkPhysicalDevice> AcceptedAdapers;
	AcceptedAdapers.Reserve(Adapters.Size());
	
	for (VkPhysicalDevice CurrentAdapter : Adapters)
	{
		VkPhysicalDeviceProperties AdapterProperties;
		vkGetPhysicalDeviceProperties(CurrentAdapter, &AdapterProperties);

		VkPhysicalDeviceFeatures AdapterFeatures;
		vkGetPhysicalDeviceFeatures(CurrentAdapter, &AdapterFeatures);

		//Check for adapter features
		constexpr uint32 NumFeatures = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

		const VkBool32* RequiredFeatures  = reinterpret_cast<const VkBool32*>(&AdapterDesc.RequiredFeatures);
		const VkBool32* AvailableFeatures = reinterpret_cast<const VkBool32*>(&AdapterFeatures);
		
		bool bHasAllFeatures = true;
		for (uint32 FeatureIndex = 0; FeatureIndex < NumFeatures; ++FeatureIndex)
		{
			const bool bRequiresFeature = (RequiredFeatures[FeatureIndex]  == VK_TRUE);
			const bool bHasFeature      = (AvailableFeatures[FeatureIndex] == VK_TRUE);
			if (bRequiresFeature && !bHasFeature)
			{
				VULKAN_WARNING(String("Adapter '") + AdapterProperties.deviceName + "' does not have all required device-features. See PhyscicalDeviceFeature[" + ToString(FeatureIndex) + "]");
				bHasAllFeatures = false;
				break;
			}
		}
		
		if (!bHasAllFeatures)
		{
			continue;
		}

		// Find indices for queue-families
		TOptional<SVulkanQueueFamilyIndices> QueueIndices = GetQueueFamilyIndices(CurrentAdapter);
		if (!QueueIndices)
		{
			VULKAN_WARNING(String("Adapter '") + AdapterProperties.deviceName + "' does not support all required QueueFamilies");
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
			VULKAN_INFO(String("The adapter'") + AdapterProperties.deviceName + "' has the following available extensions:");

			for (const VkExtensionProperties& Extension : AvailableDeviceExtensions)
			{
				LOG_INFO(String("    ") + Extension.extensionName);
			}
		}
        
		// Verify the required extensions
		bool bIsAllExtensionsSupported = true;
		for (const char* RequiredExtension : AdapterDesc.RequiredExtensionNames)
		{
			bool bIsSupported = false;
			for (const VkExtensionProperties& Extension : AvailableDeviceExtensions)
			{
				if (StringUtils::Compare(Extension.extensionName, RequiredExtension) == 0)
				{
					bIsSupported = true;
					break;
				}
			}

			if (!bIsSupported)
			{
				bIsAllExtensionsSupported = false;
				VULKAN_WARNING(String("Required Device Extension '") + RequiredExtension + "' is not supported by '" + AdapterProperties.deviceName + "'");
				break;
			}
		}

		// TODO: At this point we now the device is acceptable, now check for the most optional

        if (bIsAllExtensionsSupported)
        {
			AcceptedAdapers.Push(CurrentAdapter);
			if (AdapterProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				PhysicalDevice = CurrentAdapter;
			}
		}
	}

	// If no discrete adapter is found, select the first accepted one
	if (!VULKAN_CHECK_HANDLE(PhysicalDevice) && !AcceptedAdapers.IsEmpty())
	{
		PhysicalDevice = AcceptedAdapers[0];
	}
	
	// If there still is no physical-device, then we failed
	if (!VULKAN_CHECK_HANDLE(PhysicalDevice))
	{
		VULKAN_ERROR_ALWAYS("Failed to find a suitable Adapter");
		return false;
	}
	
	// Retrieve and cache information about the physical-device
	vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &DeviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &DeviceMemoryProperties);
	
#if VK_KHR_get_physical_device_properties2
	if (Instance->IsExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
	{
		vkGetPhysicalDeviceProperties2(PhysicalDevice, &DeviceProperties2);
		vkGetPhysicalDeviceFeatures2(PhysicalDevice, &DeviceFeatures2);
		vkGetPhysicalDeviceMemoryProperties2(PhysicalDevice, &DeviceMemoryProperties2);
	}
#endif
	
	VULKAN_INFO(String("Using adapter '") + DeviceProperties.deviceName + "' Which supports Vulkan " + GetVersionAsString(DeviceProperties.apiVersion));
	
	return true;
}

TOptional<SVulkanQueueFamilyIndices> CVulkanPhysicalDevice::GetQueueFamilyIndices(VkPhysicalDevice PhysicalDevice)
{
	const auto GetQueueFamilyIndex = [](VkQueueFlagBits QueueFlag, const TArray<VkQueueFamilyProperties>& QueueFamilies)
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

		return (~0U);
	};

	// Retrieve the queue indices for the adapter
	TOptional<SVulkanQueueFamilyIndices> QueueIndicies;

	uint32 QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, nullptr);

	TArray<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies.Data());
	
	IConsoleVariable* VerboseVulkan = CConsoleManager::Get().FindVariable("vulkan.VerboseLogging");

	const bool bVerboseLogging = VerboseVulkan && VerboseVulkan->GetBool();
	if (bVerboseLogging)
	{
		uint32 Index = 0;
		for (const VkQueueFamilyProperties& Properties : QueueFamilies)
		{
			String PropertyString = GetQueuePropertiesAsString(Properties);
			VULKAN_INFO("Queue[" + ToString(Index) + "]: " + PropertyString);
			
			Index++;
		}
	}
	
	const uint32 GraphicsIndex = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, QueueFamilies);
	if (GraphicsIndex == (~0U))
	{
		return QueueIndicies;
	}

	QueueFamilies[GraphicsIndex].queueCount--;
	
	const uint32 CopyIndex = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, QueueFamilies);
	if (CopyIndex == (~0U))
	{
		return QueueIndicies;
	}
	
	QueueFamilies[CopyIndex].queueCount--;
	
	const uint32 ComputeIndex = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT , QueueFamilies);
	if (ComputeIndex == (~0U))
	{
		return QueueIndicies;
	}
	
	QueueFamilies[ComputeIndex].queueCount--;

	QueueIndicies.Emplace(GraphicsIndex, CopyIndex, ComputeIndex);
	return QueueIndicies;
}
