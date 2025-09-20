#include "VulkanRHI/VulkanSwapChainResource.h"

static constexpr bool GVulkanReportSwapChainAquireImageNonSuccessResult = true;

FVulkanSwapChainResource::FVulkanSwapChainResource(FVulkanDevice* InDevice)
	: FVulkanDeviceChild(InDevice)
	, PresentResult(VK_SUCCESS)
	, SwapChain(VK_NULL_HANDLE)
	, Extent{ 0, 0 }
	, BufferIndex(0)
	, BufferCount(0)
	, Format{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
{
}

FVulkanSwapChainResource::~FVulkanSwapChainResource()
{
	if (VULKAN_CHECK_HANDLE(SwapChain))
	{
		vkDestroySwapchainKHR(GetDevice()->GetVkDevice(), SwapChain, nullptr);
		SwapChain = VK_NULL_HANDLE;
	}

	BufferIndex = 0;
}

bool FVulkanSwapChainResource::Initialize(const FVulkanSwapChainCreateInfo& CreateInfo)
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

	const VkSurfaceFormatKHR DesiredFormat = { ConvertFormat(CreateInfo.Format), CreateInfo.ColorSpace };
	
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
	const auto PickCompositeAlpha = [](VkCompositeAlphaFlagsKHR Supported) -> VkCompositeAlphaFlagBitsKHR
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
	};

	const VkCompositeAlphaFlagBitsKHR CompositeAlpha = PickCompositeAlpha(Capabilities.supportedCompositeAlpha);

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
	SwapChainCreateInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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

bool FVulkanSwapChainResource::GetSwapChainImages(VkImage* OutImages)
{
	VkResult Result = vkGetSwapchainImagesKHR(GetDevice()->GetVkDevice(), SwapChain, &BufferCount, OutImages);
	if (VULKAN_FAILED(Result))
	{
		VULKAN_ERROR_CRITICAL("Failed to retrieve the images of the SwapChain (%s)", ToString(Result));
		return false;
	}

	return true;
}

VkResult FVulkanSwapChainResource::Present(FVulkanQueue& Queue, FVulkanSemaphore* WaitSemaphore)
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

VkResult FVulkanSwapChainResource::AcquireNextImage(FVulkanSemaphore* AquireSemaphore)
{
	VkSemaphore Semaphore = AquireSemaphore ? AquireSemaphore->GetVkSemaphore() : VK_NULL_HANDLE;

	VkResult Result = vkAcquireNextImageKHR(GetDevice()->GetVkDevice(), SwapChain, UINT64_MAX, Semaphore, VK_NULL_HANDLE, &BufferIndex);
	if (GVulkanReportSwapChainAquireImageNonSuccessResult && Result != VK_SUCCESS)
	{
		VULKAN_WARNING("FVulkanSwapChainResource::AquireNextImage vkAcquireNextImageKHR did not return VK_SUCCESS. Result = '%s'", ToString(Result));
	}

	// Caller should treat OUT_OF_DATE -> recreate now, and SUBOPTIMAL -> recreate soon/skip frame.
	return Result;
}
