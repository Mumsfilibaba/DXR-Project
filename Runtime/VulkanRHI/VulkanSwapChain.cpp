#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanSwapChain.h"
#include "VulkanRHI/VulkanCommandBuffer.h"

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

FVulkanSwapChain::FVulkanSwapChain(FVulkanDevice* InDevice, const FRHISwapChainInfo& InSwapChainInfo)
    : FRHISwapChain(InSwapChainInfo)
    , FVulkanDeviceChild(InDevice)
    , WindowHandle(InSwapChainInfo.WindowHandle)
    , Surface(nullptr)
    , SwapChainResource(nullptr)
    , BackBuffer(nullptr)
    , BackBuffers()
    , ImageSemaphores()
    , RenderSemaphores()
    , BackBufferIndex(VULKAN_INVALID_BACK_BUFFER_INDEX)
{
}

FVulkanSwapChain::~FVulkanSwapChain()
{
    FVulkanCommandContext* InCommandContext = FVulkanRHI::Get()->ObtainVulkanCommandContext();
    DestroySwapChain(InCommandContext);
}

bool FVulkanSwapChain::Initialize(FVulkanCommandContext* InCommandContext)
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

    FRHITextureInfo BackBufferInfo = FRHITextureInfo::CreateTexture2D(GetColorFormat(), GetWidth(), GetHeight(), 1, 1, ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable);
    BackBuffer = new FVulkanBackBufferTexture(GetDevice(), this, BackBufferInfo);
    if (!BackBuffer)
    {
        VULKAN_ERROR_CRITICAL("Failed to create BackBuffer");
        return false;
    }

    if (BackBufferInfo.bEnableResourceStateTracking)
    {
        BackBuffer->EnableStateTracking(EResourceAccess::Present);
    }

    if (FVulkanImageLayoutState* LayoutState = BackBuffer->GetImageLayoutState())
    {
        FVulkanImageLayoutState::FImageState PresentState;
        PresentState.Layout = ConvertResourceStateToImageLayout(EResourceAccess::Present);
        PresentState.Access = ConvertResourceStateToAccessFlags(EResourceAccess::Present);
        PresentState.Stage  = ConvertResourceStateToPipelineStageFlags(EResourceAccess::Present);
        LayoutState->SetState(PresentState);
    }

    return true;
}

bool FVulkanSwapChain::RecreateSurface(FVulkanCommandContext* InCommandContext)
{
	Surface = new FVulkanSurface(GetDevice(), InCommandContext->GetCommandQueue(), WindowHandle);
	if (!Surface->Initialize())
	{
		VULKAN_WARNING("RecreateSurface: failed to initialize surface");
		return false;
	}

	return true;
}

