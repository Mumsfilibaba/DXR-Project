#include "VulkanRHI/VulkanSurface.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanInstance.h"
#include "VulkanRHI/Platform/PlatformVulkan.h"

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
        VULKAN_ERROR_CRITICAL("Failed to create Platform Surface");
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

ESurfaceStatus FVulkanSurface::GetSupportedFormats(TArray<VkSurfaceFormatKHR>& OutSupportedFormats) const
{
	OutSupportedFormats.Reset();

    if (Surface == VK_NULL_HANDLE)
    {
		return ESurfaceStatus::SurfaceLost;
    }

	FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

	uint32 FormatCount = 0;
	VkResult Result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &FormatCount, nullptr);
    if (Result == VK_ERROR_SURFACE_LOST_KHR)
    {
		return ESurfaceStatus::SurfaceLost;
    }

	if (VULKAN_FAILED(Result)) 
    {
		VULKAN_ERROR("FVulkanSurface::GetSupportedFormats vkGetPhysicalDeviceSurfaceFormatsKHR failed: %s", GetVkErrorString(Result));
		return ESurfaceStatus::Error;
	}

	if (FormatCount == 0)
    {
		VULKAN_ERROR("FVulkanSurface::GetSupportedFormats Surface reported zero supported formats");
		return ESurfaceStatus::Error;
	}

	for (int32 Attempt = 0; Attempt < 3; ++Attempt)
	{
		OutSupportedFormats.Resize(FormatCount);

		Result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &FormatCount, OutSupportedFormats.Data());
		if (Result == VK_SUCCESS) 
        {
			// Spec allows formatCount to shrink. Honor returned count.
            if (uint32(OutSupportedFormats.Size()) != FormatCount)
            {
				OutSupportedFormats.Resize(FormatCount);
            }

			return ESurfaceStatus::Ok;
		}

		if (Result == VK_INCOMPLETE) 
        {
			// List grew; ask again with the new count. First query the new required count.
			Result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &FormatCount, nullptr);
            if (Result == VK_ERROR_SURFACE_LOST_KHR)
            {
				return ESurfaceStatus::SurfaceLost;
            }

			if (VULKAN_FAILED(Result)) 
            {
				VULKAN_ERROR("FVulkanSurface::GetSupportedFormats vkGetPhysicalDeviceSurfaceFormatsKHR failed: %s", GetVkErrorString(Result));
				return ESurfaceStatus::Error;
			}

            // Retry with bigger buffer
			continue; 
		}

        if (Result == VK_ERROR_SURFACE_LOST_KHR)
        {
			return ESurfaceStatus::SurfaceLost;
        }

		VULKAN_ERROR("FVulkanSurface::GetSupportedFormats vkGetPhysicalDeviceSurfaceFormatsKHR failed: %s", GetVkErrorString(Result));
		return ESurfaceStatus::Error;
	}

	VULKAN_ERROR("FVulkanSurface::GetSupportedFormats Surface formats changed repeatedly (VK_INCOMPLETE) beyond retry budget");
	return ESurfaceStatus::Error;
}

ESurfaceStatus FVulkanSurface::GetSupportedPresentModes(TArray<VkPresentModeKHR>& OutPresentModes) const
{
	OutPresentModes.Reset();

	if (Surface == VK_NULL_HANDLE)
	{
		return ESurfaceStatus::SurfaceLost;
	}

	FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

	uint32 PresentModeCount = 0;
	VkResult Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &PresentModeCount, nullptr);
	if (Result == VK_ERROR_SURFACE_LOST_KHR)
	{
		return ESurfaceStatus::SurfaceLost;
	}

	if (VULKAN_FAILED(Result)) 
	{
		VULKAN_ERROR("FVulkanSurface::GetSupportedPresentModes vkGetPhysicalDeviceSurfacePresentModesKHR failed: %s", GetVkErrorString(Result));
		return ESurfaceStatus::Error;
	}

	if (PresentModeCount == 0) 
	{
		VULKAN_ERROR("FVulkanSurface::GetSupportedPresentModes Surface reported zero present modes");
		return ESurfaceStatus::Error;
	}

	for (int32 Attempt = 0; Attempt < 3; ++Attempt)
	{
		OutPresentModes.Resize(PresentModeCount);

		Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &PresentModeCount, OutPresentModes.Data());
		if (Result == VK_SUCCESS) 
		{
			if (uint32(OutPresentModes.Size()) != PresentModeCount)
			{
				OutPresentModes.Resize(PresentModeCount);
			}

			return ESurfaceStatus::Ok;
		}

		if (Result == VK_INCOMPLETE) 
		{
			// List grew. Re-query count and retry.
			Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &PresentModeCount, nullptr);
			if (Result == VK_ERROR_SURFACE_LOST_KHR)
			{
				return ESurfaceStatus::SurfaceLost;
			}
			
			if (VULKAN_FAILED(Result)) 
			{
				VULKAN_ERROR("FVulkanSurface::GetSupportedPresentModes vkGetPhysicalDeviceSurfacePresentModesKHR failed: %s", GetVkErrorString(Result));
				return ESurfaceStatus::Error;
			}

			continue;
		}

		if (Result == VK_ERROR_SURFACE_LOST_KHR)
		{
			return ESurfaceStatus::SurfaceLost;
		}

		VULKAN_ERROR("FVulkanSurface::GetSupportedPresentModes vkGetPhysicalDeviceSurfacePresentModesKHR failed: %s", GetVkErrorString(Result));
		return ESurfaceStatus::Error;
	}

	VULKAN_ERROR("FVulkanSurface::GetSupportedPresentModes Present modes changed repeatedly (VK_INCOMPLETE) beyond retry budget");
	return ESurfaceStatus::Error;
}


ESurfaceStatus FVulkanSurface::GetCapabilities(VkSurfaceCapabilitiesKHR& OutCapabilities) const
{
    if (Surface == VK_NULL_HANDLE)
    {
		return ESurfaceStatus::SurfaceLost;
    }

	FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

	VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice->GetVkPhysicalDevice(), Surface, &OutCapabilities);
	if (Result == VK_SUCCESS)
	{
		// Guard against minimized/zero drawable
		if (IsExtentZero(OutCapabilities.currentExtent) || IsExtentZero(OutCapabilities.minImageExtent))
		{
			return ESurfaceStatus::ZeroSized;
		}

		return ESurfaceStatus::Ok;
	}

	if (Result == VK_ERROR_SURFACE_LOST_KHR)
	{
		return ESurfaceStatus::SurfaceLost;
	}

	// Unexpected errors
	VULKAN_WARNING("FVulkanSurface::GetCapabilities vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed: %s", GetVkErrorString(Result));
	return ESurfaceStatus::Error;
}
