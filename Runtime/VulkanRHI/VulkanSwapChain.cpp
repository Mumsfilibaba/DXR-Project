#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanDeviceDebug.h"
#include "VulkanRHI/VulkanBackBufferProxies.h"

static TAutoConsoleVariable<int32> CVarBackbufferCount(
    "VulkanRHI.SwapChain.BackBufferCount",
    "The preferred number of backbuffers for the SwapChain",
    NUM_BACK_BUFFERS,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarEnableVSync(
    "VulkanRHI.SwapChain.EnableVSync",
    "Enable V-Sync for SwapChains (Changes take effect at the next present)",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarVulkanDefaultBackBufferFormat(
    "VulkanRHI.DefaultBackBufferFormat",
    "Default back-buffer format used when FRHISwapChainDesc::ColorFormat is Unknown. "
    "0=B8G8R8A8_Unorm (default), "
    "1=R8G8B8A8_Unorm, "
    "2=B8G8R8A8_Unorm_SRGB, "
    "3=R8G8B8A8_Unorm_SRGB, "
    "4=R10G10B10A2_Unorm (HDR10 candidate), "
    "5=R16G16B16A16_Float (scRGB candidate).",
    0);

static TAutoConsoleVariable<int32> CVarVulkanDefaultBackBufferColorSpace(
    "VulkanRHI.DefaultBackBufferColorSpace",
    "Default back-buffer color space used when FRHISwapChainDesc::ColorSpace is Unknown. "
    "0=RGB_Full_G22_None_P709 / sRGB (default), "
    "1=RGB_Full_G10_None_P709 / scRGB, "
    "2=RGB_Full_G2084_None_P2020 / HDR10, "
    "3=RGB_Full_G22_None_P2020.",
    0);

EFormat GetVulkanDefaultBackBufferFormat()
{
    static constexpr EFormat FormatTable[] =
    {
        EFormat::B8G8R8A8_Unorm,
        EFormat::R8G8B8A8_Unorm,
        EFormat::B8G8R8A8_Unorm_SRGB,
        EFormat::R8G8B8A8_Unorm_SRGB,
        EFormat::R10G10B10A2_Unorm,
        EFormat::R16G16B16A16_Float,
    };

    int32 Index = CVarVulkanDefaultBackBufferFormat.GetValue();
    if (Index < 0 || Index >= static_cast<int32>(ARRAY_COUNT(FormatTable)))
    {
        VULKAN_WARNING("VulkanRHI.DefaultBackBufferFormat=%d is out of range; clamping to 0.", Index);
        Index = 0;
    }

    return FormatTable[Index];
}

static EColorSpace GetVulkanDefaultBackBufferColorSpace()
{
    static constexpr EColorSpace ColorSpaceTable[] =
    {
        EColorSpace::RGB_Full_G22_None_P709,
        EColorSpace::RGB_Full_G10_None_P709,
        EColorSpace::RGB_Full_G2084_None_P2020,
        EColorSpace::RGB_Full_G22_None_P2020,
    };

    int32 Index = CVarVulkanDefaultBackBufferColorSpace.GetValue();
    if (Index < 0 || Index >= static_cast<int32>(ARRAY_COUNT(ColorSpaceTable)))
    {
        VULKAN_WARNING("VulkanRHI.DefaultBackBufferColorSpace=%d is out of range; clamping to 0.", Index);
        Index = 0;
    }

    return ColorSpaceTable[Index];
}

FVulkanSwapChain::FVulkanSwapChain(FVulkanDevice* InDevice)
	: FVulkanDeviceChild(InDevice)
	, PresentResult(VK_SUCCESS)
	, SwapChain(VK_NULL_HANDLE)
	, Extent{ 0, 0 }
	, BufferIndex(0)
	, BufferCount(0)
	, Format{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
	, GraphicsQueueFamilyIndex(InDevice ? InDevice->GetQueueIndexFromType(EVulkanCommandQueueType::Graphics) : 0)
	, PresentQueueFamilyIndex(InDevice ? InDevice->GetQueueIndexFromType(EVulkanCommandQueueType::Present) : 0)
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
		VULKAN_INFO("Selected Format='%s' Colorspace='%s' for SwapChain",
		            ToString(SelectedFormat.format),
		            ToString(SelectedFormat.colorSpace));
	}

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

	VkPresentModeKHR SelectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	if (CreateInfo.bVerticalSync)
	{
		// FIFO is true vsync (frame rate capped to display refresh, spec-guaranteed).
		// FIFO_RELAXED allows late frames to present immediately to reduce stuttering.
		if (HasPresentMode(VK_PRESENT_MODE_FIFO_KHR))
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
		// IMMEDIATE has no vsync and lowest latency.
		// MAILBOX replaces queued frames (triple-buffered, no frame-rate cap, reduced tearing).
		if (HasPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR))
		{
			SelectedPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
		else if (HasPresentMode(VK_PRESENT_MODE_MAILBOX_KHR))
		{
			SelectedPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
		else if (HasPresentMode(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
		{
			SelectedPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
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

	VULKAN_INFO("SwapChain Extent: current=(%u,%u) min=(%u,%u) max=(%u,%u) chosen=(%u,%u)", Capabilities.currentExtent.width, 
        Capabilities.currentExtent.height, Capabilities.minImageExtent.width, Capabilities.minImageExtent.height, 
        Capabilities.maxImageExtent.width, Capabilities.maxImageExtent.height, CurrentExtent.width, CurrentExtent.height);

	// Image count (respect min/max)
	uint32 DesiredCount = Math::Max<uint32>(CreateInfo.BufferCount, Capabilities.minImageCount);
    if (Capabilities.maxImageCount > 0)
    {
		DesiredCount = Math::Min(DesiredCount, Capabilities.maxImageCount);
    }

	if (DesiredCount != CreateInfo.BufferCount)
	{
		VULKAN_INFO("Adjusted buffer count from %u to %u (min=%u max=%u)", 
            CreateInfo.BufferCount, DesiredCount, Capabilities.minImageCount, Capabilities.maxImageCount);
	}

	GraphicsQueueFamilyIndex = GetDevice()->GetQueueIndexFromType(EVulkanCommandQueueType::Graphics);
	PresentQueueFamilyIndex  = GetDevice()->GetQueueIndexFromType(EVulkanCommandQueueType::Present);

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
	const VkImageUsageFlags RequestedUsage = ConvertSwapChainUsage(CreateInfo.Usage);
	const VkImageUsageFlags FinalUsage     = RequestedUsage & SupportedUsage;

	if (FinalUsage != RequestedUsage)
	{
		VULKAN_ERROR_CRITICAL("Surface does not support the requested ImageUsage flags (requested=0x%x, supported=0x%x)", RequestedUsage, SupportedUsage);
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
	SwapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
	SwapChainCreateInfo.queueFamilyIndexCount = 0;
	SwapChainCreateInfo.pQueueFamilyIndices   = nullptr;
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

void FVulkanSwapChain::ReleaseOwnershipForPresent(FVulkanCommandBuffer& GraphicsCmdBuffer, VkImage SwapChainImage)
{
	if (!HasSeparatePresentQueue())
	{
		return;
	}

	VkImageMemoryBarrier Barrier = {};
	Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	Barrier.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	Barrier.dstAccessMask                   = 0;
	Barrier.oldLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	Barrier.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	Barrier.srcQueueFamilyIndex             = GraphicsQueueFamilyIndex;
	Barrier.dstQueueFamilyIndex             = PresentQueueFamilyIndex;
	Barrier.image                           = SwapChainImage;
	Barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	Barrier.subresourceRange.baseMipLevel   = 0;
	Barrier.subresourceRange.levelCount     = 1;
	Barrier.subresourceRange.baseArrayLayer = 0;
	Barrier.subresourceRange.layerCount     = 1;

	GraphicsCmdBuffer->ImageMemoryPipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 1, &Barrier);
}

void FVulkanSwapChain::AcquireOwnershipAfterPresent(FVulkanCommandBuffer& GraphicsCmdBuffer, VkImage SwapChainImage)
{
	if (!HasSeparatePresentQueue())
	{
		return;
	}

	VkImageMemoryBarrier Barrier = {};
	Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	Barrier.srcAccessMask                   = 0;
	Barrier.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	Barrier.oldLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	Barrier.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	Barrier.srcQueueFamilyIndex             = PresentQueueFamilyIndex;
	Barrier.dstQueueFamilyIndex             = GraphicsQueueFamilyIndex;
	Barrier.image                           = SwapChainImage;
	Barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	Barrier.subresourceRange.baseMipLevel   = 0;
	Barrier.subresourceRange.levelCount     = 1;
	Barrier.subresourceRange.baseArrayLayer = 0;
	Barrier.subresourceRange.layerCount     = 1;

	GraphicsCmdBuffer->ImageMemoryPipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 1, &Barrier);
}

VkResult FVulkanSwapChain::Present(FVulkanQueue& GraphicsQueue, FVulkanQueue* PresentQueue, FVulkanSemaphore* WaitSemaphore)
{
	FVulkanQueue& QueueForPresent = PresentQueue ? *PresentQueue : GraphicsQueue;

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

	return vkQueuePresentKHR(QueueForPresent.GetVkQueue(), &PresentInfo);
}

VkResult FVulkanSwapChain::AcquireNextImage(FVulkanSemaphore* AcquireSemaphore)
{
	VkSemaphore Semaphore = AcquireSemaphore ? AcquireSemaphore->GetVkSemaphore() : VK_NULL_HANDLE;

	VkResult Result = vkAcquireNextImageKHR(GetDevice()->GetVkDevice(), SwapChain, UINT64_MAX, Semaphore, VK_NULL_HANDLE, &BufferIndex);
#if VULKAN_REPORT_SWAPCHAIN_ACQUIRE_IMAGE_NON_SUCCESS_RESULT
	if (Result != VK_SUCCESS)
	{
		VULKAN_WARNING("FVulkanSwapChain::AcquireNextImage vkAcquireNextImageKHR did not return VK_SUCCESS. Result = '%s'", ToString(Result));
	}
#endif
	
	// BufferIndex is only valid on SUCCESS/SUBOPTIMAL
	if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
	{
		BufferIndex = 0;
	}

	// Caller should treat OUT_OF_DATE -> recreate now, and SUBOPTIMAL -> recreate soon/skip frame.
	return Result;
}

FVulkanSwapChainRHI::FVulkanSwapChainRHI(FVulkanDevice* InDevice, FVulkanCommandContext* InCommandContext, const FRHISwapChainDesc& InSwapChainDesc)
    : FRHISwapChain(InSwapChainDesc)
    , FVulkanDeviceChild(InDevice)
    , WindowHandle(InSwapChainDesc.WindowHandle)
    , CommandContext(InCommandContext)
    , Surface(nullptr)
    , SwapChainResource(nullptr)
    , BackBufferProxy(nullptr)
    , BackBufferProxyRenderTargetView(nullptr)
    , BackBufferProxyUnorderedAccessView(nullptr)
    , BackBuffers()
    , ImageSemaphores()
    , RenderSemaphores()
    , CurrentColorSpace(EColorSpace::RGB_Full_G22_None_P709)
    , SemaphoreIndex(0)
    , BackBufferIndex(0)
    , ActiveBackBufferCount(0)
    , bActiveVSync(CVarEnableVSync.GetValue())
{
}

FVulkanSwapChainRHI::~FVulkanSwapChainRHI()
{
    DestroySwapChain();

    if (BackBufferProxy)
    {
        BackBufferProxy->SetSwapChain(nullptr);
    }

    if (BackBufferProxyRenderTargetView)
    {
        BackBufferProxyRenderTargetView->SetSwapChain(nullptr);
    }

    if (BackBufferProxyUnorderedAccessView)
    {
        BackBufferProxyUnorderedAccessView->SetSwapChain(nullptr);
    }
}

bool FVulkanSwapChainRHI::Initialize()
{
    if (!CommandContext)
    {
        VULKAN_ERROR_CRITICAL("CommandContext cannot be nullptr");
        return false;
    }
    
    Surface = new FVulkanSurface(GetDevice(), CommandContext->GetCommandQueue(), WindowHandle);
    if (!Surface->Initialize())
    {
        VULKAN_ERROR_CRITICAL("Failed to create Surface");
        return false;
    }

    // Resolve format / color space using the policy described on FRHISwapChainDesc.
    EFormat     RequestedFormat     = Desc.ColorFormat;
    EColorSpace RequestedColorSpace = Desc.ColorSpace;

    if (RequestedFormat == EFormat::Unknown)
    {
        RequestedFormat = GetVulkanDefaultBackBufferFormat();
    }

    if (RequestedColorSpace == EColorSpace::Unknown)
    {
        RequestedColorSpace = GetVulkanDefaultBackBufferColorSpace();
    }

    Desc.ColorFormat  = RequestedFormat;
    Desc.ColorSpace   = RequestedColorSpace;
    CurrentColorSpace = RequestedColorSpace;

    if (!Desc.IsRenderTarget() && !Desc.IsUnorderedAccess())
    {
        VULKAN_ERROR("FVulkanSwapChainRHI::Initialize: Desc.Usage must include at least one of ESwapChainUsageFlags::RenderTarget or ESwapChainUsageFlags::UnorderedAccess.");
        return false;
    }

    // Create the proxy texture/RTV up-front so that higher-level code always gets a stable handle.
    ETextureUsageFlags BackBufferUsageFlags = ETextureUsageFlags::Presentable;
    if (Desc.IsRenderTarget())
    {
        BackBufferUsageFlags |= ETextureUsageFlags::RenderTarget;
    }
    if (Desc.IsUnorderedAccess())
    {
        BackBufferUsageFlags |= ETextureUsageFlags::UnorderedAccessTexture;
    }

    const FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(Desc.ColorFormat, Desc.Width, Desc.Height, 1, 1, BackBufferUsageFlags);
    BackBufferProxy = new FVulkanBackBufferProxyTextureRHI(this, BackBufferDesc);

    if (!BackBufferProxy)
    {
        VULKAN_ERROR_CRITICAL("Failed to create BackBuffer proxy texture");
        return false;
    }

    if (Desc.IsRenderTarget())
    {
        BackBufferProxyRenderTargetView = new FVulkanBackBufferProxyRenderTargetViewRHI(this, BackBufferProxy.Get());
        BackBufferProxy->SetProxyRenderTargetView(BackBufferProxyRenderTargetView.Get());
    }

    if (Desc.IsUnorderedAccess())
    {
        BackBufferProxyUnorderedAccessView = new FVulkanBackBufferProxyUnorderedAccessViewRHI(this, BackBufferProxy.Get());
        BackBufferProxy->SetProxyUnorderedAccessView(BackBufferProxyUnorderedAccessView.Get());
    }
    
    // We need to start the context since that locks it to this thread
    CommandContext->StartContext();

    if (!CreateSwapChain(Desc.Width, Desc.Height))
    {
        return false;
    }

    const VkResult AcquireResult = AcquireNextImage();
    if (AcquireResult != VK_SUCCESS && AcquireResult != VK_SUBOPTIMAL_KHR)
    {
        VULKAN_ERROR("FVulkanSwapChainRHI::Initialize initial AcquireNextImage failed (%s)", ToString(AcquireResult));
        CommandContext->FinishContext();
        CommandContext->Flush();
        return false;
    }
    
    // Unlock the context from this thread
    CommandContext->FinishContext();
    CommandContext->Flush();

    return true;
}

bool FVulkanSwapChainRHI::RecreateSurface()
{
	Surface = new FVulkanSurface(GetDevice(), CommandContext->GetCommandQueue(), WindowHandle);
	if (!Surface->Initialize())
	{
		VULKAN_WARNING("RecreateSurface: failed to initialize surface");
		return false;
	}

	return true;
}

bool FVulkanSwapChainRHI::ValidateSurfaceAndSize(uint32& OutWidth, uint32& OutHeight)
{
	VkSurfaceCapabilitiesKHR Capabilities = { };

	ESurfaceStatus SurfaceStatus = Surface->GetCapabilities(Capabilities);
	if (SurfaceStatus == ESurfaceStatus::SurfaceLost)
	{
		VULKAN_INFO("FVulkanSwapChainRHI::ValidateSurfaceAndSize Surface lost. Attempting one recreation.");
        if (!RecreateSurface())
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

bool FVulkanSwapChainRHI::CreateSwapChain(uint32 InWidth, uint32 InHeight)
{
	if (InWidth == 0 || InHeight == 0)
	{
		VULKAN_INFO("SwapChain Width or Height of zero is not supported (w=%u h=%u).", InWidth, InHeight);
		return false;
	}

	uint32 CreateWidth  = InWidth;
	uint32 CreateHeight = InHeight;

	if (!ValidateSurfaceAndSize(CreateWidth, CreateHeight))
	{
		VULKAN_WARNING("Surface not ready or zero-sized.");
		return false;
	}

	FVulkanSwapChainCreateInfo SwapChainCreateInfo;
	SwapChainCreateInfo.ColorSpace        = ConvertColorSpace(CurrentColorSpace);
	SwapChainCreateInfo.Surface           = Surface.Get();
	SwapChainCreateInfo.PreviousSwapChain = SwapChainResource.Get();
	SwapChainCreateInfo.BufferCount       = CVarBackbufferCount.GetValue();
	SwapChainCreateInfo.Extent.width      = CreateWidth;
	SwapChainCreateInfo.Extent.height     = CreateHeight;
	SwapChainCreateInfo.Format            = Desc.ColorFormat;
	SwapChainCreateInfo.Usage             = Desc.Usage;
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
		SwapChainResource = NewSwapChainResource;
	}

	bActiveVSync          = SwapChainCreateInfo.bVerticalSync;
	ActiveBackBufferCount = SwapChainCreateInfo.BufferCount;

    // Update the description if the requested image size was not supported
    VkExtent2D SwapChainExtent = SwapChainResource->GetExtent();
    if (InWidth != SwapChainExtent.width || InHeight != SwapChainExtent.height)
    {
        VULKAN_WARNING("Requested size [w=%d, h=%d] was not supported, the actual size is [w=%d, h=%d]", 
			Desc.Width, Desc.Height, SwapChainExtent.width, SwapChainExtent.height);
        
        // Update the size of the viewport to the actual swapchain size
        Desc.Width  = static_cast<uint16>(SwapChainExtent.width);
        Desc.Height = static_cast<uint16>(SwapChainExtent.height);

        if (BackBufferProxy)
        {
            BackBufferProxy->Resize(Desc.Width, Desc.Height);
        }
    }

    // Per-image binary semaphores must always be recreated so the post-resize AcquireNextImage
    // populates WAIT/SIGNAL on fresh unsignaled VkSemaphores and can't collide with any
    // pre-resize signaled state. Callers are expected to have idled the GPU and drained
    // pending WAIT/SIGNAL entries from the queue before reaching this point.
    const uint32 BufferCount = SwapChainResource->GetBufferCount();
    ImageSemaphores.Resize(BufferCount);
    RenderSemaphores.Resize(BufferCount);

    for (uint32 i = 0; i < BufferCount; ++i)
    {
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
    }

    // Per-image texture wrappers must be recreated on every (re)create so their Desc.Extent and
    // CreateInfo.extent reflect the current swap-chain extent. Reusing the wrappers and only
    // swapping the VkImage via SetVkImage would leave GetWidth/GetHeight reporting the previous
    // extent, which propagates into BeginRenderPass's renderArea and trips Vulkan validation.
    ImageFences.Resize(BufferCount);
    BackBuffers.Resize(BufferCount);

    ETextureUsageFlags UsageFlags = ETextureUsageFlags::Presentable;
    if (Desc.IsRenderTarget())
    {
        UsageFlags |= ETextureUsageFlags::RenderTarget;
    }
    if (Desc.IsUnorderedAccess())
    {
        UsageFlags |= ETextureUsageFlags::UnorderedAccessTexture;
    }
    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(Desc.ColorFormat, SwapChainExtent.width, SwapChainExtent.height, 1, 1, UsageFlags);

    for (uint32 i = 0; i < BufferCount; ++i)
    {
        if (FVulkanTextureRHIRef NewTexture = new FVulkanTextureRHI(GetDevice(), BackBufferDesc))
        {
            BackBuffers[i].Texture = NewTexture;
        }
        else
        {
            return false;
        }

        ImageFences[i] = nullptr;
    }

    // Retrieve the images
    TArray<VkImage> SwapChainImages(SwapChainResource->GetBufferCount());
    SwapChainResource->GetSwapChainImages(SwapChainImages.Data());

    // Ensure we are recording
    CHECK(CommandContext->IsRecording());
    CHECK(!CommandContext->NeedsCommandBuffer());

    for (FBackBufferData& Data : BackBuffers)
    {
        Data.RenderTargetView    = nullptr;
        Data.UnorderedAccessView = nullptr;
    }

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

        CommandContext->GetBarrierBatcher().AddImageMemoryBarrier(0, ImageBarrier);
        BackBuffers[Index].Texture->SetVkImage(Image);

        FVulkanTextureRHI* BackBufferTexture = BackBuffers[Index].Texture.Get();

        if (Desc.IsRenderTarget())
        {
            const FRHIRenderTargetViewDesc RTVDesc = FRHIRenderTargetViewDesc::CreateTexture2D(Desc.ColorFormat, 0);

            FVulkanRenderTargetViewRHIRef NewRTV = new FVulkanRenderTargetViewRHI(GetDevice(), BackBufferTexture, RTVDesc);
            if (!NewRTV->Initialize(BackBufferTexture, RTVDesc))
            {
                VULKAN_ERROR_CRITICAL("FVulkanSwapChainRHI: Failed to create back-buffer RTV for index %d", Index);
                return false;
            }

            BackBuffers[Index].RenderTargetView = NewRTV;
        }

        if (Desc.IsUnorderedAccess())
        {
            const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(Desc.ColorFormat, 0);

            FVulkanUnorderedAccessViewRHIRef NewUAV = new FVulkanUnorderedAccessViewRHI(GetDevice(), BackBufferTexture, UAVDesc);
            if (!NewUAV->Initialize(BackBufferTexture, UAVDesc))
            {
                VULKAN_ERROR_CRITICAL("FVulkanSwapChainRHI: Failed to create back-buffer UAV for index %d", Index);
                return false;
            }

            BackBuffers[Index].UnorderedAccessView = NewUAV;
        }

        ++Index;
    }

    CommandContext->SplitCommandBuffer(false, false);

	// Reset indices; AcquireNextImage callers will populate BackBufferIndex.
	SemaphoreIndex  = 0;
	BackBufferIndex = 0;
    return true;
}

void FVulkanSwapChainRHI::DestroySwapChain()
{
    if (!CommandContext)
    {
        return;
    }

    // Ensure that all work is completed
    CommandContext->GetCommandQueue().WaitForCompletion();

    // Destroy the swapchain
    SwapChainResource.Reset();

	BackBufferIndex = 0;
	SemaphoreIndex  = 0;
}

bool FVulkanSwapChainRHI::Resize(uint32 InWidth, uint32 InHeight, EFormat NewFormat, EColorSpace NewColorSpace)
{
    // -------------------------------------------------------------------------------------------
    // Resolve effective values - 0 / Unknown means "keep current".
    // -------------------------------------------------------------------------------------------
    const uint32      ResolvedWidth       = (InWidth  > 0u) ? InWidth  : Desc.Width;
    const uint32      ResolvedHeight      = (InHeight > 0u) ? InHeight : Desc.Height;
    const EFormat     EffectiveFormat     = (NewFormat     == EFormat::Unknown)     ? Desc.ColorFormat   : NewFormat;
    const EColorSpace EffectiveColorSpace = (NewColorSpace == EColorSpace::Unknown) ? CurrentColorSpace  : NewColorSpace;

    const bool bSizeChanged       = (ResolvedWidth != Desc.Width || ResolvedHeight != Desc.Height) && ResolvedWidth > 0 && ResolvedHeight > 0;
    const bool bFormatChanged     = (NewFormat     != EFormat::Unknown)     && (EffectiveFormat     != Desc.ColorFormat);
    const bool bColorSpaceChanged = (NewColorSpace != EColorSpace::Unknown) && (EffectiveColorSpace != CurrentColorSpace);

    if (!bSizeChanged && !bFormatChanged && !bColorSpaceChanged)
    {
        return true;
    }

    // -------------------------------------------------------------------------------------------
    // Fail-fast on an unsupported (Format, ColorSpace) combination before we touch GPU state.
    // -------------------------------------------------------------------------------------------
    if (bFormatChanged || bColorSpaceChanged)
    {
        if (!IsFormatSupported(EffectiveFormat, EffectiveColorSpace))
        {
            VULKAN_ERROR("FVulkanSwapChainRHI::Resize: requested (%s, %s) not supported by this swap-chain.",
                         ToString(EffectiveFormat), ToString(EffectiveColorSpace));
            return false;
        }
    }

    CHECK(!CommandContext->IsInsideRenderPass());
    CHECK(CommandContext->IsRecording());

    // Ensure that all work is completed. If this function is called from a RHICommandList we do
    // this "manually" since the context is already started and we need to ensure that there is a
    // valid CommandBuffer.
    CommandContext->SplitCommandBuffer(false, true);

    // The end-of-Present eager AcquireNextImage queues WAIT IS[k] / SIGNAL RS[k] on the queue's
    // pending semaphore lists. When the CB before Resize is empty those pending entries are not
    // drained by the SplitCommandBuffer above, and would otherwise be flushed by the barrier
    // submit inside CreateSwapChain without a matching wait. Drop them now; the GPU is idle and
    // the referenced VkSemaphores are about to be destroyed and recreated.
    CommandContext->GetCommandQueue().ClearPendingSemaphores();

    VULKAN_INFO("FVulkanSwapChainRHI::Resize Width=%u Height=%u Format=%s Colorspace=%s",
        ResolvedWidth, ResolvedHeight, ToString(ConvertFormat(EffectiveFormat)), ToString(ConvertColorSpace(EffectiveColorSpace)));

    // Capture the new resolved format / color space BEFORE CreateSwapChain so the create info uses
    // them. The size is applied below from the actual VkSurfaceCapabilitiesKHR-clamped extent.
    if (bFormatChanged)
    {
        Desc.ColorFormat = EffectiveFormat;
    }

    if (bColorSpaceChanged)
    {
        CurrentColorSpace = EffectiveColorSpace;
        Desc.ColorSpace   = EffectiveColorSpace;
    }

    if (!CreateSwapChain(ResolvedWidth, ResolvedHeight))
    {
        VULKAN_WARNING("FVulkanSwapChainRHI::Resize FAILED");
        return false;
    }

    // CreateSwapChain may have clamped the extent to the surface capabilities; reflect that.
    if (BackBufferProxy)
    {
        BackBufferProxy->Resize(Desc.Width, Desc.Height);
    }

    // Eagerly acquire the first image after a resize so the proxy stays resolved.
    const VkResult AcquireResult = AcquireNextImage();
    if (AcquireResult != VK_SUCCESS && AcquireResult != VK_SUBOPTIMAL_KHR)
    {
        VULKAN_WARNING("FVulkanSwapChainRHI::Resize AcquireNextImage after resize failed (%s)", ToString(AcquireResult));
        return false;
    }

    return true;
}

bool FVulkanSwapChainRHI::Present(bool bVerticalSync)
{
	// If we don't have a drawable size, don't try to acquire/present
    if (Desc.Width == 0 || Desc.Height == 0 || !SwapChainResource)
    {
		return false;
    }

    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];
#if VULKAN_LOG_SEMAPHORE_INDEX
	VULKAN_INFO("FVulkanSwapChainRHI::Present SemaphoreIndex=%d", SemaphoreIndex);
#endif

    bool bNeedsRecreation = false;

    FVulkanQueue* PresentQueue = FVulkanDeviceRHI::Get()->GetPresentQueue();
    VkResult Result = SwapChainResource->Present(CommandContext->GetCommandQueue(), PresentQueue, RenderSemaphore.Get());
    if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || Result == VK_ERROR_SURFACE_LOST_KHR)
    {
		VULKAN_INFO("FVulkanSwapChainRHI::Present [Present] SwapChain is %s", 
			(Result == VK_SUBOPTIMAL_KHR ? "Suboptimal" :
			(Result == VK_ERROR_SURFACE_LOST_KHR ? "SurfaceLost" : "OutOfDate")));
        bNeedsRecreation = true;
    }
	else if (Result != VK_SUCCESS)
	{
		VULKAN_ERROR_CRITICAL("FVulkanSwapChainRHI::Present vkQueuePresentKHR failed with %s.", ToString(Result));
		return false;
	}

    // Detect runtime settings changes that require swapchain recreation
    if (bVerticalSync != bActiveVSync)
    {
        VULKAN_INFO("FVulkanSwapChainRHI::Present VSync changed (%s -> %s)", bActiveVSync ? "on" : "off", bVerticalSync ? "on" : "off");
        CVarEnableVSync->SetAsBool(bVerticalSync, EConsoleVariableFlags::SetByCode);
        bNeedsRecreation = true;
    }

    const int32 DesiredBackBufferCount = CVarBackbufferCount.GetValue();
    if (DesiredBackBufferCount != ActiveBackBufferCount)
    {
        VULKAN_INFO("FVulkanSwapChainRHI::Present BackBuffer count changed (%d -> %d)", ActiveBackBufferCount, DesiredBackBufferCount);
        bNeedsRecreation = true;
    }

    if (CVarEnableVSync.GetValue() != bActiveVSync)
    {
        VULKAN_INFO("FVulkanSwapChainRHI::Present VSync CVar changed (%s -> %s)", bActiveVSync ? "on" : "off", CVarEnableVSync.GetValue() ? "on" : "off");
        bNeedsRecreation = true;
    }

    if (bNeedsRecreation)
    {
        CommandContext->SplitCommandBuffer(false, true);

        // See the matching drain in FVulkanSwapChainRHI::Resize. The pending WAIT/SIGNAL entries
        // reference the old per-image semaphores that CreateSwapChain is about to destroy and
        // recreate; dropping them here prevents the barrier submit inside CreateSwapChain from
        // signaling a semaphore without a matching wait.
        CommandContext->GetCommandQueue().ClearPendingSemaphores();

        if (!CreateSwapChain(Desc.Width, Desc.Height))
        {
            VULKAN_WARNING("FVulkanSwapChainRHI::Present CreateSwapChain Failed");
            return false;
        }
    }

    AdvanceSemaphoreIndex();

    // Eagerly acquire the next image for the following frame so callers can query proxies
    // (GetBackBuffer / GetBackBufferRenderTargetView) and receive valid per-image resources
    // immediately, without requiring a deferred-acquire path.
    const VkResult AcquireResult = AcquireNextImage();
    if (AcquireResult != VK_SUCCESS && AcquireResult != VK_SUBOPTIMAL_KHR)
    {
        VULKAN_WARNING("FVulkanSwapChainRHI::Present AcquireNextImage for next frame failed (%s)", ToString(AcquireResult));
    }

    return true;
}

void FVulkanSwapChainRHI::SetDebugName(const String& InName)
{
    // Name the swapchain object
    if (SwapChainResource)
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), *InName, SwapChainResource->GetVkSwapChain(), VK_OBJECT_TYPE_SWAPCHAIN_KHR);

        // Name all the images
        for (int32 i = 0; i < BackBuffers.Size(); ++i)
        {
            const String ImageName = InName + String::CreateFormatted(" BackBuffer Image[%d]", i);
            BackBuffers[i].Texture->SetDebugName(ImageName);
        }
    }
}

FVulkanTextureRHI* FVulkanSwapChainRHI::GetCurrentBackBuffer() const
{
    if (!SwapChainResource || Desc.Width == 0 || Desc.Height == 0)
    {
        return nullptr;
    }

    if (!BackBuffers.IsValidIndex(static_cast<int32>(BackBufferIndex)))
    {
        return nullptr;
    }

    return BackBuffers[BackBufferIndex].Texture.Get();
}

FVulkanRenderTargetViewRHI* FVulkanSwapChainRHI::GetCurrentBackBufferRenderTargetView() const
{
    if (!BackBuffers.IsValidIndex(static_cast<int32>(BackBufferIndex)))
    {
        return nullptr;
    }

    return BackBuffers[BackBufferIndex].RenderTargetView.Get();
}

FVulkanUnorderedAccessViewRHI* FVulkanSwapChainRHI::GetCurrentBackBufferUnorderedAccessView() const
{
    if (!BackBuffers.IsValidIndex(static_cast<int32>(BackBufferIndex)))
    {
        return nullptr;
    }
    
    return BackBuffers[BackBufferIndex].UnorderedAccessView.Get();
}

void* FVulkanSwapChainRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(SwapChainResource->GetVkSwapChain());
}

