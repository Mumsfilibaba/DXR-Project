#include "VulkanDriverInstance.h"
#include "VulkanFunctions.h"

#define VULKAN_LOAD_DRIVER_INSTANCE_FUNCTION(FunctionName)                                           \
    NVulkan::FunctionName = reinterpret_cast<PFN_vk##FunctionName>(LoadFunction("vk"#FunctionName)); \
    if (!NVulkan::FunctionName)                                                                      \
    {                                                                                                \
        VULKAN_ERROR_ALWAYS("Failed to load vk"#FunctionName);                                       \
        return false;                                                                                \
    }

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDriverInstance

TSharedRef<CVulkanDriverInstance> CVulkanDriverInstance::CreateInstance() noexcept
{
    TSharedRef<CVulkanDriverInstance> NewInstance = dbg_new CVulkanDriverInstance();
    if (NewInstance && NewInstance->Load())
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

bool CVulkanDriverInstance::Load()
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
		
    return true;
}

bool CVulkanDriverInstance::Initialize(const TArray<const char*>& InstanceExtensionNames, const TArray<const char*>& InstanceLayerNames)
{
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
	InstanceCreateInfo.enabledExtensionCount    = InstanceLayerNames.Size();
	InstanceCreateInfo.ppEnabledExtensionNames  = InstanceExtensionNames.Data();
	InstanceCreateInfo.enabledLayerCount        = InstanceLayerNames.Size();
	InstanceCreateInfo.ppEnabledLayerNames      = InstanceLayerNames.Data();
	
	VkResult Result = NVulkan::CreateInstance(&InstanceCreateInfo, nullptr, &Instance);
	if (VK_FAILED(Result))
	{
		VULKAN_ERROR_ALWAYS("Failed to create Instance");
		return false;
	}
	
	return true;
}
