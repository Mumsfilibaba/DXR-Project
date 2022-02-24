#include "VulkanDriverInstance.h"
#include "VulkanLoader.h"

#include "Core/Templates/StringUtils.h"
#include "Core/Debug/Console/ConsoleManager.h"

#include "Platform/PlatformVulkanMisc.h"

#define VULKAN_LOAD_DRIVER_INSTANCE_FUNCTION(FunctionName)                                           \
	vk##FunctionName = reinterpret_cast<PFN_vk##FunctionName>(LoadFunction("vk"#FunctionName)); \
	if (!vk##FunctionName)                                                                      \
    {                                                                                                \
        VULKAN_ERROR_ALWAYS("Failed to load vk"#FunctionName);                                       \
        return false;                                                                                \
    }

static const auto RawStringComparator = [](const char* Lhs, const char* Rhs) -> bool
{
	return StringUtils::Compare(Lhs, Rhs) == 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// Console Variables

TAutoConsoleVariable<bool> GVulkanVerboseLogging("vulkan.VerboseLogging", true);

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDriverInstance

TSharedRef<CVulkanDriverInstance> CVulkanDriverInstance::CreateInstance(const SVulkanDriverInstanceDesc& InstanceDesc) noexcept
{
    TSharedRef<CVulkanDriverInstance> NewInstance = dbg_new CVulkanDriverInstance();
    if (NewInstance && NewInstance->Initialize(InstanceDesc))
    {
        return NewInstance;
    }
    else
    {
        return nullptr;
    }
} 

CVulkanDriverInstance::CVulkanDriverInstance()
    : DriverHandle(nullptr)
    , GetInstanceProcAddrFunc(nullptr)
    , Instance(VK_NULL_HANDLE)
{
}

CVulkanDriverInstance::~CVulkanDriverInstance()
{
    if (DriverHandle)
    {
        PlatformLibrary::FreeDynamicLib(DriverHandle);
    }
	
	if (VULKAN_CHECK_HANDLE(Instance))
	{
		vkDestroyInstance(Instance, nullptr);
	}

#if VK_EXT_debug_utils
	if (VULKAN_CHECK_HANDLE(DebugMessenger))
	{
		vkDestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
	}
#endif
}

bool CVulkanDriverInstance::Initialize(const SVulkanDriverInstanceDesc& InstanceDesc)
{
	DriverHandle = PlatformVulkanMisc::LoadVulkanLibrary();
    if (!DriverHandle)
    {
		VULKAN_ERROR_ALWAYS("Failed to load Vulkan library");
        return false;
    }

	GetInstanceProcAddrFunc = PlatformLibrary::LoadSymbolAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr", DriverHandle);
	if (!GetInstanceProcAddrFunc)
	{
		VULKAN_ERROR_ALWAYS("Failed to load vkGetInstanceProcAddr");
		return false;
	}
		
	VULKAN_LOAD_DRIVER_INSTANCE_FUNCTION(CreateInstance);
	VULKAN_LOAD_DRIVER_INSTANCE_FUNCTION(EnumerateInstanceLayerProperties);
	VULKAN_LOAD_DRIVER_INSTANCE_FUNCTION(EnumerateInstanceExtensionProperties);

	VkResult Result = VK_SUCCESS;
	
	// Instance Layers
	uint32 LayerPropertiesCount = 0;
	Result = vkEnumerateInstanceLayerProperties(&LayerPropertiesCount, nullptr);
	VULKAN_CHECK_RESULT(Result, "Failed to retrive Instance LayerProperties Count");
	
	TArray<VkLayerProperties> LayerProperties(LayerPropertiesCount);
	Result = vkEnumerateInstanceLayerProperties(&LayerPropertiesCount, LayerProperties.Data());
	VULKAN_CHECK_RESULT(Result, "Failed to retrive Instance LayerProperties");

	// Instance Extensions
	uint32 ExtensionPropertiesCount = 0;
	Result = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionPropertiesCount, nullptr);
	VULKAN_CHECK_RESULT(Result, "Failed to retrive Instance ExtensionProperties Count");
	
	TArray<VkExtensionProperties> ExtensionProperties(ExtensionPropertiesCount);
	Result = vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionPropertiesCount, ExtensionProperties.Data());
	VULKAN_CHECK_RESULT(Result, "Failed to retrive Instance ExtensionProperties");

	const bool bVerboseLogging = GVulkanVerboseLogging.GetBool();
	if (bVerboseLogging)
	{
		if (!ExtensionProperties.IsEmpty())
		{
			VULKAN_INFO("Available Instance Extensions:");
			
			for (const VkExtensionProperties& ExtensionProperty : ExtensionProperties)
			{
				LOG_INFO(String("    ") + ExtensionProperty.extensionName);
			}
		}
		else
		{
			VULKAN_INFO("No available Instance Extensions");
		}

		if (!LayerProperties.IsEmpty())
		{
			VULKAN_INFO("Available Instance Layers:");

			for (const VkLayerProperties& LayerProperty : LayerProperties)
			{
				LOG_INFO(String("    ") + LayerProperty.layerName + ": " + LayerProperty.description);
			}
		}
		else
		{
			VULKAN_INFO("No available Instance Layers");
		}
	}

	// Verify Layers
	TArray<const char*> EnabledLayerNames;
	for (const VkLayerProperties& LayerProperty : LayerProperties)
	{
		const char* LayerName = LayerProperty.layerName;
		if (InstanceDesc.RequiredLayerNames.Contains(LayerName, RawStringComparator) || InstanceDesc.OptionalLayerNames.Contains(LayerName, RawStringComparator))
		{
			EnabledLayerNames.Push(LayerName);
		}
	}

	for (const char* LayerName : InstanceDesc.RequiredLayerNames)
	{
		if (!EnabledLayerNames.Contains(LayerName, RawStringComparator))
		{
			VULKAN_ERROR_ALWAYS(String("Instance layer '") + LayerName + "' could not be enabled");
			return false;
		}
		else
		{
			LayerNames.insert(String(LayerName));
		}
	}

	// Verify Extensions
	TArray<const char*> EnabledExtensionNames;
	for (const VkExtensionProperties& ExtensionProperty : ExtensionProperties)
	{
		const char* ExtensionName = ExtensionProperty.extensionName;
		if (InstanceDesc.RequiredExtensionNames.Contains(ExtensionName, RawStringComparator) || InstanceDesc.OptionalExtensionNames.Contains(ExtensionName, RawStringComparator))
		{
			EnabledExtensionNames.Push(ExtensionName);
		}
	}

	for (const char* ExtensionName : InstanceDesc.RequiredExtensionNames)
	{
		if (!EnabledExtensionNames.Contains(ExtensionName, RawStringComparator))
		{
			VULKAN_ERROR_ALWAYS(String("Instance layer '") + ExtensionName + "' could not be enabled");
			return false;
		}
		else
		{
			ExtensionNames.insert(String(ExtensionName));
		}
	}
	
	if (bVerboseLogging)
	{
		if (!EnabledLayerNames.IsEmpty())
		{
			VULKAN_INFO("Enabled Instance Layers:");

			for (const char* LayerName : EnabledLayerNames)
			{
				LOG_INFO(String("    ") + LayerName);
			}
		}
		
		if (!EnabledExtensionNames.IsEmpty())
		{
			VULKAN_INFO("Enabled Instance Extensions:");
			
			for (const char* ExtensionName : EnabledExtensionNames)
			{
				LOG_INFO(String("    ") + ExtensionName);
			}
		}
	}

	VkApplicationInfo ApplicationInfo;
	CMemory::Memzero(&ApplicationInfo);
	
	ApplicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ApplicationInfo.pNext              = nullptr;
	ApplicationInfo.apiVersion         = VK_API_VERSION_1_2;
	ApplicationInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.pApplicationName   = "DXR-Project";
	ApplicationInfo.pEngineName        = "DXR-Engine";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	
	VkInstanceCreateInfo InstanceCreateInfo{};
	CMemory::Memzero(&InstanceCreateInfo);
	
	InstanceCreateInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstanceCreateInfo.flags                    = 0;
	InstanceCreateInfo.pApplicationInfo         = &ApplicationInfo;
	InstanceCreateInfo.enabledExtensionCount    = EnabledExtensionNames.Size();
	InstanceCreateInfo.ppEnabledExtensionNames  = EnabledExtensionNames.Data();
	InstanceCreateInfo.enabledLayerCount        = EnabledLayerNames.Size();
	InstanceCreateInfo.ppEnabledLayerNames      = EnabledLayerNames.Data();

#if VK_EXT_debug_utils
	VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo;
	CMemory::Memzero(&DebugMessengerCreateInfo);

	if (InstanceDesc.bEnableValidationLayer)
	{
		DebugMessengerCreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		DebugMessengerCreateInfo.flags           = 0;
		DebugMessengerCreateInfo.pNext           = nullptr;
		DebugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		DebugMessengerCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		DebugMessengerCreateInfo.pfnUserCallback = DebugLayerCallback;
		DebugMessengerCreateInfo.pUserData       = nullptr;

		InstanceCreateInfo.pNext = reinterpret_cast<const void*>(&DebugMessengerCreateInfo);
	}
	else
#endif
	{
		InstanceCreateInfo.pNext = nullptr;
	}

	Result = vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance);
	VULKAN_CHECK_RESULT(Result, "Failed to create Instance");

	/*///////////////////////////////////////////////////////////////////////////////////////////*/
	// Load functions that require the instance to be created

	VULKAN_LOAD_DRIVER_INSTANCE_FUNCTION(DestroyInstance);
	
#if VK_EXT_debug_utils
	if (IsExtensionEnabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
	{
		VULKAN_LOAD_DRIVER_INSTANCE_FUNCTION(SetDebugUtilsObjectNameEXT);
		VULKAN_LOAD_DRIVER_INSTANCE_FUNCTION(CreateDebugUtilsMessengerEXT);
		VULKAN_LOAD_DRIVER_INSTANCE_FUNCTION(DestroyDebugUtilsMessengerEXT);
	}

	if (InstanceDesc.bEnableValidationLayer)
	{
		Result = vkCreateDebugUtilsMessengerEXT(Instance, &DebugMessengerCreateInfo, nullptr, &DebugMessenger);
		VULKAN_CHECK_RESULT(Result, "Failed to create DebugMessenger");
	}
#endif

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL CVulkanDriverInstance::DebugLayerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT MessageType, 
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, 
	void* UserData)
{

	if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOG_ERROR(String("[Vulkan Validation layer] ") + CallbackData->pMessage);
	}
	else if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOG_WARNING(String("[Vulkan Validation layer] ") + CallbackData->pMessage);
	}

	return VK_FALSE;
}
