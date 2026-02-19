#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanCommandBuffer.h"

static constexpr bool GVulkanReportSwapChainAcquireImageNonSuccessResult = true;

// ============================================================================
// FVulkanSwapChain (Low-level Resource) Implementation
// ============================================================================

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

    BufferIndex = 0;
}

bool FVulkanSwapChain::Initialize(const FVulkanSwapChainCreateInfo& CreateInfo)
{
    FVulkanSurface* Surface = CreateInfo.Surface;
    CHECK(Surface != nullptr);

    VkSurfaceCapabilitiesKHR Capabilities = { };
    {
        ESurfaceStatus SurfaceStatus = Surface->GetCapabilities(Capabilities);
        if (SurfaceStatus != ESurfaceStatus::Ok)
        {
            VULKAN_WARNING("Swapchain init aborted: GetCapabilities returned (status=%s)", ToString(SurfaceStatus));
            return false;
        }
    }

    TArray<VkSurfaceFormatKHR> SupportedFormats;
    {
        ESurfaceStatus SurfaceStatus = Surface->GetSupportedFormats(SupportedFormats);
        if (SurfaceStatus != ESurfaceStatus::Ok || SupportedFormats.IsEmpty())
        {
            VULKAN_WARNING("Swapchain init aborted: no supported surface formats (status=%s)", ToString(SurfaceStatus));
            return false;
        }
    }

    TArray<VkPresentModeKHR> SupportedPresentModes;
    {
        ESurfaceStatus SurfaceStatus = Surface->GetSupportedPresentModes(SupportedPresentModes);
        if (SurfaceStatus != ESurfaceStatus::Ok || SupportedPresentModes.IsEmpty())
        {
            VULKAN_WARNING("Swapchain init aborted: no supported present modes (status=%s)", ToString(SurfaceStatus));
            return false;
        }
    }

    // Pick surface format
    const auto MatchFormat = [&](VkSurfaceFormatKHR Desired) -> VkSurfaceFormatKHR
    {
        VkSurfaceFormatKHR Selected = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        for (const VkSurfaceFormatKHR& Format : SupportedFormats)
        {
            if (Format.format == Desired.format && Format.colorSpace == Desired.colorSpace)
            { 
                Selected = Format;
                break;
            }

            if (Format.format == Desired.format)
            {
                // Keep best partial match
                Selected = Format;
            }
        }

        // If still undefined, just take first supported as absolute fallback.
        if (Selected.format == VK_FORMAT_UNDEFINED)
        {
            Selected = SupportedFormats[0];
        }

        return Selected;
    };

    const VkSurfaceFormatKHR DesiredFormat =
    { 
        ConvertFormat(CreateInfo.Format),
        CreateInfo.ColorSpace
    };
    
    VkSurfaceFormatKHR SelectedFormat = MatchFormat(DesiredFormat);
    if (SelectedFormat.format == VK_FORMAT_UNDEFINED)
    {
        VULKAN_ERROR("Failed to select a surface format");
        return false;
    }
    else
    {
        VULKAN_INFO("Selected format '%s' (colorspace=%d) for SwapChain", ToString(SelectedFormat.format), int(SelectedFormat.colorSpace));
    }

    // TODO: Investigate Vulkan V-sync
    // Pick present mode (FIFO guaranteed by spec)
    const auto HasPresentMode = [&](const VkPresentModeKHR& InPresentMode)
    {
        for (const VkPresentModeKHR& PresentMode : SupportedPresentModes)
        {
            if (PresentMode == InPresentMode)
            {
                return true;
            }
        }

        return false;
    };

    // VK_PRESENT_MODE_FIFO_KHR is spec-guaranteed
    VkPresentModeKHR SelectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (CreateInfo.bVerticalSync)
    {
        if (HasPresentMode(VK_PRESENT_MODE_MAILBOX_KHR))
        {
            SelectedPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if (HasPresentMode(VK_PRESENT_MODE_FIFO_KHR))
        {
            SelectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        }
        else if (HasPresentMode(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
        {
            SelectedPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }
    }
    else
    {
        if (HasPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR))
        {
            SelectedPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        else if (HasPresentMode(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
        {
            SelectedPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }
        else
        {
            SelectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        }
    }

    VULKAN_INFO("Selected present mode '%s' for SwapChain", ToString(SelectedPresentMode));

    // Determine extent (handle undefined and zero-sized properly)
    VkExtent2D CurrentExtent = { };
    if (!IsUndefinedExtent(Capabilities))
    {
        // Surface dictates size
        CurrentExtent = Capabilities.currentExtent;
    }
    else
    {
        if (CreateInfo.Extent.width == 0 || CreateInfo.Extent.height == 0)
        {
            // Caller should wait until non-zero before init/recreate.
            VULKAN_WARNING("Swapchain creation requested with zero-sized extent (probably minimized).");
            return false;
        }

        CurrentExtent.width  = Math::Clamp(CreateInfo.Extent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
        CurrentExtent.height = Math::Clamp(CreateInfo.Extent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);
    }

    CurrentExtent.width  = Math::Max(CurrentExtent.width, 1u);
    CurrentExtent.height = Math::Max(CurrentExtent.height, 1u);

    VULKAN_INFO("SwapChain Extent: current=(%u,%u) min=(%u,%u) max=(%u,%u) chosen=(%u,%u)", Capabilities.currentExtent.width, Capabilities.currentExtent.height,
        Capabilities.minImageExtent.width, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.width, Capabilities.maxImageExtent.height, 
        CurrentExtent.width, CurrentExtent.height);

    // Image count (respect min/max)
    uint32 DesiredCount = Math::Max<uint32>(CreateInfo.BufferCount, Capabilities.minImageCount);
    if (Capabilities.maxImageCount > 0)
    {
        DesiredCount = Math::Min(DesiredCount, Capabilities.maxImageCount);
    }

    if (DesiredCount != CreateInfo.BufferCount)
    {
        VULKAN_INFO("Adjusted buffer count from %u to %u (min=%u max=%u)", CreateInfo.BufferCount, DesiredCount, Capabilities.minImageCount, Capabilities.maxImageCount);
    }

    // Queue sharing mode (graphics/present may differ)
    const uint32 QueueFamilyIndices[2] = 
    {
        // TODO: Separate Present- and Graphics- Queues
        GetDevice()->GetQueueIndexFromType(EVulkanCommandQueueType::Graphics),
        GetDevice()->GetQueueIndexFromType(EVulkanCommandQueueType::Graphics)
    };

    const bool bSameFamily = QueueFamilyIndices[0] == QueueFamilyIndices[1];

    // Pre-transform 
    const VkSurfaceTransformFlagBitsKHR PreTransform = (Capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? 
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : Capabilities.currentTransform;

    // Composite alpha
    const VkCompositeAlphaFlagBitsKHR CompositeAlpha = [](VkCompositeAlphaFlagsKHR Supported) -> VkCompositeAlphaFlagBitsKHR
    {
        const VkCompositeAlphaFlagBitsKHR Preferences[] =
        {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        };

        for (VkCompositeAlphaFlagBitsKHR CurrentFlag : Preferences)
        {
            if (Supported & CurrentFlag)
            {
                return CurrentFlag;
            }
        }

        return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }(Capabilities.supportedCompositeAlpha);

    // Ensure that all the image usage flags are supported
    const VkImageUsageFlags SupportedUsage = Capabilities.supportedUsageFlags;
    const VkImageUsageFlags RequestedUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    const VkImageUsageFlags FinalUsage = RequestedUsage & SupportedUsage;
    if (FinalUsage != RequestedUsage)
    {
        VULKAN_ERROR_CRITICAL("Surface does not support the requested ImageUsage flags");
        return false;
    }

    // Create swapchain
    VkSwapchainCreateInfoKHR SwapChainCreateInfo = { };
    SwapChainCreateInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapChainCreateInfo.pNext                 = nullptr;
    SwapChainCreateInfo.surface               = Surface->GetVkSurface();
    SwapChainCreateInfo.oldSwapchain          = CreateInfo.PreviousSwapChain ? CreateInfo.PreviousSwapChain->GetVkSwapChain() : VK_NULL_HANDLE;
    SwapChainCreateInfo.minImageCount         = DesiredCount;
    SwapChainCreateInfo.imageFormat           = SelectedFormat.format;
    SwapChainCreateInfo.imageColorSpace       = SelectedFormat.colorSpace;
    SwapChainCreateInfo.imageExtent           = CurrentExtent;
    SwapChainCreateInfo.imageArrayLayers      = 1;
    SwapChainCreateInfo.imageUsage            = FinalUsage;
    SwapChainCreateInfo.imageSharingMode      = bSameFamily ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
    SwapChainCreateInfo.queueFamilyIndexCount = bSameFamily ? 0u : 2u;
    SwapChainCreateInfo.pQueueFamilyIndices   = bSameFamily ? nullptr : QueueFamilyIndices;
    SwapChainCreateInfo.preTransform          = PreTransform;
    SwapChainCreateInfo.compositeAlpha        = CompositeAlpha;
    SwapChainCreateInfo.presentMode           = SelectedPresentMode;
    SwapChainCreateInfo.clipped               = VK_TRUE;

    VkResult Result = vkCreateSwapchainKHR(GetDevice()->GetVkDevice(), &SwapChainCreateInfo, nullptr, &SwapChain);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create SwapChain (vkCreateSwapchainKHR=%s)", ToString(Result));
        return false;
    }

    // Fetch image count
    Result = vkGetSwapchainImagesKHR(GetDevice()->GetVkDevice(), SwapChain, &BufferCount, nullptr);
    if (VULKAN_FAILED(Result) || BufferCount == 0)
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve the number of images in SwapChain (res=%s count=%u)", ToString(Result), BufferCount);
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
        VULKAN_ERROR_CRITICAL("Failed to retrieve the images of the SwapChain (%s)", ToString(Result));
        return false;
    }

    return true;
}

VkResult FVulkanSwapChain::Present(FVulkanQueue& Queue, FVulkanSemaphore* WaitSemaphore)
{
    VkPresentInfoKHR PresentInfo = { };
    PresentInfo.sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains    = &SwapChain;
    PresentInfo.pImageIndices  = &BufferIndex;
    PresentInfo.pResults       = &PresentResult;

    VkSemaphore WaitSemaphoreHandle = WaitSemaphore ? WaitSemaphore->GetVkSemaphore() : VK_NULL_HANDLE;
    if (WaitSemaphoreHandle)
    {
        PresentInfo.waitSemaphoreCount = 1;
        PresentInfo.pWaitSemaphores    = &WaitSemaphoreHandle;
    }
    else
    {
        PresentInfo.waitSemaphoreCount = 0;
        PresentInfo.pWaitSemaphores    = nullptr;
    }

    // Caller should handle VK_ERROR_OUT_OF_DATE_KHR / VK_SUBOPTIMAL_KHR to trigger recreate.
    return vkQueuePresentKHR(Queue.GetVkQueue(), &PresentInfo);
}

VkResult FVulkanSwapChain::AcquireNextImage(FVulkanSemaphore* AcquireSemaphore)
{
    VkSemaphore Semaphore = AcquireSemaphore ? AcquireSemaphore->GetVkSemaphore() : VK_NULL_HANDLE;

    VkResult Result = vkAcquireNextImageKHR(GetDevice()->GetVkDevice(), SwapChain, UINT64_MAX, Semaphore, VK_NULL_HANDLE, &BufferIndex);
    if (GVulkanReportSwapChainAcquireImageNonSuccessResult && Result != VK_SUCCESS)
    {
        VULKAN_WARNING("FVulkanSwapChain::AcquireNextImage vkAcquireNextImageKHR did not return VK_SUCCESS. Result = '%s'", ToString(Result));
    }
    
    // BufferIndex is only valid on SUCCESS/SUBOPTIMAL
    if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    {
        BufferIndex = 0;
    }

    // Caller should treat OUT_OF_DATE -> recreate now, and SUBOPTIMAL -> recreate soon/skip frame.
    return Result;
}

// ============================================================================
// FVulkanSwapChainRHI (RHI Interface) Implementation
// ============================================================================

static TAutoConsoleVariable<int32> CVarBackbufferCount(
    "VulkanRHI.BackbufferCount",
    "The preferred number of backbuffers for the SwapChain",
    NUM_BACK_BUFFERS,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarEnableVSync(
    "VulkanRHI.EnableVSync",
    "Enable V-Sync for SwapChains (Already created viewports does not change mode at the moment)",
    true,
    EConsoleVariableFlags::Default);

static constexpr const bool GVulkanLogSemaphoreIndex = false;

FVulkanSwapChainRHI::FVulkanSwapChainRHI(FVulkanDevice* InDevice, const FRHISwapChainDesc& InSwapChainDesc)
    : FRHISwapChain(InSwapChainDesc)
    , FVulkanDeviceChild(InDevice)
    , WindowHandle(InSwapChainDesc.WindowHandle)
    , Surface(nullptr)
    , SwapChain(nullptr)
    , BackBuffer(nullptr)
    , BackBuffers()
    , ImageSemaphores()
    , RenderSemaphores()
    , BackBufferIndex(VULKAN_INVALID_BACK_BUFFER_INDEX)
{
}

FVulkanSwapChainRHI::~FVulkanSwapChainRHI()
{
    FVulkanCommandContext* InCommandContext = FVulkanRHI::Get()->ObtainVulkanCommandContext();
    DestroySwapChain(InCommandContext);
}

bool FVulkanSwapChainRHI::Initialize(FVulkanCommandContext* InCommandContext)
{
    if (!InCommandContext)
    {
        VULKAN_ERROR_CRITICAL("CommandContext cannot be nullptr");
        return false;
    }
    
    Surface = new FVulkanSurface(GetDevice(), InCommandContext->GetCommandQueue(), WindowHandle);
    if (!Surface->Initialize())
    {
        VULKAN_ERROR_CRITICAL("Failed to create Surface");
        return false;
    }
    
    // We need to start the context since that locks it to this thread
    InCommandContext->StartContext();

    if (!CreateSwapChain(InCommandContext, GetWidth(), GetHeight()))
    {
        return false;
    }
    
    // Unlock the context from this thread
    InCommandContext->FinishContext();
    InCommandContext->Flush();

    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(GetColorFormat(), GetWidth(), GetHeight(), 1, 1, ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable);
    BackBuffer = new FVulkanBackBufferTexture(GetDevice(), this, BackBufferDesc);
    if (!BackBuffer)
    {
        VULKAN_ERROR_CRITICAL("Failed to create BackBuffer");
        return false;
    }

    if (BackBufferDesc.bEnableResourceStateTracking)
    {
        BackBuffer->EnableStateTracking(EResourceAccess::Present);
    }

    if (FVulkanImageLayoutState* LayoutState = BackBuffer->GetImageLayoutState())
    {
        FVulkanImageLayoutState::FImageState PresentState;
        PresentState.Layout = FVulkanRHI::ResourceStateToImageLayout(EResourceAccess::Present);
        PresentState.Access = FVulkanRHI::ResourceStateToAccessFlags(EResourceAccess::Present);
        PresentState.Stage  = FVulkanRHI::ResourceStateToPipelineStageFlags(EResourceAccess::Present);
        LayoutState->SetState(PresentState);
    }

    return true;
}

bool FVulkanSwapChainRHI::RecreateSurface(FVulkanCommandContext* InCommandContext)
{
    Surface = new FVulkanSurface(GetDevice(), InCommandContext->GetCommandQueue(), WindowHandle);
    if (!Surface->Initialize())
    {
        VULKAN_WARNING("RecreateSurface: failed to initialize surface");
        return false;
    }

    return true;
}

bool FVulkanSwapChainRHI::ValidateSurfaceAndSize(FVulkanCommandContext* InCommandContext, uint32& OutWidth, uint32& OutHeight)
{
    VkSurfaceCapabilitiesKHR Capabilities = { };

    ESurfaceStatus SurfaceStatus = Surface->GetCapabilities(Capabilities);
    if (SurfaceStatus == ESurfaceStatus::SurfaceLost)
    {
        VULKAN_INFO("FVulkanSwapChainRHI::ValidateSurfaceAndSize Surface lost. Attempting one recreation.");
        if (!RecreateSurface(InCommandContext))
        {
            return false;
        }

        SurfaceStatus = Surface->GetCapabilities(Capabilities);
        if (SurfaceStatus != ESurfaceStatus::Ok)
        {
            VULKAN_WARNING("FVulkanSwapChainRHI::ValidateSurfaceAndSize Capabilities still not OK after surface recreation (status=%s)", ToString(SurfaceStatus));
            return false;
        }
    }
    else if (SurfaceStatus != ESurfaceStatus::Ok)
    {
        // ZeroSized or Error, don't proceed right now.
        return false;
    }

    if (!IsUndefinedExtent(Capabilities))
    {
        OutWidth  = Math::Max<uint32>(1u, Capabilities.currentExtent.width);
        OutHeight = Math::Max<uint32>(1u, Capabilities.currentExtent.height);
    }
    else
    {
        // Caller provided size must be non-zero
        OutWidth  = OutWidth ? OutWidth : Desc.Width;
        OutHeight = OutHeight ? OutHeight : Desc.Height;

        if (OutWidth == 0 || OutHeight == 0)
        {
            // Minimized or no drawable size
            return false; 
        }
    }

    return true;
}

bool FVulkanSwapChainRHI::CreateSwapChain(FVulkanCommandContext* InCommandContext, uint32 InWidth, uint32 InHeight)
{
    if (InWidth == 0 || InHeight == 0)
    {
        VULKAN_INFO("SwapChain Width or Height of zero is not supported (w=%u h=%u).", InWidth, InHeight);
        return false;
    }

    uint32 CreateWidth  = InWidth;
    uint32 CreateHeight = InHeight;
    
    if (!ValidateSurfaceAndSize(InCommandContext, CreateWidth, CreateHeight))
    {
        VULKAN_WARNING("Surface not ready or zero-sized.");
        return false;
    }

    FVulkanSwapChainCreateInfo SwapChainCreateInfo;
    SwapChainCreateInfo.ColorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    SwapChainCreateInfo.Surface           = Surface.Get();
    SwapChainCreateInfo.PreviousSwapChain = SwapChain.Get();
    SwapChainCreateInfo.BufferCount       = CVarBackbufferCount.GetValue();
    SwapChainCreateInfo.Extent.width      = CreateWidth;
    SwapChainCreateInfo.Extent.height     = CreateHeight;
    SwapChainCreateInfo.Format            = GetColorFormat();
    SwapChainCreateInfo.bVerticalSync     = CVarEnableVSync.GetValue();

    // NOTE: Create a temporary SwapChain, keeping old alive until success
    FVulkanSwapChainRef NewSwapChainResource = new FVulkanSwapChain(GetDevice());
    if (!NewSwapChainResource->Initialize(SwapChainCreateInfo))
    {
        VULKAN_ERROR_CRITICAL("Failed to create SwapChain");
        return false;
    }
    else
    {
        SwapChain = NewSwapChainResource;
    }

    // Update the description if the requested image size was not supported
    VkExtent2D SwapChainExtent = SwapChain->GetExtent();
    if (InWidth != SwapChainExtent.width || InHeight != SwapChainExtent.height)
    {
        VULKAN_WARNING("Requested size [w=%d, h=%d] was not supported, the actual size is [w=%d, h=%d]", Desc.Width, Desc.Height, SwapChainExtent.width, SwapChainExtent.height);
        
        // Update the size of the viewport to the actual swapchain size
        Desc.Width  = static_cast<uint16>(SwapChainExtent.width);
        Desc.Height = static_cast<uint16>(SwapChainExtent.height);

        if (BackBuffer)
        {
            BackBuffer->ResizeBackBuffer(Desc.Width, Desc.Height);
        }
    }

    // Initialize semaphores and BackBuffers
    const uint32 BufferCount = SwapChain->GetBufferCount();
    if (BufferCount != static_cast<uint32>(BackBuffers.Size()))
    {
        ImageFences.Resize(BufferCount);
        ImageSemaphores.Resize(BufferCount);
        RenderSemaphores.Resize(BufferCount);
        BackBuffers.Resize(BufferCount);

        // Setup the info for each back-buffer, and ensure that we create the info with the actual image-size
        const ETextureUsageFlags UsageFlags = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable;
        FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(GetColorFormat(), SwapChainExtent.width, SwapChainExtent.height, 1, 1, UsageFlags);
        
        // Create a FVulkanTextureRHI for each back-buffer
        for (uint32 i = 0; i < BufferCount; ++i)
        {
            // Create semaphores
            FVulkanSemaphoreRef NewImageSemaphore = new FVulkanSemaphore(GetDevice());
            if (NewImageSemaphore->Initialize())
            {
                NewImageSemaphore->SetDebugName("ImageSemaphore[" + TTypeToString<int32>::ToString(i) + "]");
                ImageSemaphores[i] = NewImageSemaphore;
            }
            else
            {
                return false;
            }

            FVulkanSemaphoreRef NewRenderSemaphore = new FVulkanSemaphore(GetDevice());
            if (NewRenderSemaphore->Initialize())
            {
                NewRenderSemaphore->SetDebugName("RenderSemaphore[" + TTypeToString<int32>::ToString(i) + "]");
                RenderSemaphores[i] = NewRenderSemaphore;
            }
            else
            {
                return false;
            }

            // Create image texture-wrapper
            if (FVulkanTextureRHIRef NewTexture = new FVulkanTextureRHI(GetDevice(), BackBufferDesc))
            {
                if (BackBufferDesc.bEnableResourceStateTracking)
                {
                    NewTexture->EnableStateTracking(EResourceAccess::Present);
                }

                BackBuffers[i] = NewTexture;
                
                if (FVulkanImageLayoutState* LayoutState = BackBuffers[i]->GetImageLayoutState())
                {
                    FVulkanImageLayoutState::FImageState PresentState;
                    PresentState.Layout = FVulkanRHI::ResourceStateToImageLayout(EResourceAccess::Present);
                    PresentState.Access = FVulkanRHI::ResourceStateToAccessFlags(EResourceAccess::Present);
                    PresentState.Stage  = FVulkanRHI::ResourceStateToPipelineStageFlags(EResourceAccess::Present);
                    LayoutState->SetState(PresentState);
                }
            }
            else
            {
                return false;
            }

            // Reset fence for this image
            ImageFences[i] = nullptr;
        }
    }
    else
    {
        // Buffer count unchanged. However, make sure we drop any stale per-image fences.
        for (uint32 i = 0; i < BufferCount; ++i)
        {
            ImageFences[i] = nullptr;
        }
    }

    // Retrieve the images
    TArray<VkImage> SwapChainImages(SwapChain->GetBufferCount());
    SwapChain->GetSwapChainImages(SwapChainImages.Data());

    // Ensure we are recording
    CHECK(InCommandContext->IsRecording());
    CHECK(!InCommandContext->NeedsCommandBuffer());
    
    int32 Index = 0;
    for (VkImage Image : SwapChainImages)
    {
        VkImageMemoryBarrier2 ImageBarrier = {};
        ImageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        ImageBarrier.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        ImageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        ImageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ImageBarrier.image                           = Image;
        ImageBarrier.srcAccessMask                   = VK_ACCESS_2_NONE;
        ImageBarrier.dstAccessMask                   = VK_ACCESS_2_NONE;
        ImageBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        ImageBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        ImageBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageBarrier.subresourceRange.baseArrayLayer = 0;
        ImageBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        ImageBarrier.subresourceRange.baseMipLevel   = 0;
        ImageBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        InCommandContext->GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);
        FVulkanTextureRHI* BackBufferTexture = BackBuffers[Index++].Get();
        BackBufferTexture->SetVkImage(Image);

        if (FVulkanImageLayoutState* LayoutState = BackBufferTexture->GetImageLayoutState())
        {
            FVulkanImageLayoutState::FImageState PresentState;
            PresentState.Layout = FVulkanRHI::ResourceStateToImageLayout(EResourceAccess::Present);
            PresentState.Access = FVulkanRHI::ResourceStateToAccessFlags(EResourceAccess::Present);
            PresentState.Stage  = FVulkanRHI::ResourceStateToPipelineStageFlags(EResourceAccess::Present);
            LayoutState->SetState(PresentState);
        }
    }

    InCommandContext->SplitCommandBuffer(false, false);

    // Reset indices
    SemaphoreIndex  = 0;
    BackBufferIndex = VULKAN_INVALID_BACK_BUFFER_INDEX;
    return true;
}

void FVulkanSwapChainRHI::DestroySwapChain(FVulkanCommandContext* InCommandContext)
{
    // Ensure that all work is completed
    InCommandContext->GetCommandQueue().WaitForCompletion();

    // Destroy the swapchain
    SwapChain.Reset();

    BackBufferIndex = VULKAN_INVALID_BACK_BUFFER_INDEX;
    SemaphoreIndex  = 0;
}

bool FVulkanSwapChainRHI::Resize(FVulkanCommandContext* InCommandContext, uint32 InWidth, uint32 InHeight)
{
    if ((InWidth != Desc.Width || InHeight != Desc.Height) && InWidth > 0 && InHeight > 0)
    {
        CHECK(!InCommandContext->IsInsideRenderPass());
        CHECK(InCommandContext->IsRecording());

        // Ensure that all work is completed. If this function is called from a RHICommandList the we do 
        // this "manually" since the context is already started and we need to ensure that there is a valid CommandBuffer
        InCommandContext->SplitCommandBuffer(false, true);
        VULKAN_INFO("FVulkanSwapChainRHI::Resize w=%d h=%d", InWidth, InHeight);

        if (!CreateSwapChain(InCommandContext, InWidth, InHeight))
        {
            VULKAN_WARNING("FVulkanSwapChainRHI::Resize FAILED");
            return false;
        }

        Desc.Width  = static_cast<uint16>(InWidth);
        Desc.Height = static_cast<uint16>(InHeight);
        BackBuffer->ResizeBackBuffer(Desc.Width, Desc.Height);
    }

    return true;
}

bool FVulkanSwapChainRHI::Present(FVulkanCommandContext* InCommandContext, bool bVerticalSync)
{
    // TODO: Recreate SwapChain based on V-Sync
    UNREFERENCED_VARIABLE(bVerticalSync);
   
    // If we don't have a drawable size, don't try to acquire/present
    if (Desc.Width == 0 || Desc.Height == 0 || !SwapChain)
    {
        return false;
    }

    VkResult Result = VK_SUCCESS;
    if (BackBufferIndex == VULKAN_INVALID_BACK_BUFFER_INDEX)
    {
        Result = AcquireNextImage(InCommandContext);
        if (Result != VK_SUCCESS)
        {
            VULKAN_INFO("FVulkanSwapChainRHI::Present [AcquireNextImage] SwapChain is %s", Result == VK_SUBOPTIMAL_KHR ? "Suboptimal" : "OutOfDate");

            if (Result != VK_SUBOPTIMAL_KHR)
            {
                // Try to recreate with current size (Recreate surface if needed)
                InCommandContext->SplitCommandBuffer(false, true);

                if (!CreateSwapChain(InCommandContext, Desc.Width, Desc.Height))
                {
                    return false;
                }

                // Try once more next frame
                return true;
            }
        }
    }

    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];
    if constexpr (GVulkanLogSemaphoreIndex)
    {
        VULKAN_INFO("FVulkanSwapChainRHI::Present SemaphoreIndex=%d", SemaphoreIndex);
    }

    Result = SwapChain->Present(InCommandContext->GetCommandQueue(), RenderSemaphore.Get());
    if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || Result == VK_ERROR_SURFACE_LOST_KHR)
    {
        VULKAN_INFO("FVulkanSwapChainRHI::Present [Present] SwapChain is %s", (Result == VK_SUBOPTIMAL_KHR ? "Suboptimal" :
                (Result == VK_ERROR_SURFACE_LOST_KHR ? "SurfaceLost" : "OutOfDate")));

        InCommandContext->SplitCommandBuffer(false, true);

        if (!CreateSwapChain(InCommandContext, GetWidth(), GetHeight()))
        {
            VULKAN_WARNING("FVulkanSwapChainRHI::Present CreateSwapChain Failed");
            return false;
        }
    }
    else if (Result != VK_SUCCESS)
    {
        VULKAN_ERROR_CRITICAL("FVulkanSwapChainRHI::Present vkQueuePresentKHR failed with %s.", ToString(Result));
        return false;
    }

    AdvanceSemaphoreIndex();
    BackBufferIndex = VULKAN_INVALID_BACK_BUFFER_INDEX;
    return true;
}

void FVulkanSwapChainRHI::SetDebugName(const FString& InName)
{
    // Name the swapchain object
    if (SwapChain)
    {
        VulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), *InName, SwapChain->GetVkSwapChain(), VK_OBJECT_TYPE_SWAPCHAIN_KHR);
        BackBuffer->SetDebugName("BackBuffer Proxy");

        // Name all the images
        for (int32 i = 0; i < BackBuffers.Size(); ++i)
        {
            const FString ImageName = InName + FString::CreateFormatted(" BackBuffer Image[%d]", i);
            BackBuffers[i]->SetDebugName(ImageName);
        }
    }
}

FVulkanTextureRHI* FVulkanSwapChainRHI::GetCurrentBackBuffer(FVulkanCommandContext* InCommandContext)
{
    if (!SwapChain || Desc.Width == 0 || Desc.Height == 0)
    {
        VULKAN_WARNING("FVulkanSwapChainRHI::GetCurrentBackBuffer SwapChain is Invalid");
        return nullptr;
    }

    if (BackBufferIndex == VULKAN_INVALID_BACK_BUFFER_INDEX)
    {
        VkResult Result = AcquireNextImage(InCommandContext);
        if (Result == VK_SUBOPTIMAL_KHR)
        {
            VULKAN_WARNING("FVulkanSwapChainRHI::GetCurrentBackBuffer SwapChain is Suboptimal");
        }
        
        if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
        {
            VULKAN_WARNING("FVulkanSwapChainRHI::GetCurrentBackBuffer SwapChain is OutOfDate");
            InCommandContext->SplitCommandBuffer(false, true);

            if (!CreateSwapChain(InCommandContext, Desc.Width, Desc.Height))
            {
                VULKAN_WARNING("FVulkanSwapChainRHI::GetCurrentBackBuffer SwapChain recreation failed");
                return nullptr;
            }

            return nullptr;
        }
    }
    
    return BackBuffers[BackBufferIndex].Get();
}

FRHITexture* FVulkanSwapChainRHI::GetBackBuffer() const
{
    return BackBuffer.Get();
}

VkResult FVulkanSwapChainRHI::AcquireNextImage(FVulkanCommandContext* InCommandContext)
{
    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];
    FVulkanSemaphoreRef ImageSemaphore  = ImageSemaphores[SemaphoreIndex];

    if constexpr (GVulkanLogSemaphoreIndex)
    {
        VULKAN_INFO("FVulkanSwapChainRHI::AcquireNextImage SemaphoreIndex=%d", SemaphoreIndex);
    }

    if (FVulkanFence* Fence = ImageFences[SemaphoreIndex])
    {
        ImageFences[SemaphoreIndex] = nullptr;

        Fence->Wait();
        Fence->Release();
    }

    VkResult Result = SwapChain->AcquireNextImage(ImageSemaphore.Get());
    if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    {
        VULKAN_ERROR("Failed to aquire SwapChain image");
        return Result;
    }

    InCommandContext->GetCommandQueue().AddWaitSemaphore(ImageSemaphore->GetVkSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    InCommandContext->GetCommandQueue().AddSignalSemaphore(RenderSemaphore->GetVkSemaphore());

    if (FVulkanFence* Fence = InCommandContext->GetSubmissionFence())
    {
        ImageFences[SemaphoreIndex] = Fence;
        Fence->AddRef();
    }

    // Update the BackBuffer index
    BackBufferIndex = SwapChain->GetBufferIndex();
    return Result;
}
