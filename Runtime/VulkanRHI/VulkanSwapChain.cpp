#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanCommandBuffer.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanCore.h"
#include "VulkanRHI/VulkanDeviceDebug.h"

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

static void DestroyBackBufferImageView(FVulkanDevice* Device, VkImageView& ImageView)
{
    if (VULKAN_CHECK_HANDLE(ImageView))
    {
    #if VULKAN_ENABLE_NON_DYNAMIC_RENDERING_PATH
        Device->GetRenderPassCache().OnReleaseImageView(ImageView);
    #endif

        vkDestroyImageView(Device->GetVkDevice(), ImageView, nullptr);
        ImageView = VK_NULL_HANDLE;
    }
}

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
	, CompositeAlpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
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
	const VkCompositeAlphaFlagBitsKHR SelectedCompositeAlpha = [](VkCompositeAlphaFlagsKHR Supported, bool bIsTransparent) -> VkCompositeAlphaFlagBitsKHR
	{
		const VkCompositeAlphaFlagBitsKHR TransparentPreferences[] =
		{
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
		};

		if (bIsTransparent)
		{
			for (VkCompositeAlphaFlagBitsKHR CurrentFlag : TransparentPreferences)
			{
				if (Supported & CurrentFlag)
				{
					return CurrentFlag;
				}
			}
		}

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
	}(Capabilities.supportedCompositeAlpha, CreateInfo.bIsTransparent);

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
	SwapChainCreateInfo.compositeAlpha        = SelectedCompositeAlpha;
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

	Extent         = CurrentExtent;
	Format         = SelectedFormat;
	CompositeAlpha = SelectedCompositeAlpha;
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

	return QueueForPresent.Present(PresentInfo);
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
    , BackBuffer(nullptr)
    , BackBufferImages()
    , ImageSemaphores()
    , RenderSemaphores()
    , PendingAcquireSemaphore(nullptr)
    , PendingRenderSemaphore(nullptr)
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

    if (!CreateBackBuffer())
    {
        return false;
    }

    CommandContext->StartContext();

    if (!CreateSwapChain(Desc.Width, Desc.Height))
    {
        CommandContext->FinishContext();
        CommandContext->Flush();
        return false;
    }

    CommandContext->FinishContext();
    CommandContext->Flush();

    if (CurrentColorSpace == EColorSpace::RGB_Full_G2084_None_P2020 && !Desc.HDRMetadata.bIsValid)
    {
        FRHIHDRMetadata DefaultMetadata = RHI::GetDefaultHDRMetadata();

        FRHIDisplayHDRInfo DisplayInfo;
        if (RHI::ShouldUseDisplayLuminance() && QueryDisplayHDRInfo(DisplayInfo))
        {
            DefaultMetadata.MinMasteringLuminance     = DisplayInfo.MinLuminance;
            DefaultMetadata.MaxMasteringLuminance     = DisplayInfo.MaxLuminance;
            DefaultMetadata.MaxFrameAverageLightLevel = DisplayInfo.MaxFullFrameLuminance;
        }

        SetHDRMetadata(DefaultMetadata);
    }

    return true;
}

bool FVulkanSwapChainRHI::CreateBackBuffer()
{
    ETextureUsageFlags BackBufferUsageFlags = ETextureUsageFlags::Presentable;

    if (Desc.IsRenderTarget())
    {
        BackBufferUsageFlags |= ETextureUsageFlags::RenderTarget;
    }

    if (Desc.IsUnorderedAccess())
    {
        BackBufferUsageFlags |= ETextureUsageFlags::UnorderedAccessTexture;
    }

    if (Desc.IsShaderResource())
    {
        BackBufferUsageFlags |= ETextureUsageFlags::ShaderResourceTexture;
    }

    const FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(Desc.ColorFormat, Desc.Width, Desc.Height, 1, 1, BackBufferUsageFlags);
    BackBuffer = new FVulkanTextureRHI(GetDevice(), BackBufferDesc);

    if (!BackBuffer)
    {
        VULKAN_ERROR_CRITICAL("Failed to create the BackBuffer texture");
        return false;
    }

    return true;
}