bool FVulkanSwapChain::ValidateSurfaceAndSize(FVulkanCommandContext* InCommandContext, uint32& OutWidth, uint32& OutHeight)
{
	VkSurfaceCapabilitiesKHR Capabilities = { };

	ESurfaceStatus SurfaceStatus = Surface->GetCapabilities(Capabilities);
	if (SurfaceStatus == ESurfaceStatus::SurfaceLost)
	{
		VULKAN_INFO("FVulkanSwapChain::ValidateSurfaceAndSize Surface lost. Attempting one recreation.");
        if (!RecreateSurface(InCommandContext))
        {
			return false;
        }

		SurfaceStatus = Surface->GetCapabilities(Capabilities);
		if (SurfaceStatus != ESurfaceStatus::Ok)
		{
			VULKAN_WARNING("FVulkanSwapChain::ValidateSurfaceAndSize Capabilities still not OK after surface recreation (status=%s)", ToString(SurfaceStatus));
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
		OutWidth  = OutWidth ? OutWidth : Info.Width;
		OutHeight = OutHeight ? OutHeight : Info.Height;

        if (OutWidth == 0 || OutHeight == 0)
        {
            // Minimized or no drawable size
			return false; 
        }
	}

	return true;
}

bool FVulkanSwapChain::CreateSwapChain(FVulkanCommandContext* InCommandContext, uint32 InWidth, uint32 InHeight)
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
	SwapChainCreateInfo.PreviousSwapChain = SwapChainResource.Get();
	SwapChainCreateInfo.BufferCount       = CVarBackbufferCount.GetValue();
	SwapChainCreateInfo.Extent.width      = CreateWidth;
	SwapChainCreateInfo.Extent.height     = CreateHeight;
	SwapChainCreateInfo.Format            = GetColorFormat();
	SwapChainCreateInfo.bVerticalSync     = CVarEnableVSync.GetValue();

	// NOTE: Create a temporary SwapChain, keeping old alive until success
    FVulkanSwapChainResourceRef NewSwapChainResource = new FVulkanSwapChainResource(GetDevice());
	if (!NewSwapChainResource->Initialize(SwapChainCreateInfo))
	{
        VULKAN_ERROR_CRITICAL("Failed to create SwapChain");
		return false;
	}
	else
	{
		SwapChainResource = NewSwapChainResource;
	}

    // Update the description if the requested image size was not supported
    VkExtent2D SwapChainExtent = SwapChainResource->GetExtent();
    if (InWidth != SwapChainExtent.width || InHeight != SwapChainExtent.height)
    {
        VULKAN_WARNING("Requested size [w=%d, h=%d] was not supported, the actual size is [w=%d, h=%d]", Info.Width, Info.Height, SwapChainExtent.width, SwapChainExtent.height);
        
        // Update the size of the viewport to the actual swapchain size
        Info.Width  = static_cast<uint16>(SwapChainExtent.width);
        Info.Height = static_cast<uint16>(SwapChainExtent.height);

        if (BackBuffer)
        {
            BackBuffer->ResizeBackBuffer(Info.Width, Info.Height);
        }
    }

    // Initialize semaphores and BackBuffers
    const uint32 BufferCount = SwapChainResource->GetBufferCount();
    if (BufferCount != static_cast<uint32>(BackBuffers.Size()))
	{
		ImageFences.Resize(BufferCount);
        ImageSemaphores.Resize(BufferCount);
        RenderSemaphores.Resize(BufferCount);
        BackBuffers.Resize(BufferCount);

        // Setup the info for each back-buffer, and ensure that we create the info with the actual image-size
        const ETextureUsageFlags UsageFlags = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable;
        FRHITextureInfo BackBufferInfo = FRHITextureInfo::CreateTexture2D(GetColorFormat(), SwapChainExtent.width, SwapChainExtent.height, 1, 1, UsageFlags);
        
        // Create a FVulkanTexture for each back-buffer
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
            if (FVulkanTextureRef NewTexture = new FVulkanTexture(GetDevice(), BackBufferInfo))
            {
                if (BackBufferInfo.bEnableResourceStateTracking)
                {
                    NewTexture->EnableStateTracking(EResourceAccess::Present);
                }

                BackBuffers[i] = NewTexture;
                
                if (FVulkanImageLayoutState* LayoutState = BackBuffers[i]->GetImageLayoutState())
                {
                    FVulkanImageLayoutState::FImageState PresentState;
                    PresentState.Layout = ConvertResourceStateToImageLayout(EResourceAccess::Present);
                    PresentState.Access = ConvertResourceStateToAccessFlags(EResourceAccess::Present);
                    PresentState.Stage  = ConvertResourceStateToPipelineStageFlags(EResourceAccess::Present);
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
    TArray<VkImage> SwapChainImages(SwapChainResource->GetBufferCount());
    SwapChainResource->GetSwapChainImages(SwapChainImages.Data());

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
        FVulkanTexture* BackBufferTexture = BackBuffers[Index++].Get();
        BackBufferTexture->SetVkImage(Image);

        if (FVulkanImageLayoutState* LayoutState = BackBufferTexture->GetImageLayoutState())
        {
            FVulkanImageLayoutState::FImageState PresentState;
            PresentState.Layout = ConvertResourceStateToImageLayout(EResourceAccess::Present);
            PresentState.Access = ConvertResourceStateToAccessFlags(EResourceAccess::Present);
            PresentState.Stage  = ConvertResourceStateToPipelineStageFlags(EResourceAccess::Present);
            LayoutState->SetState(PresentState);
        }
    }

    InCommandContext->SplitCommandBuffer(false, false);

	// Reset indices
	SemaphoreIndex  = 0;
	BackBufferIndex = VULKAN_INVALID_BACK_BUFFER_INDEX;
    return true;
}

void FVulkanSwapChain::DestroySwapChain(FVulkanCommandContext* InCommandContext)
{
    // Ensure that all work is completed
    InCommandContext->GetCommandQueue().WaitForCompletion();

    // Destroy the swapchain
    SwapChainResource.Reset();

	BackBufferIndex = VULKAN_INVALID_BACK_BUFFER_INDEX;
	SemaphoreIndex  = 0;
}

bool FVulkanSwapChain::Resize(FVulkanCommandContext* InCommandContext, uint32 InWidth, uint32 InHeight)
{
    if ((InWidth != Info.Width || InHeight != Info.Height) && InWidth > 0 && InHeight > 0)
    {
        CHECK(!InCommandContext->IsInsideRenderPass());
        CHECK(InCommandContext->IsRecording());

        // Ensure that all work is completed. If this function is called from a RHICommandList the we do 
        // this "manually" since the context is already started and we need to ensure that there is a valid CommandBuffer
        InCommandContext->SplitCommandBuffer(false, true);
        VULKAN_INFO("FVulkanSwapChain::Resize w=%d h=%d", InWidth, InHeight);

        if (!CreateSwapChain(InCommandContext, InWidth, InHeight))
        {
            VULKAN_WARNING("FVulkanSwapChain::Resize FAILED");
            return false;
        }

        Info.Width  = static_cast<uint16>(InWidth);
        Info.Height = static_cast<uint16>(InHeight);
        BackBuffer->ResizeBackBuffer(Info.Width, Info.Height);
    }

    return true;
}

bool FVulkanSwapChain::Present(FVulkanCommandContext* InCommandContext, bool bVerticalSync)
{
    // TODO: Recreate SwapChain based on V-Sync
    UNREFERENCED_VARIABLE(bVerticalSync);
   
	// If we don't have a drawable size, don't try to acquire/present
    if (Info.Width == 0 || Info.Height == 0 || !SwapChainResource)
    {
		return false;
    }

    VkResult Result = VK_SUCCESS;
    if (BackBufferIndex == VULKAN_INVALID_BACK_BUFFER_INDEX)
    {
        Result = AcquireNextImage(InCommandContext);
        if (Result != VK_SUCCESS)
        {
            VULKAN_INFO("FVulkanSwapChain::Present [AcquireNextImage] SwapChain is %s", Result == VK_SUBOPTIMAL_KHR ? "Suboptimal" : "OutOfDate");

            if (Result != VK_SUBOPTIMAL_KHR)
            {
			    // Try to recreate with current size (Recreate surface if needed)
			    InCommandContext->SplitCommandBuffer(false, true);

                if (!CreateSwapChain(InCommandContext, Info.Width, Info.Height))
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
		VULKAN_INFO("FVulkanSwapChain::Present SemaphoreIndex=%d", SemaphoreIndex);
	}

    Result = SwapChainResource->Present(InCommandContext->GetCommandQueue(), RenderSemaphore.Get());
    if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || Result == VK_ERROR_SURFACE_LOST_KHR)
    {
		VULKAN_INFO("FVulkanSwapChain::Present [Present] SwapChain is %s", (Result == VK_SUBOPTIMAL_KHR ? "Suboptimal" :
				(Result == VK_ERROR_SURFACE_LOST_KHR ? "SurfaceLost" : "OutOfDate")));

        InCommandContext->SplitCommandBuffer(false, true);

        if (!CreateSwapChain(InCommandContext, GetWidth(), GetHeight()))
        {
            VULKAN_WARNING("FVulkanSwapChain::Present CreateSwapChain Failed");
            return false;
        }
    }
	else if (Result != VK_SUCCESS)
	{
		VULKAN_ERROR_CRITICAL("FVulkanSwapChain::Present vkQueuePresentKHR failed with %s.", ToString(Result));
		return false;
	}

    AdvanceSemaphoreIndex();
    BackBufferIndex = VULKAN_INVALID_BACK_BUFFER_INDEX;
    return true;
}

void FVulkanSwapChain::SetDebugName(const FString& InName)
{
    // Name the swapchain object
    if (SwapChainResource)
    {
        VulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), *InName, SwapChainResource->GetVkSwapChain(), VK_OBJECT_TYPE_SWAPCHAIN_KHR);
        BackBuffer->SetDebugName("BackBuffer Proxy");

        // Name all the images
        for (int32 i = 0; i < BackBuffers.Size(); ++i)
        {
            const FString ImageName = InName + FString::CreateFormatted(" BackBuffer Image[%d]", i);
            BackBuffers[i]->SetDebugName(ImageName);
        }
    }
}

FVulkanTexture* FVulkanSwapChain::GetCurrentBackBuffer(FVulkanCommandContext* InCommandContext)
{
    if (!SwapChainResource || Info.Width == 0 || Info.Height == 0)
    {
        VULKAN_WARNING("FVulkanSwapChain::GetCurrentBackBuffer SwapChain is Invalid");
		return nullptr;
    }

    if (BackBufferIndex == VULKAN_INVALID_BACK_BUFFER_INDEX)
    {
        VkResult Result = AcquireNextImage(InCommandContext);
		if (Result == VK_SUBOPTIMAL_KHR)
		{
			VULKAN_WARNING("FVulkanSwapChain::GetCurrentBackBuffer SwapChain is Suboptimal");
		}
        
        if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
        {
            VULKAN_WARNING("FVulkanSwapChain::GetCurrentBackBuffer SwapChain is OutOfDate");
			InCommandContext->SplitCommandBuffer(false, true);

			if (!CreateSwapChain(InCommandContext, Info.Width, Info.Height))
			{
                VULKAN_WARNING("FVulkanSwapChain::GetCurrentBackBuffer SwapChain recreation failed");
				return nullptr;
			}

            return nullptr;
        }
    }
    
    return BackBuffers[BackBufferIndex].Get();
}

FRHITexture* FVulkanSwapChain::GetBackBuffer() const
{
    return BackBuffer.Get();
}

VkResult FVulkanSwapChain::AcquireNextImage(FVulkanCommandContext* InCommandContext)
{
    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];
    FVulkanSemaphoreRef ImageSemaphore  = ImageSemaphores[SemaphoreIndex];

    if constexpr (GVulkanLogSemaphoreIndex)
    {
        VULKAN_INFO("FVulkanSwapChain::AcquireNextImage SemaphoreIndex=%d", SemaphoreIndex);
    }

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

    InCommandContext->GetCommandQueue().AddWaitSemaphore(ImageSemaphore->GetVkSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    InCommandContext->GetCommandQueue().AddSignalSemaphore(RenderSemaphore->GetVkSemaphore());

    if (FVulkanFence* Fence = InCommandContext->GetSubmissionFence())
    {
        ImageFences[SemaphoreIndex] = Fence;
        Fence->AddRef();
    }

    // Update the BackBuffer index
    BackBufferIndex = SwapChainResource->GetBufferIndex();
    return Result;
}