void* FVulkanSwapChainRHI::GetRHINativeBackBufferResourceFromIndex(uint32 Index) const
{
    FVulkanTextureRHI* Texture = GetBackBufferAtIndex(Index);
    return Texture ? Texture->GetRHINativeResource() : nullptr;
}

void* FVulkanSwapChainRHI::GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 Index) const
{
    FVulkanRenderTargetViewRHI* View = GetBackBufferRenderTargetViewAtIndex(Index);
    return View ? View->GetRHINativeHandle() : nullptr;
}

void* FVulkanSwapChainRHI::GetRHINativeBackBufferUnorderedAccessViewFromIndex(uint32 Index) const
{
    FVulkanUnorderedAccessViewRHI* View = GetBackBufferUnorderedAccessViewAtIndex(Index);
    return View ? View->GetRHINativeHandle() : nullptr;
}

FRHITexture* FVulkanSwapChainRHI::GetBackBuffer() const
{
    return BackBufferProxy.Get();
}

FRHITexture* FVulkanSwapChainRHI::GetBackBufferResourceFromIndex(uint32 Index) const
{
    return GetBackBufferAtIndex(Index);
}

uint32 FVulkanSwapChainRHI::GetNumBackBufferResources() const
{
    return GetNumBackBuffers();
}

FRHIRenderTargetView* FVulkanSwapChainRHI::GetBackBufferRenderTargetView() const
{
    return BackBufferProxyRenderTargetView.Get();
}