VkImageView FVulkanSwapChainRHI::CreateBackBufferImageView(VkImage InImage) const
{
    const VkFormat BackBufferFormat = ConvertFormat(Desc.ColorFormat);

    VkImageViewCreateInfo ImageViewCreateInfo = {};
    ImageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.image                           = InImage;
    ImageViewCreateInfo.format                          = BackBufferFormat;
    ImageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    ImageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    ImageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    ImageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    ImageViewCreateInfo.subresourceRange.aspectMask     = GetImageAspectFlagsFromFormat(BackBufferFormat);
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount     = 1;
    ImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    ImageViewCreateInfo.subresourceRange.levelCount     = 1;

    VkImageView ImageView = VK_NULL_HANDLE;

    const VkResult Result = vkCreateImageView(GetDevice()->GetVkDevice(), &ImageViewCreateInfo, nullptr, &ImageView);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("vkCreateImageView failed for a BackBuffer image");
        return VK_NULL_HANDLE;
    }

    return ImageView;
}

void FVulkanSwapChainRHI::DestroyBackBufferImageViews()
{
    for (FBackBufferImage& BackBufferImage : BackBufferImages)
    {
        DestroyBackBufferImageView(GetDevice(), BackBufferImage.ImageView);
    }

    BackBufferImages.Clear();
}

bool FVulkanSwapChainRHI::CreateBackBufferImages()
{
    TArray<VkImage> SwapChainImages(SwapChainResource->GetBufferCount());
    if (!SwapChainResource->GetSwapChainImages(SwapChainImages.Data()))
    {
        VULKAN_ERROR_CRITICAL("Failed to retrieve the SwapChain images");
        return false;
    }

    DestroyBackBufferImageViews();
    BackBufferImages.Resize(SwapChainImages.Size());

    for (int32 Index = 0; Index < SwapChainImages.Size(); ++Index)
    {
        FBackBufferImage& BackBufferImage = BackBufferImages[Index];
        BackBufferImage.Image     = SwapChainImages[Index];
        BackBufferImage.ImageView = CreateBackBufferImageView(BackBufferImage.Image);

        if (!VULKAN_CHECK_HANDLE(BackBufferImage.ImageView))
        {
            return false;
        }
    }

    const VkExtent2D SwapChainExtent = SwapChainResource->GetExtent();
    BackBuffer->SetSwapChainImage(BackBufferImages[0].Image, Desc.ColorFormat, SwapChainExtent.width, SwapChainExtent.height);

    if (!BackBuffer->InitializeSwapChainTexture())
    {
        return false;
    }

    SwapResources(0);
    return true;
}

void FVulkanSwapChainRHI::ReleaseBackBufferImages()
{
    if (BackBufferImages.IsEmpty())
    {
        return;
    }

    // A barrier names its image only when the batch reaches a command buffer, and it reads that image
    // back out of the texture at that point. Anything still holding one of these handles once the
    // swapchain is gone therefore records a barrier against an image that no longer exists.
    CommandContext->GetCommandQueue().WaitForCompletion();

    ReleaseResources();
    DestroyBackBufferImageViews();
}

void FVulkanSwapChainRHI::SwapResources(uint32 Index)
{
    if (!BackBufferImages.IsValidIndex(static_cast<int32>(Index)) || !BackBuffer)
    {
        return;
    }

    const FBackBufferImage& BackBufferImage = BackBufferImages[Index];

    BackBuffer->UpdateImage(BackBufferImage.Image);
    BackBuffer->GetImageLayoutState().SetImageLayout(VK_IMAGE_LAYOUT_TO_BE_DETERMINED);

    if (FVulkanRenderTargetViewRHI* View = BackBuffer->RenderTargetView.Get())
    {
        View->UpdateImageView(BackBufferImage.Image, BackBufferImage.ImageView);
    }

    if (FVulkanUnorderedAccessViewRHI* View = BackBuffer->UnorderedAccessView.Get())
    {
        View->UpdateImageView(BackBufferImage.Image, BackBufferImage.ImageView);
    }

    if (FVulkanShaderResourceViewRHI* View = BackBuffer->ShaderResourceView.Get())
    {
        View->UpdateImageView(BackBufferImage.Image, BackBufferImage.ImageView);
    }
}

