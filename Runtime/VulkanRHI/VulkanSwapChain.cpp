#include "VulkanSwapChain.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSwapChain

CVulkanSwapChainRef CVulkanSwapChain::CreateSwapChain(CVulkanDevice* InDevice, const SVulkanSwapChainCreateInfo& CreateInfo)
{
    CVulkanSwapChainRef NewSwapChain = dbg_new CVulkanSwapChain(InDevice);
    if (NewSwapChain && NewSwapChain->Initialize(CreateInfo))
    {
        return NewSwapChain;
    }

    return nullptr;
}

CVulkanSwapChain::CVulkanSwapChain(CVulkanDevice* InDevice)
    : CVulkanDeviceObject(InDevice)
    , PresentResult(VK_SUCCESS)
    , SwapChain(VK_NULL_HANDLE)
    , BufferIndex(0)
{
}

CVulkanSwapChain::~CVulkanSwapChain()
{
    if (VULKAN_CHECK_HANDLE(SwapChain))
    {
        vkDestroySwapchainKHR(GetDevice()->GetVkDevice(), SwapChain, nullptr);
        SwapChain = VK_NULL_HANDLE;
    }
}

bool CVulkanSwapChain::Initialize(const SVulkanSwapChainCreateInfo& CreateInfo)
{
	CVulkanSurface* Surface = CreateInfo.Surface;
	Assert(Surface != nullptr);
 
    TArray<VkSurfaceFormatKHR> SupportedFormats;
    if (!Surface->GetSupportedFormats(SupportedFormats))
    {
        return false;
    }

    const VkFormat DesiredFormat = ConvertFormat(CreateInfo.Format);
    
    VkSurfaceFormatKHR SelectedFormat = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    for (const VkSurfaceFormatKHR& SurfaceFormat : SupportedFormats)
    {
        if (SurfaceFormat.format == DesiredFormat && SurfaceFormat.colorSpace == CreateInfo.ColorSpace)
        {
            SelectedFormat = SurfaceFormat;
            break;
        }
        else if (SurfaceFormat.format == DesiredFormat)
        {
            SelectedFormat = SurfaceFormat;
        }
    }

    if (SelectedFormat.format == VK_FORMAT_UNDEFINED)
    {
        VULKAN_ERROR_ALWAYS("Failed to find desired format");
        return false;
    }
    else
    {
        VULKAN_INFO("Selected format '" + String(ToString(SelectedFormat.format)) + "' for SwapChain");
    }

    TArray<VkPresentModeKHR> SupportedPresentModes;
    if (!Surface->GetPresentModes(SupportedPresentModes))
    {
        return false;
    }

    // TODO: Investigate Vulkan V-sync
    VkPresentModeKHR SelectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!CreateInfo.bVerticalSync)
    {
        for (const VkPresentModeKHR& PresentMode : SupportedPresentModes)
        {
            if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                SelectedPresentMode = PresentMode;
                break;
            }
        }

        if (SelectedPresentMode == VK_PRESENT_MODE_FIFO_KHR)
        {
            for (const VkPresentModeKHR& PresentMode : SupportedPresentModes)
            {
                if (PresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    SelectedPresentMode = PresentMode;
                    break;
                }
            }
        }
    }

    VULKAN_INFO("Selected presentmode '" + String(ToString(SelectedPresentMode)) + "' for SwapChain");

    VkSurfaceCapabilitiesKHR Capabilities;
    if (!Surface->GetCapabilities(Capabilities))
    {
        return false;
    }

    VkExtent2D CurrentExtent;
    if (Capabilities.currentExtent.width != UINT32_MAX || Capabilities.currentExtent.height != UINT32_MAX || CreateInfo.Extent.width == 0 || CreateInfo.Extent.height == 0)
    {
        CurrentExtent = Capabilities.currentExtent;
    }
    else
    {
        CurrentExtent.width  = NMath::Max(Capabilities.minImageExtent.width , NMath::Min(Capabilities.maxImageExtent.width , CreateInfo.Extent.width));
        CurrentExtent.height = NMath::Max(Capabilities.minImageExtent.height, NMath::Min(Capabilities.maxImageExtent.height, CreateInfo.Extent.height));
    }

    CurrentExtent.width  = NMath::Max(CurrentExtent.width , 1u);
    CurrentExtent.height = NMath::Max(CurrentExtent.height, 1u);

    const uint32 SupportedBufferCount = NMath::Clamp<uint32>(Capabilities.minImageCount, Capabilities.maxImageCount, CreateInfo.BufferCount);
    if (SupportedBufferCount != CreateInfo.BufferCount)
    {
        VULKAN_INFO("Number of buffers(" + ToString(CreateInfo.BufferCount) + ") is not supported. MinBuffers=" + ToString(Capabilities.minImageCount) + "MaxBuffers=" + ToString(Capabilities.maxImageCount));
    }

    VULKAN_INFO("Number of buffers in SwapChain=" + ToString(SupportedBufferCount));

    VkSwapchainCreateInfoKHR SwapChainCreateInfo;
    CMemory::Memzero(&SwapChainCreateInfo);

    SwapChainCreateInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapChainCreateInfo.pNext                 = nullptr;
    SwapChainCreateInfo.surface               = Surface->GetVkSurface();
    SwapChainCreateInfo.minImageCount         = SupportedBufferCount;
    SwapChainCreateInfo.imageFormat           = SelectedFormat.format;
    SwapChainCreateInfo.imageColorSpace       = SelectedFormat.colorSpace;
    SwapChainCreateInfo.imageExtent           = CurrentExtent;
    SwapChainCreateInfo.imageArrayLayers      = 1;
    SwapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    SwapChainCreateInfo.queueFamilyIndexCount = 0;
    SwapChainCreateInfo.pQueueFamilyIndices   = nullptr;
    SwapChainCreateInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    SwapChainCreateInfo.preTransform          = Capabilities.currentTransform;
    SwapChainCreateInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapChainCreateInfo.presentMode           = SelectedPresentMode;
    SwapChainCreateInfo.clipped               = VK_TRUE;

    if (CreateInfo.PreviousSwapChain)
    {
        SwapChainCreateInfo.oldSwapchain = CreateInfo.PreviousSwapChain->GetVkSwapChain();
    }
    else
    {
        SwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    }

    VkResult Result = vkCreateSwapchainKHR(GetDevice()->GetVkDevice(), &SwapChainCreateInfo, nullptr, &SwapChain);
    VULKAN_CHECK_RESULT(Result, "Failed to create SwapChain");

    Result = vkGetSwapchainImagesKHR(GetDevice()->GetVkDevice(), SwapChain, &BufferCount, nullptr);
    VULKAN_CHECK_RESULT(Result, "Failed to retrive the number of images in SwapChain");

    return true;
}