FRHIUnorderedAccessView* FVulkanSwapChainRHI::GetBackBufferUnorderedAccessView() const
{
    return BackBufferProxyUnorderedAccessView.Get();
}

bool FVulkanSwapChainRHI::IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const
{
    if (Format == EFormat::Unknown || ColorSpace == EColorSpace::Unknown)
    {
        return false;
    }

    if (!Surface)
    {
        return false;
    }

    TArray<VkSurfaceFormatKHR> SupportedFormats;
    if (Surface->GetSupportedFormats(SupportedFormats) != ESurfaceStatus::Ok)
    {
        return false;
    }

    const VkFormat        TargetFormat     = ConvertFormat(Format);
    const VkColorSpaceKHR TargetColorSpace = ConvertColorSpace(ColorSpace);

    for (const VkSurfaceFormatKHR& SupportedFormat : SupportedFormats)
    {
        if (SupportedFormat.format == TargetFormat && SupportedFormat.colorSpace == TargetColorSpace)
        {
            return true;
        }
    }
    
    return false;
}

VkResult FVulkanSwapChainRHI::AcquireNextImage()
{
	FVulkanSemaphoreRef ImageSemaphore  = ImageSemaphores[SemaphoreIndex];
    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];

#if VULKAN_LOG_SEMAPHORE_INDEX
    VULKAN_INFO("FVulkanSwapChainRHI::AcquireNextImage SemaphoreIndex=%d", SemaphoreIndex);
#endif

	if (FVulkanFence* Fence = ImageFences[SemaphoreIndex])
	{
        ImageFences[SemaphoreIndex] = nullptr;

        Fence->Wait();
        Fence->Release();
	}

    VkResult Result = SwapChainResource->AcquireNextImage(ImageSemaphore.Get());
    if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    {
        VULKAN_ERROR("Failed to aquire SwapChain image");
        return Result;
    }

    CommandContext->GetCommandQueue().AddWaitSemaphore(ImageSemaphore->GetVkSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    CommandContext->GetCommandQueue().AddSignalSemaphore(RenderSemaphore->GetVkSemaphore());

    if (FVulkanFence* Fence = CommandContext->GetSubmissionFence())
    {
        ImageFences[SemaphoreIndex] = Fence;
        Fence->AddRef();
    }

    // Update the BackBuffer index
    BackBufferIndex = SwapChainResource->GetBufferIndex();
    return Result;
}