void FVulkanSwapChainRHI::ReleaseResources()
{
    if (!BackBuffer)
    {
        return;
    }

    BackBuffer->UpdateImage(VK_NULL_HANDLE);
    BackBuffer->GetImageLayoutState().SetImageLayout(VK_IMAGE_LAYOUT_TO_BE_DETERMINED);

    if (FVulkanRenderTargetViewRHI* View = BackBuffer->RenderTargetView.Get())
    {
        View->UpdateImageView(VK_NULL_HANDLE, VK_NULL_HANDLE);
    }

    if (FVulkanUnorderedAccessViewRHI* View = BackBuffer->UnorderedAccessView.Get())
    {
        View->UpdateImageView(VK_NULL_HANDLE, VK_NULL_HANDLE);
    }

    if (FVulkanShaderResourceViewRHI* View = BackBuffer->ShaderResourceView.Get())
    {
        View->UpdateImageView(VK_NULL_HANDLE, VK_NULL_HANDLE);
    }
}

bool FVulkanSwapChainRHI::RecreateSurface()
{
	if (SwapChainResource && !RetiredSurface)
	{
		RetiredSurface = Surface;
	}

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
	SwapChainCreateInfo.PreviousSwapChain = RetiredSurface ? nullptr : SwapChainResource.Get();
	SwapChainCreateInfo.BufferCount       = CVarBackbufferCount.GetValue();
	SwapChainCreateInfo.Extent.width      = CreateWidth;
	SwapChainCreateInfo.Extent.height     = CreateHeight;
	SwapChainCreateInfo.Format            = Desc.ColorFormat;
	SwapChainCreateInfo.Usage             = Desc.Usage;
	SwapChainCreateInfo.bVerticalSync     = CVarEnableVSync.GetValue();
	SwapChainCreateInfo.bIsTransparent    = Desc.IsTransparent();

	// NOTE: Create a temporary SwapChain, keeping old alive until success
    FVulkanSwapChainRef NewSwapChainResource = new FVulkanSwapChain(GetDevice());
	if (!NewSwapChainResource->Initialize(SwapChainCreateInfo))
	{
        VULKAN_ERROR_CRITICAL("Failed to create SwapChain");
		return false;
	}

	ReleaseBackBufferImages();

	SwapChainResource = NewSwapChainResource;
	RetiredSurface.Reset();

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
    }

    if (Desc.IsTransparent() && !SwapChainResource->IsTransparent())
    {
        VULKAN_WARNING("The surface supports no non-opaque composite alpha, so the SwapChain will present opaque");
        Desc.Flags &= ~ESwapChainFlags::Transparent;
    }

    PendingAcquireSemaphore.Reset();
    PendingRenderSemaphore.Reset();

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

    ImageFences.Resize(BufferCount);

    for (uint32 i = 0; i < BufferCount; ++i)
    {
        ImageFences[i] = nullptr;
    }

    // Ensure we are recording
    CHECK(CommandContext->IsRecording());
    CHECK(!CommandContext->NeedsCommandBuffer());

    if (!CreateBackBufferImages())
    {
        return false;
    }

    CommandContext->SplitCommandBuffer(false, false);

	// Reset indices. AcquireNextImage callers will populate BackBufferIndex.
	SemaphoreIndex  = 0;
	BackBufferIndex = 0;

    // VK_EXT_hdr_metadata is per VkSwapchainKHR and does not carry over from the retired one.
    ApplyHDRMetadata();
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

    // Nothing can claim the pair once the swapchain is gone, and the semaphores it points at die with this object.
    PendingAcquireSemaphore.Reset();
    PendingRenderSemaphore.Reset();

    // The acquire path is the only other place these are dropped, and it will not run again
    ImageFences.Clear();

    // The images belong to the presentation engine, but the views over them are ours.
    ReleaseBackBufferImages();

    // Destroy the swapchain, then the surface it was created from if that one has been retired.
    SwapChainResource.Reset();
    RetiredSurface.Reset();

	BackBufferIndex = 0;
	SemaphoreIndex  = 0;
}

