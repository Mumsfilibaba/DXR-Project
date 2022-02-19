#include "VulkanDriverInstance.h"
#include "VulkanFunctions.h"

#include "Core/Templates/StringUtils.h"
#include "Core/Debug/Console/ConsoleManager.h"

#define VULKAN_LOAD_DRIVER_INSTANCE_FUNCTION(FunctionName)                                           \
    NVulkan::FunctionName = reinterpret_cast<PFN_vk##FunctionName>(LoadFunction("vk"#FunctionName)); \
    if (!NVulkan::FunctionName)                                                                      \
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

TAutoConsoleVariable<bool> GVerboseVulkan("vulkan.VerboseVulkan", true);

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
		NVulkan::DestroyInstance(Instance, nullptr);
	}
}

bool CVulkanDriverInstance::Initialize(const SVulkanDriverInstanceDesc& InstanceDesc)
{
	DriverHandle = PlatformLibrary::LoadDynamicLib("vulkan");
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
	Result = NVulkan::EnumerateInstanceLayerProperties(&LayerPropertiesCount, nullptr);
	VULKAN_CHECK_RESULT(Result, "Failed to retrive Instance LayerProperties Count");
	
	TArray<VkLayerProperties> LayerProperties(LayerPropertiesCount);
	Result = NVulkan::EnumerateInstanceLayerProperties(&LayerPropertiesCount, LayerProperties.Data());
	VULKAN_CHECK_RESULT(Result, "Failed to retrive Instance LayerProperties");

	// Instance Extensions
	uint32 ExtensionPropertiesCount = 0;
	Result = NVulkan::EnumerateInstanceExtensionProperties(nullptr, &ExtensionPropertiesCount, nullptr);
	VULKAN_CHECK_RESULT(Result, "Failed to retrive Instance ExtensionProperties Count");
	
	TArray<VkExtensionProperties> ExtensionProperties(ExtensionPropertiesCount);
	Result = NVulkan::EnumerateInstanceExtensionProperties(nullptr, &ExtensionPropertiesCount, ExtensionProperties.Data());
	VULKAN_CHECK_RESULT(Result, "Failed to retrive Instance ExtensionProperties");

	if (GVerboseVulkan.GetBool())
	{
		LOG_INFO("[VulkanRHI] Available Instance Extensions:");
		
		for (const VkExtensionProperties& ExtensionProperty : ExtensionProperties)
		{
			LOG_INFO(ExtensionProperty.extensionName);
		}
		
		LOG_INFO("[VulkanRHI] Available Instance Layers:");

		for (const VkLayerProperties& LayerProperty : LayerProperties)
		{
			String LayerName(LayerProperty.layerName);
			LOG_INFO(LayerName + ": " + LayerProperty.description);
		}
	}

	// Varify Layers
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
			LOG_ERROR(String("Instance layer '") + LayerName + "' could not be enabled");
			return false;
		}
	}

	// Varify Extensions
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
			LOG_ERROR(String("Instance layer '") + ExtensionName + "' could not be enabled");
			return false;
		}
	}
	
	if (GVerboseVulkan.GetBool())
	{
		if (!EnabledLayerNames.IsEmpty())
		{
			LOG_INFO("[VulkanRHI] Enabled Instance Layers:");

			for (const char* LayerName : EnabledLayerNames)
			{
				LOG_INFO(LayerName);
			}
		}
		
		if (!EnabledExtensionNames.IsEmpty())
		{
			LOG_INFO("[VulkanRHI] Enabled Instance Extensions:");
			
			for (const char* ExtensionName : EnabledExtensionNames)
			{
				LOG_INFO(ExtensionName);
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
	InstanceCreateInfo.pNext                    = nullptr;
	InstanceCreateInfo.pApplicationInfo         = &ApplicationInfo;
	InstanceCreateInfo.enabledExtensionCount    = EnabledExtensionNames.Size();
	InstanceCreateInfo.ppEnabledExtensionNames  = EnabledExtensionNames.Data();
	InstanceCreateInfo.enabledLayerCount        = EnabledLayerNames.Size();
	InstanceCreateInfo.ppEnabledLayerNames      = EnabledLayerNames.Data();
	
	Result = NVulkan::CreateInstance(&InstanceCreateInfo, nullptr, &Instance);
	VULKAN_CHECK_RESULT(Result, "Failed to create Instance");

	return true;
}
