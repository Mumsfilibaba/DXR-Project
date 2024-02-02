#include "VulkanSwapChain.h"

FVulkanSwapChain::FVulkanSwapChain(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , PresentResult(VK_SUCCESS)
    , SwapChain(VK_NULL_HANDLE)
    , Extent{ 0, 0 }
    , BufferIndex(0)
    , BufferCount(0)
    , Format{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
{
}

FVulkanSwapChain::~FVulkanSwapChain()
{
    if (VULKAN_CHECK_HANDLE(SwapChain))
    {
        vkDestroySwapchainKHR(GetDevice()->GetVkDevice(), SwapChain, nullptr);
        SwapChain = VK_NULL_HANDLE;
    }
}

bool FVulkanSwapChain::Initialize(const FVulkanSwapChainCreateInfo& CreateInfo)
{
    FVulkanSurface* Surface = CreateInfo.Surface;
    CHECK(Surface != nullptr);
 
    TArray<VkSurfaceFormatKHR> SupportedFormats;
    if (!Surface->GetSupportedFormats(SupportedFormats))
    {
        return false;
    }

    // This lambda tries to match the format that we want with one that is available
    const auto MatchFormat = [&](VkSurfaceFormatKHR DesiredFormat) -> VkSurfaceFormatKHR
    {
        VkSurfaceFormatKHR SelectedFormat = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        for (const VkSurfaceFormatKHR& SurfaceFormat : SupportedFormats)
        {
            if (SurfaceFormat.format == DesiredFormat.format && SurfaceFormat.colorSpace == DesiredFormat.colorSpace)
            {
                SelectedFormat = SurfaceFormat;
                break;
            }
            else if (SurfaceFormat.format == DesiredFormat.format)
            {
                SelectedFormat = SurfaceFormat;
            }
        }
        
        return SelectedFormat;
    };

    // The format we want to use for the swapchain
    const VkSurfaceFormatKHR DesiredFormat = { ConvertFormat(CreateInfo.Format), CreateInfo.ColorSpace };
    
    // See if the exact format is available
    VkSurfaceFormatKHR SelectedFormat = MatchFormat(DesiredFormat);

    // Here we need to see if we failed to find any format
    if (SelectedFormat.format == VK_FORMAT_UNDEFINED)
    {
        VULKAN_ERROR("Failed to find desired format");
        return false;
    }
    else
    {
        VULKAN_INFO("Selected format '%s' for SwapChain", ToString(SelectedFormat.format));
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

    VULKAN_INFO("Selected presentmode '%s' for SwapChain", ToString(SelectedPresentMode));

    VkSurfaceCapabilitiesKHR Capabilities;
    if (!Surface->GetCapabilities(Capabilities))
    {
        return false;
    }

    // Determine the size of the SwapChain
    VkExtent2D CurrentExtent;
    if (Capabilities.currentExtent.width != UINT32_MAX || Capabilities.currentExtent.height != UINT32_MAX || CreateInfo.Extent.width == 0 || CreateInfo.Extent.height == 0)
    {
        CurrentExtent = Capabilities.currentExtent;
    }
    else
    {
        CurrentExtent.width  = FMath::Clamp(Capabilities.minImageExtent.width , Capabilities.maxImageExtent.width , CreateInfo.Extent.width);
        CurrentExtent.height = FMath::Clamp(Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height, CreateInfo.Extent.height);
    }

    CurrentExtent.width  = FMath::Max(CurrentExtent.width , 1u);
    CurrentExtent.height = FMath::Max(CurrentExtent.height, 1u);
    VULKAN_INFO("SwapChain - CurrentExtent: w=%d h=%d, MinExtent: w=%d h=%d, MaxExtent: w=%d h=%d", Capabilities.currentExtent.width, Capabilities.currentExtent.height, Capabilities.minImageExtent.width, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.width, Capabilities.maxImageExtent.height);

    // Get the number of swapchain image that we can have based on the BackBuffer CVar
    const uint32 SupportedBufferCount = Capabilities.maxImageCount == 0 ? CreateInfo.BufferCount : FMath::Clamp<uint32>(Capabilities.minImageCount, Capabilities.maxImageCount, CreateInfo.BufferCount);
    if (SupportedBufferCount != CreateInfo.BufferCount)
    {
        VULKAN_INFO("Number of buffers(=%d) is not supported. MinBuffers=%d MaxBuffers=%d", CreateInfo.BufferCount, Capabilities.minImageCount, Capabilities.maxImageCount);
    }

    VULKAN_INFO("Number of buffers in SwapChain=%d", SupportedBufferCount);

    VkSwapchainCreateInfoKHR SwapChainCreateInfo;
    FMemory::Memzero(&SwapChainCreateInfo);

    SwapChainCreateInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapChainCreateInfo.pNext                 = nullptr;
    SwapChainCreateInfo.surface               = Surface->GetVkSurface();
    SwapChainCreateInfo.oldSwapchain          = CreateInfo.PreviousSwapChain ? CreateInfo.PreviousSwapChain->GetVkSwapChain() : VK_NULL_HANDLE;
    SwapChainCreateInfo.minImageCount         = SupportedBufferCount;
    SwapChainCreateInfo.imageFormat           = SelectedFormat.format;
    SwapChainCreateInfo.imageColorSpace       = SelectedFormat.colorSpace;
    SwapChainCreateInfo.imageExtent           = CurrentExtent;
    SwapChainCreateInfo.imageArrayLayers      = 1;
    SwapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    SwapChainCreateInfo.queueFamilyIndexCount = 0;
    SwapChainCreateInfo.pQueueFamilyIndices   = nullptr;
    SwapChainCreateInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    SwapChainCreateInfo.preTransform          = Capabilities.currentTransform;
    SwapChainCreateInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapChainCreateInfo.presentMode           = SelectedPresentMode;
    SwapChainCreateInfo.clipped               = VK_TRUE;

    VkResult Result = vkCreateSwapchainKHR(GetDevice()->GetVkDevice(), &SwapChainCreateInfo, nullptr, &SwapChain);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create SwapChain");
        return false;
    }

    Result = vkGetSwapchainImagesKHR(GetDevice()->GetVkDevice(), SwapChain, &BufferCount, nullptr);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve the number of images in SwapChain");
        return false;
    }

    Extent = CurrentExtent;
    Format = SelectedFormat;
    return true;
}

bool FVulkanSwapChain::GetSwapChainImages(VkImage* OutImages)
{
    VkResult Result = vkGetSwapchainImagesKHR(GetDevice()->GetVkDevice(), SwapChain, &BufferCount, OutImages);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to retrieve the images of the SwapChain");
        return false;
    }

    return true;
}

VkResult FVulkanSwapChain::Present(FVulkanQueue* Queue, FVulkanSemaphore* WaitSemaphore)
{
    CHECK(Queue != nullptr);

    VkPresentInfoKHR PresentInfo;
    FMemory::Memzero(&PresentInfo);

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

VkResult FVulkanSwapChain::AquireNextImage(FVulkanSemaphore* AquireSemaphore)
{
    VkSemaphore CurrentImageSemaphore = AquireSemaphore ? AquireSemaphore->GetVkSemaphore() : VK_NULL_HANDLE;
    return vkAcquireNextImageKHR(GetDevice()->GetVkDevice(), SwapChain, UINT64_MAX, CurrentImageSemaphore, VK_NULL_HANDLE, &BufferIndex);
}
