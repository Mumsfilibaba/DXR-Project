#include "VulkanSwapChain.h"

#define NUM_STATIC_WAIT_SEMAPHORES (2)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSwapChain

CVulkanSwapChainRef CVulkanSwapChain::CreateSwapChain(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CVulkanSurface* InSurface, const SVulkanSwapChainCreateInfo& CreateInfo)
{
    CVulkanSwapChainRef NewSwapChain = dbg_new CVulkanSwapChain(InDevice, InQueue, InSurface);
    if (NewSwapChain && NewSwapChain->Initialize(CreateInfo))
    {
        return NewSwapChain;
    }

    return nullptr;
}

CVulkanSwapChain::CVulkanSwapChain(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CVulkanSurface* InSurface)
    : CVulkanDeviceObject(InDevice)
    , PresentResult(VK_SUCCESS)
    , SwapChain(VK_NULL_HANDLE)
    , Surface(::AddRef(InSurface))
    , Queue(::AddRef(InQueue))
    , BufferIndex(0)
    , SemaphoreIndex(0)
    , Images()
    , ImageSemaphores()
{
}

CVulkanSwapChain::~CVulkanSwapChain()
{
    Surface.Reset();
    Queue.Reset();

    if (VULKAN_CHECK_HANDLE(SwapChain))
    {
        vkDestroySwapchainKHR(GetDevice()->GetVkDevice(), SwapChain, nullptr);
        SwapChain = VK_NULL_HANDLE;
    }
}

bool CVulkanSwapChain::Initialize(const SVulkanSwapChainCreateInfo& CreateInfo)
{
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

    const uint32 BufferCount = NMath::Clamp<uint32>(Capabilities.minImageCount, Capabilities.maxImageCount, CreateInfo.BufferCount);
    if (BufferCount != CreateInfo.BufferCount)
    {
        VULKAN_INFO("Number of buffers(" + ToString(CreateInfo.BufferCount) + ") is not supported. MinBuffers=" + ToString(Capabilities.minImageCount) + "MaxBuffers=" + ToString(Capabilities.maxImageCount));
    }

    VULKAN_INFO("Number of buffers in SwapChain=" + ToString(BufferCount));

    VkSwapchainCreateInfoKHR SwapChainCreateInfo;
    CMemory::Memzero(&SwapChainCreateInfo);

    SwapChainCreateInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapChainCreateInfo.pNext                 = nullptr;
    SwapChainCreateInfo.surface               = Surface->GetVkSurface();
    SwapChainCreateInfo.minImageCount         = BufferCount;
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
    SwapChainCreateInfo.oldSwapchain          = VK_NULL_HANDLE;

    VkResult Result = vkCreateSwapchainKHR(GetDevice()->GetVkDevice(), &SwapChainCreateInfo, nullptr, &SwapChain);
    VULKAN_CHECK_RESULT(Result, "Failed to create SwapChain");

    // Create Semaphores to retrieve the swapchain images
    for (uint32 Index = 0; Index < CreateInfo.BufferCount; ++Index)
    {
        CVulkanSemaphore& ImageSemaphore = ImageSemaphores.Emplace(GetDevice());
        if (!ImageSemaphore.Initialize())
        {
            return false;
        }

        ImageSemaphore.SetName("SwapChain ImageSemaphore[" + ToString(Index) + "]");

        CVulkanSemaphore& RenderSemaphore = RenderSemaphores.Emplace(GetDevice());
        if (!RenderSemaphore.Initialize())
        {
            return false;
        }

        RenderSemaphore.SetName("SwapChain RenderSemaphore[" + ToString(Index) + "]");
    }

    if (!RetrieveSwapChainImages())
    {
        return false;
    }

    Result = AquireNextImage();
    VULKAN_CHECK_RESULT(Result, "Failed to retrieve the next swapchain image");
        
    return true;
}

void CVulkanSwapChain::GetSwapChainImages(TArray<VkImage>& OutImages)
{
    OutImages.Clear();
    OutImages.Append(Images.Data(), Images.Size());
}

VkResult CVulkanSwapChain::Present()
{
    Assert(Queue != nullptr);

    VkSemaphore WaitSemaphores[] = { RenderSemaphores[SemaphoreIndex].GetVkSemaphore() };

    VkPresentInfoKHR PresentInfo;
    CMemory::Memzero(&PresentInfo);

    PresentInfo.sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.pNext          = nullptr;
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains    = &SwapChain;
    PresentInfo.pResults       = &PresentResult;
    PresentInfo.pImageIndices  = &BufferIndex;

    // If there is a signal semaphore then the queue has not yet been submitted since we retrieved the image and we should not wait for this semaphore
    if (Queue->IsSignalingSemaphore(WaitSemaphores[0]))
    {
        PresentInfo.waitSemaphoreCount = 0;
        PresentInfo.pWaitSemaphores    = nullptr;
    }
    else
    {
        PresentInfo.waitSemaphoreCount = 1;
        PresentInfo.pWaitSemaphores    = WaitSemaphores;
    }

    VkResult Result = vkQueuePresentKHR(Queue->GetVkQueue(), &PresentInfo);
    if (Result == VK_SUCCESS)
    {
        AquireNextSemaphoreIndex();
        Result = AquireNextImage();
    }
    
    return Result;
}

VkResult CVulkanSwapChain::AquireNextImage()
{
    VkSemaphore CurrentImageSemaphore = ImageSemaphores[SemaphoreIndex].GetVkSemaphore();

    VkResult Result = vkAcquireNextImageKHR(GetDevice()->GetVkDevice(), SwapChain, UINT64_MAX, CurrentImageSemaphore, VK_NULL_HANDLE, &BufferIndex);
    if (Result == VK_SUCCESS)
    {
        Queue->AddWaitSemaphore(CurrentImageSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        
        VkSemaphore CurrentRenderSemaphore = RenderSemaphores[SemaphoreIndex].GetVkSemaphore();
        Queue->AddSignalSemaphore(CurrentRenderSemaphore);
    }
    else
    {
        VULKAN_ERROR_ALWAYS("vkAcquireNextImageKHR failed");
    }

    return Result;
}

bool CVulkanSwapChain::RetrieveSwapChainImages()
{
    uint32 ImageCount = 0;

    VkResult Result = vkGetSwapchainImagesKHR(GetDevice()->GetVkDevice(), SwapChain, &ImageCount, nullptr);
    VULKAN_CHECK_RESULT(Result, "Failed to retrive the number of images in SwapChain");

    Images.Resize(ImageCount);

    Result = vkGetSwapchainImagesKHR(GetDevice()->GetVkDevice(), SwapChain, &ImageCount, Images.Data());
    VULKAN_CHECK_RESULT(Result, "Failed to retrive the images of the SwapChain");

    return true;
}