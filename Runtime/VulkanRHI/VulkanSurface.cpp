#include "VulkanSurface.h"
#include "VulkanDevice.h"
#include "VulkanDriverInstance.h"

#include "Platform/PlatformVulkan.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSurface

CVulkanSurfaceRef CVulkanSurface::CreateSurface(CVulkanDevice* InDevice, CVulkanQueue* InQueue, void* InWindowHandle)
{
    CVulkanSurfaceRef NewSurface = dbg_new CVulkanSurface(InDevice, InQueue, InWindowHandle);
	if (NewSurface && NewSurface->Initialize())
	{
		return NewSurface;
	}
	
	return nullptr;
}
	
CVulkanSurface::CVulkanSurface(CVulkanDevice* InDevice, CVulkanQueue* InQueue, void* InWindowHandle)
    : CVulkanDeviceObject(InDevice)
    , Queue(::AddRef(InQueue))
	, WindowHandle(InWindowHandle)
    , Surface(VK_NULL_HANDLE)
{
}

CVulkanSurface::~CVulkanSurface()
{
    if (VULKAN_CHECK_HANDLE(Surface))
    {
		CVulkanDriverInstance* Instance = GetDevice()->GetInstance();
        vkDestroySurfaceKHR(Instance->GetVkInstance(), Surface, nullptr);
        Surface = VK_NULL_HANDLE;
    }
}

bool CVulkanSurface::Initialize()
{
	CVulkanDriverInstance* Instance = GetDevice()->GetInstance();
	
    VkResult Result = PlatformVulkan::CreateSurface(Instance->GetVkInstance(), WindowHandle, &Surface);
    VULKAN_CHECK_RESULT(Result, "Failed to create Platform Surface");

    CVulkanPhysicalDevice* Adapter = GetDevice()->GetPhysicalDevice();

    VkBool32 PresentSupport = false;
    Result = vkGetPhysicalDeviceSurfaceSupportKHR(Adapter->GetVkPhysicalDevice(), Queue->GetQueueFamilyIndex(), Surface, &PresentSupport);
	VULKAN_CHECK_RESULT(Result, "Failed to retrieve presentation support for surface");
	
    if (!PresentSupport)
    {
        VULKAN_ERROR_ALWAYS("Queue does not support presentation");
        return false;
    }

    return true;
}

bool CVulkanSurface::GetSupportedFormats(TArray<VkSurfaceFormatKHR>& OutSupportedFormats) const
{
    CVulkanPhysicalDevice* Adapter = GetDevice()->GetPhysicalDevice();

    uint32 FormatCount = 0;
    VkResult Result = vkGetPhysicalDeviceSurfaceFormatsKHR(Adapter->GetVkPhysicalDevice(), Surface, &FormatCount, nullptr);
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve supported surface formats");

    OutSupportedFormats.Resize(FormatCount);

    Result = vkGetPhysicalDeviceSurfaceFormatsKHR(Adapter->GetVkPhysicalDevice(), Surface, &FormatCount, OutSupportedFormats.Data());
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve supported surface formats");

    return true;
}

bool CVulkanSurface::GetPresentModes(TArray<VkPresentModeKHR>& OutPresentModes) const
{
    CVulkanPhysicalDevice* Adapter = GetDevice()->GetPhysicalDevice();

    uint32 PresentModeCount = 0;
    VkResult Result = vkGetPhysicalDeviceSurfacePresentModesKHR(Adapter->GetVkPhysicalDevice(), Surface, &PresentModeCount, nullptr);
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve supported surface presentation modes");

    OutPresentModes.Resize(PresentModeCount);

    Result = vkGetPhysicalDeviceSurfacePresentModesKHR(Adapter->GetVkPhysicalDevice(), Surface, &PresentModeCount, OutPresentModes.Data());
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve supported surface presentation modes");

    return true;
}

bool CVulkanSurface::GetCapabilities(VkSurfaceCapabilitiesKHR& OutCapabilities) const
{
    CVulkanPhysicalDevice* Adapter = GetDevice()->GetPhysicalDevice();

    VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Adapter->GetVkPhysicalDevice(), Surface, &OutCapabilities);
    VULKAN_CHECK_RESULT(Result, "Failed to get surface capabilities");

    return true;
}