bool FVulkanSwapChainRHI::Resize(uint32 InWidth, uint32 InHeight, EFormat NewFormat, EColorSpace NewColorSpace)
{
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

    if (bFormatChanged || bColorSpaceChanged)
    {
        if (!IsFormatSupported(EffectiveFormat, EffectiveColorSpace))
        {
            VULKAN_ERROR("FVulkanSwapChainRHI::Resize: requested (%s, %s) not supported by this swap-chain.", ToString(EffectiveFormat), ToString(EffectiveColorSpace));
            return false;
        }
    }

    CHECK(!CommandContext->IsInsideRenderPass());
    CHECK(CommandContext->IsRecording());

    CommandContext->SplitCommandBuffer(false, true);

    PendingAcquireSemaphore.Reset();
    PendingRenderSemaphore.Reset();

    VULKAN_INFO("FVulkanSwapChainRHI::Resize Width=%u Height=%u Format=%s Colorspace=%s",
        ResolvedWidth, ResolvedHeight, ToString(ConvertFormat(EffectiveFormat)), ToString(ConvertColorSpace(EffectiveColorSpace)));

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

    return true;
}

bool FVulkanSwapChainRHI::Present(bool bVerticalSync)
{
    TRACE_SCOPE("Vulkan SwapChain Present");

	// If we don't have a drawable size, don't try to acquire/present
    if (!CanPresent())
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
			(Result == VK_SUBOPTIMAL_KHR ? "Suboptimal" : (Result == VK_ERROR_SURFACE_LOST_KHR ? "SurfaceLost" : "OutOfDate")));
        bNeedsRecreation = true;
    }
	else if (Result != VK_SUCCESS)
	{
		VULKAN_ERROR_CRITICAL("FVulkanSwapChainRHI::Present vkQueuePresentKHR failed with %s.", ToString(Result));
		return false;
	}

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

        PendingAcquireSemaphore.Reset();
        PendingRenderSemaphore.Reset();

        if (!CreateSwapChain(Desc.Width, Desc.Height))
        {
            VULKAN_WARNING("FVulkanSwapChainRHI::Present CreateSwapChain Failed");
            return false;
        }
    }

    AdvanceSemaphoreIndex();
    return true;
}

void FVulkanSwapChainRHI::SetDebugName(const String& InName)
{
    if (SwapChainResource)
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), *InName, SwapChainResource->GetVkSwapChain(), VK_OBJECT_TYPE_SWAPCHAIN_KHR);

        for (int32 Index = 0; Index < BackBufferImages.Size(); ++Index)
        {
            const String ImageName = InName + String::Printf(" BackBuffer Image[%d]", Index);
            VulkanSetObjectName(GetDevice()->GetVkDevice(), *ImageName, BackBufferImages[Index].Image, VK_OBJECT_TYPE_IMAGE);

            const String ImageViewName = InName + String::Printf(" BackBuffer ImageView[%d]", Index);
            VulkanSetObjectName(GetDevice()->GetVkDevice(), *ImageViewName, BackBufferImages[Index].ImageView, VK_OBJECT_TYPE_IMAGE_VIEW);
        }
    }
}

void* FVulkanSwapChainRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(SwapChainResource->GetVkSwapChain());
}

void* FVulkanSwapChainRHI::GetRHINativeResourceFromIndex(uint32 Index) const
{
    const int32 ImageIndex = static_cast<int32>(Index);
    return BackBufferImages.IsValidIndex(ImageIndex) ? reinterpret_cast<void*>(BackBufferImages[ImageIndex].Image) : nullptr;
}

