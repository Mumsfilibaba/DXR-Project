#include "VulkanSurface.h"
#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "Platform/PlatformVulkan.h"
   
FVulkanSurface::FVulkanSurface(FVulkanDevice* InDevice, FVulkanQueue& InQueue, void* InWindowHandle)
    : FVulkanDeviceChild(InDevice)
    , Surface(VK_NULL_HANDLE)
    , WindowHandle(InWindowHandle)
    , Queue(InQueue)
{
}

FVulkanSurface::~FVulkanSurface()
{
    if (VULKAN_CHECK_HANDLE(Surface))
    {
        FVulkanInstance* Instance = GetDevice()->GetInstance();
        vkDestroySurfaceKHR(Instance->GetVkInstance(), Surface, nullptr);
        Surface = VK_NULL_HANDLE;
    }
}

bool FVulkanSurface::Initialize()
{
    FVulkanInstance* Instance = GetDevice()->GetInstance();
    
    VkResult Result = FPlatformVulkan::CreateSurface(Instance->GetVkInstance(), WindowHandle, &Surface);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Platform Surface");
        return false;
    }

    FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();
    VkBool32 PresentSupport = false;
    Result = vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice->GetVkPhysicalDevice(), Queue.GetQueueFamilyIndex(), Surface, &PresentSupport);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve presentation support for surface");
        return false;
    }

    if (!PresentSupport)
    {
        VULKAN_ERROR("Queue does not support presentation");
        return false;
    }

    return true;
}

bool FVulkanSurface::GetSupportedFormats(TArray<VkSurfaceFormatKHR>& OutSupportedFormats) const
{
    FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

    uint32 FormatCount = 0;
    VkResult Result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &FormatCount, nullptr);
    if (Result == VK_ERROR_SURFACE_LOST_KHR)
    {
        return false;
    }

    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve supported surface formats");
        return false;
    }

    OutSupportedFormats.Resize(FormatCount);
    if (OutSupportedFormats.IsEmpty())
    {
        VULKAN_ERROR("Surface does not support any formats");
        return false;
    }

    Result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &FormatCount, OutSupportedFormats.Data());
    if (Result == VK_ERROR_SURFACE_LOST_KHR)
    {
        return false;
    }

    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve supported surface formats");
        return false;
    }

    return true;
}

bool FVulkanSurface::GetSupportedPresentModes(TArray<VkPresentModeKHR>& OutPresentModes) const
{
    FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

    uint32 PresentModeCount = 0;
    VkResult Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &PresentModeCount, nullptr);
    if (Result == VK_ERROR_SURFACE_LOST_KHR)
    {
        return false;
    }
    
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve supported surface presentation modes");
        return false;
    }

    OutPresentModes.Resize(PresentModeCount);
    if (OutPresentModes.IsEmpty())
    {
        VULKAN_ERROR("Surface does not support any present-mode");
        return false;
    }

    Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &PresentModeCount, OutPresentModes.Data());
    if (Result == VK_ERROR_SURFACE_LOST_KHR)
    {
        return false;
    }

    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve supported surface presentation modes");
        return false;
    }

    return true;
}

bool FVulkanSurface::GetCapabilities(VkSurfaceCapabilitiesKHR& OutCapabilities) const
{
    FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();
    VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &OutCapabilities);
    if (Result == VK_ERROR_SURFACE_LOST_KHR)
    {
        return false;
    }

    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to get surface capabilities");
        return false;
    }

    return true;
}
