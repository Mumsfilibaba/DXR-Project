#include "VulkanSurface.h"
#include "VulkanDevice.h"
#include "VulkanDriverInstance.h"

#include "Platform/PlatformVulkanMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSurface

TSharedRef<CVulkanSurface> CVulkanSurface::CreateSurface(CVulkanDevice* InDevice, CPlatformWindow* InWindow)
{
    TSharedRef<CVulkanSurface> NewSurface = dbg_new CVulkanSurface(InDevice, InWindow);
	if (NewSurface && NewSurface->Initialize())
	{
		return NewSurface;
	}
	
	return nullptr;
}
	
CVulkanSurface::CVulkanSurface(CVulkanDevice* InDevice, CPlatformWindow* InWindow)
    : CVulkanDeviceObject(InDevice)
	, Window(::AddRef(InWindow))
    , Surface(VK_NULL_HANDLE)
	, SupportedFormats()
	, PresentModes()
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
	
	// Create platform specific surface
    VkResult Result = PlatformVulkanMisc::CreateSurface(Instance->GetVkInstance(), Window.Get(), &Surface);
    VULKAN_CHECK_RESULT(Result, "Failed to create Platform Surface");

    TOptional<SVulkanQueueFamilyIndices> QueueFamilyIndices = GetDevice()->GetQueueIndicies();
    if (!QueueFamilyIndices)
    {
        VULKAN_ERROR_ALWAYS("Queue familiy indices are not initialized properly");
        return false;
    }

    CVulkanPhysicalDevice* Adapter = GetDevice()->GetPhysicalDevice();

    // TODO: Don't assume that we are using the graphics queue
    VkBool32 PresentSupport = false;
    Result = vkGetPhysicalDeviceSurfaceSupportKHR(Adapter->GetVkPhysicalDevice(), QueueFamilyIndices->GraphicsQueueIndex, Surface, &PresentSupport);
	VULKAN_CHECK_RESULT(Result, "Failed to retrieve presentation support for surface");
	
    if (!PresentSupport)
    {
        VULKAN_ERROR_ALWAYS("GraphicsQueue does not support presentation");
        return false;
    }

    // Get supported surface formats
    uint32 FormatCount = 0;
    Result = vkGetPhysicalDeviceSurfaceFormatsKHR(Adapter->GetVkPhysicalDevice(), Surface, &FormatCount, nullptr);
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve supported surface formats");

    SupportedFormats.Resize(FormatCount);

    Result = vkGetPhysicalDeviceSurfaceFormatsKHR(Adapter->GetVkPhysicalDevice(), Surface, &FormatCount, SupportedFormats.Data());
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve supported surface formats");
	
    // Find the swapchain format we want
    /*VkFormat DesiredFormat = ConvertFormat(m_Desc.Format);
    m_VkFormat = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    for (const VkSurfaceFormatKHR& availableFormat : formats)
    {
        if (availableFormat.format == lookingFor && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            m_VkFormat = availableFormat;
            break;
        }
    }

    if (m_VkFormat.format != VK_FORMAT_UNDEFINED)
    {
        D_LOG_MESSAGE("[SwapChainVK]: Chosen SwapChain format '%s'", VkFormatToString(m_VkFormat.format));
    }
    else
    {
        LOG_ERROR("Vulkan: Format %s is not supported on. The following formats are supported for Creating a SwapChain", VkFormatToString(lookingFor));
        for (const VkSurfaceFormatKHR& availableFormat : formats)
        {
            LOG_ERROR("    %s", VkFormatToString(availableFormat.format));
        }

        return false;
    }*/

    // Get presentation modes
    uint32 PresentModeCount = 0;
    Result = vkGetPhysicalDeviceSurfacePresentModesKHR(Adapter->GetVkPhysicalDevice(), Surface, &PresentModeCount, nullptr);
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve supported surface presentation modes");

    PresentModes.Resize(PresentModeCount);

    Result = vkGetPhysicalDeviceSurfacePresentModesKHR(Adapter->GetVkPhysicalDevice(), Surface, &PresentModeCount, PresentModes.Data());
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve supported surface presentation modes");

    /*m_PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!m_Desc.VerticalSync)
    {
        // Search for the mailbox mode
        for (const VkPresentModeKHR& availablePresentMode : presentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                m_PresentationMode = availablePresentMode;
                break;
            }
        }

        // If mailbox is not available we choose immediete
        if (m_PresentationMode == VK_PRESENT_MODE_FIFO_KHR)
        {
            for (const VkPresentModeKHR& availablePresentMode : presentModes)
            {
                if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    m_PresentationMode = availablePresentMode;
                    break;
                }
            }
        }
    }

    D_LOG_MESSAGE("[SwapChainVK]: Chosen SwapChain PresentationMode '%s'", VkPresentatModeToString(m_PresentationMode));*/

    return true;
}