bool CVulkanSwapChain::GetSwapChainImages(VkImage* OutImages)
{
    VkResult Result = vkGetSwapchainImagesKHR(GetDevice()->GetVkDevice(), SwapChain, &BufferCount, OutImages);
    VULKAN_CHECK_RESULT(Result, "Failed to retrive the images of the SwapChain");

    return true;
}

VkResult CVulkanSwapChain::Present(CVulkanQueue* Queue, CVulkanSemaphore* WaitSemaphore)
{
    Assert(Queue != nullptr);

    VkPresentInfoKHR PresentInfo;
    CMemory::Memzero(&PresentInfo);

    PresentInfo.sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.pNext          = nullptr;
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains    = &SwapChain;
    PresentInfo.pResults       = &PresentResult;
    PresentInfo.pImageIndices  = &BufferIndex;

    // If there is a signal semaphore then the queue has not yet been submitted since we retrieved the image and we should not wait for this semaphore
    VkSemaphore WaitSemaphoreHandle = VK_NULL_HANDLE;
    if (WaitSemaphore)
    {
		WaitSemaphoreHandle = WaitSemaphore->GetVkSemaphore();

		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pWaitSemaphores    = &WaitSemaphoreHandle;
    }
    else
    {
		PresentInfo.waitSemaphoreCount = 0;
		PresentInfo.pWaitSemaphores    = nullptr;
    }

    return vkQueuePresentKHR(Queue->GetVkQueue(), &PresentInfo);
}

VkResult CVulkanSwapChain::AquireNextImage(CVulkanSemaphore* AquireSemaphore)
{
    VkSemaphore CurrentImageSemaphore = AquireSemaphore ? AquireSemaphore->GetVkSemaphore() : VK_NULL_HANDLE;
    return vkAcquireNextImageKHR(GetDevice()->GetVkDevice(), SwapChain, UINT64_MAX, CurrentImageSemaphore, VK_NULL_HANDLE, &BufferIndex);
}
