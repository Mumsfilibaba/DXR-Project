#include "VulkanSwapChain.h"
#include "Core/Misc/ConsoleManager.h"

TAutoConsoleVariable<bool> CVarOverrideSwapchainFormat(
    "Vulkan.OverrideSwapchainFormat",
    "Enables VulkanRHI to override the specified format with a more suiting format based on the input-format (Example. RGBA -> BGRA)",
    true);

static VkFormat GetOverrideFormat(VkFormat InFormat)
{
    switch(InFormat)
    {
        case VK_FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
        default: return VK_FORMAT_UNDEFINED;
    }
}


FVulkanSwapChain::FVulkanSwapChain(FVulkanDevice* InDevice)
    : FVulkanDeviceObject(InDevice)
    , PresentResult(VK_SUCCESS)
    , SwapChain(VK_NULL_HANDLE)
    , BufferIndex(0)
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
    if (CVarOverrideSwapchainFormat.GetValue())
    {
        // If we did not find the format (And the CVar is true) we try and override the desired format and see if that is available
        if (SelectedFormat.format == VK_FORMAT_UNDEFINED)
        {
            const VkSurfaceFormatKHR OverrideFormat { GetOverrideFormat(DesiredFormat.format), CreateInfo.ColorSpace };
            SelectedFormat = MatchFormat(OverrideFormat);
        }
    }
    
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

    VkExtent2D CurrentExtent;
    if (Capabilities.currentExtent.width != UINT32_MAX || Capabilities.currentExtent.height != UINT32_MAX || CreateInfo.Extent.width == 0 || CreateInfo.Extent.height == 0)
    {
        CurrentExtent = Capabilities.currentExtent;
    }
    else
    {
        CurrentExtent.width  = FMath::Max(Capabilities.minImageExtent.width , FMath::Min(Capabilities.maxImageExtent.width , CreateInfo.Extent.width));
        CurrentExtent.height = FMath::Max(Capabilities.minImageExtent.height, FMath::Min(Capabilities.maxImageExtent.height, CreateInfo.Extent.height));
    }

    CurrentExtent.width  = FMath::Max(CurrentExtent.width , 1u);
    CurrentExtent.height = FMath::Max(CurrentExtent.height, 1u);

    // Get the number of swapchain image that we can have based on the backbuffer CVar
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
    VULKAN_CHECK_RESULT(Result, "Failed to create SwapChain");

    Result = vkGetSwapchainImagesKHR(GetDevice()->GetVkDevice(), SwapChain, &BufferCount, nullptr);
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve the number of images in SwapChain");

    Format = SelectedFormat;
    return true;
}

bool FVulkanSwapChain::GetSwapChainImages(VkImage* OutImages)
{
    VkResult Result = vkGetSwapchainImagesKHR(GetDevice()->GetVkDevice(), SwapChain, &BufferCount, OutImages);
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve the images of the SwapChain");
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