void* FVulkanSwapChainRHI::GetRHINativeRenderTargetViewFromIndex(uint32 Index) const
{
    const int32 ImageIndex = static_cast<int32>(Index);
    return BackBufferImages.IsValidIndex(ImageIndex) ? reinterpret_cast<void*>(BackBufferImages[ImageIndex].ImageView) : nullptr;
}

void* FVulkanSwapChainRHI::GetRHINativeUnorderedAccessViewFromIndex(uint32 Index) const
{
    const int32 ImageIndex = static_cast<int32>(Index);
    return BackBufferImages.IsValidIndex(ImageIndex) ? reinterpret_cast<void*>(BackBufferImages[ImageIndex].ImageView) : nullptr;
}

void* FVulkanSwapChainRHI::GetRHINativeShaderResourceViewFromIndex(uint32 Index) const
{
    const int32 ImageIndex = static_cast<int32>(Index);
    return BackBufferImages.IsValidIndex(ImageIndex) ? reinterpret_cast<void*>(BackBufferImages[ImageIndex].ImageView) : nullptr;
}

FRHITexture* FVulkanSwapChainRHI::GetBackBuffer() const
{
    return BackBuffer.Get();
}

FRHIRenderTargetView* FVulkanSwapChainRHI::GetRenderTargetView() const
{
    return BackBuffer ? BackBuffer->GetRenderTargetView() : nullptr;
}

FRHIUnorderedAccessView* FVulkanSwapChainRHI::GetUnorderedAccessView() const
{
    return BackBuffer ? BackBuffer->GetUnorderedAccessView() : nullptr;
}

FRHIShaderResourceView* FVulkanSwapChainRHI::GetShaderResourceView() const
{
    return BackBuffer ? BackBuffer->GetShaderResourceView() : nullptr;
}

uint32 FVulkanSwapChainRHI::GetNumResources() const
{
    return GetNumBackBuffers();
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

bool FVulkanSwapChainRHI::SetHDRMetadata(const FRHIHDRMetadata& Metadata)
{
    Desc.HDRMetadata = Metadata;
    return ApplyHDRMetadata();
}

bool FVulkanSwapChainRHI::ApplyHDRMetadata()
{
#if VK_EXT_hdr_metadata
    // VK_EXT_hdr_metadata has no way to clear metadata once submitted, so an invalid metadata is simply not applied rather than reset.
    if (!Desc.HDRMetadata.bIsValid || CurrentColorSpace != EColorSpace::RGB_Full_G2084_None_P2020)
    {
        return false;
    }

    if (!SwapChainResource || !GetDevice()->IsExtensionEnabled(VK_EXT_HDR_METADATA_EXTENSION_NAME) || !vkSetHdrMetadataEXT)
    {
        return false;
    }

    const FRHIHDRMetadata& Source = Desc.HDRMetadata;

    VkHdrMetadataEXT HdrMetadata          = {};
    HdrMetadata.sType                     = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
    HdrMetadata.displayPrimaryRed         = { Source.RedPrimary.X,   Source.RedPrimary.Y   };
    HdrMetadata.displayPrimaryGreen       = { Source.GreenPrimary.X, Source.GreenPrimary.Y };
    HdrMetadata.displayPrimaryBlue        = { Source.BluePrimary.X,  Source.BluePrimary.Y  };
    HdrMetadata.whitePoint                = { Source.WhitePoint.X,   Source.WhitePoint.Y   };
    HdrMetadata.maxLuminance              = Source.MaxMasteringLuminance;
    HdrMetadata.minLuminance              = Source.MinMasteringLuminance;
    HdrMetadata.maxContentLightLevel      = Source.MaxContentLightLevel;
    HdrMetadata.maxFrameAverageLightLevel = Source.MaxFrameAverageLightLevel;

    const VkSwapchainKHR VkSwapChain = SwapChainResource->GetVkSwapChain();
    vkSetHdrMetadataEXT(GetDevice()->GetVkDevice(), 1, &VkSwapChain, &HdrMetadata);

    VULKAN_INFO("FVulkanSwapChainRHI: HDR10 metadata set (max=%.1f nits, min=%.4f nits, MaxCLL=%.1f, MaxFALL=%.1f)",
        Source.MaxMasteringLuminance, Source.MinMasteringLuminance, Source.MaxContentLightLevel, Source.MaxFrameAverageLightLevel);
    return true;
#else
    return false;
#endif
}

bool FVulkanSwapChainRHI::QueryDisplayHDRInfo(FRHIDisplayHDRInfo& /*OutInfo*/) const
{
    return false;
}

void FVulkanSwapChainRHI::ClaimPendingSemaphores(FVulkanCommands& InCommands)
{
    ClaimPendingAcquireSemaphore(InCommands);

    if (PendingRenderSemaphore && CanPresent())
    {
        InCommands.AddSignalSemaphore(PendingRenderSemaphore->GetVkSemaphore());
        PendingRenderSemaphore.Reset();
    }
}

void FVulkanSwapChainRHI::ClaimPendingAcquireSemaphore(FVulkanCommands& InCommands)
{
    if (PendingAcquireSemaphore)
    {
        InCommands.AddWaitSemaphore(PendingAcquireSemaphore->GetVkSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        PendingAcquireSemaphore.Reset();
    }
}

VkResult FVulkanSwapChainRHI::AcquireNextBackBuffer(FVulkanCommands* InCommands)
{
    const VkResult Result = AcquireNextImage();
    if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    {
        return Result;
    }

    SwapResources(BackBufferIndex);

    if (InCommands)
    {
        ClaimPendingAcquireSemaphore(*InCommands);
    }

    return Result;
}

VkResult FVulkanSwapChainRHI::AcquireNextImage()
{
    TRACE_SCOPE("Vulkan SwapChain Acquire Image");

	FVulkanSemaphoreRef ImageSemaphore  = ImageSemaphores[SemaphoreIndex];
    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];

#if VULKAN_LOG_SEMAPHORE_INDEX
    VULKAN_INFO("FVulkanSwapChainRHI::AcquireNextImage SemaphoreIndex=%d", SemaphoreIndex);
#endif

    if (HasPendingSemaphores())
    {
        const bool bKeepsRenderSemaphore = (PendingRenderSemaphore.Get() == RenderSemaphore.Get());

        CommandContext->GetCommandQueue().SubmitSemaphoresOnly(PendingAcquireSemaphore ? PendingAcquireSemaphore->GetVkSemaphore() :
            VK_NULL_HANDLE,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, (PendingRenderSemaphore && !bKeepsRenderSemaphore) ? PendingRenderSemaphore->GetVkSemaphore() : VK_NULL_HANDLE);

        PendingAcquireSemaphore.Reset();
        PendingRenderSemaphore.Reset();
    }

    if (ImageFences[SemaphoreIndex])
    {
        {
            TRACE_SCOPE("Vulkan SwapChain Image Fence Wait");
            ImageFences[SemaphoreIndex]->Wait();
        }

        ImageFences[SemaphoreIndex] = nullptr;
    }

    VkResult Result = VK_SUCCESS;
    {
        TRACE_SCOPE("Vulkan Acquire Next Image");
        Result = SwapChainResource->AcquireNextImage(ImageSemaphore.Get());
    }
    
    if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    {
        VULKAN_ERROR("Failed to aquire SwapChain image");
        return Result;
    }

    PendingAcquireSemaphore = ImageSemaphore;
    PendingRenderSemaphore  = RenderSemaphore;

    ImageFences[SemaphoreIndex] = CommandContext->GetSubmissionFence();

    // Update the BackBuffer index
    BackBufferIndex = SwapChainResource->GetBufferIndex();

    return Result;
}
