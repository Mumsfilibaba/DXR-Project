#include "Core/Misc/ConsoleManager.h"
#include "VulkanRHI/VulkanViewport.h"
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

// Recreate the SwapChain when it's reported to be suboptimal when acquiring next image
static constexpr bool GVulkanRecreateSuboptimalSwapChainOnAquire = true;

FVulkanViewport::FVulkanViewport(FVulkanDevice* InDevice, const FRHIViewportInfo& InViewportInfo)
    : FRHIViewport(InViewportInfo)
    , FVulkanDeviceChild(InDevice)
    , WindowHandle(InViewportInfo.WindowHandle)
    , Surface(nullptr)
    , SwapChain(nullptr)
    , BackBuffer(nullptr)
    , BackBuffers()
    , ImageSemaphores()
    , RenderSemaphores()
    , BackBufferIndex(VULKAN_INVALID_BACK_BUFFER_INDEX)
{
}

FVulkanViewport::~FVulkanViewport()
{
    FVulkanCommandContext* InCommandContext = FVulkanRHI::GetRHI()->ObtainCommandContext();
    DestroySwapChain(InCommandContext);
}

bool FVulkanViewport::Initialize(FVulkanCommandContext* InCommandContext)
{
    if (!InCommandContext)
    {
        VULKAN_ERROR("CommandContext cannot be nullptr");
        return false;
    }
    
    Surface = new FVulkanSurface(GetDevice(), InCommandContext->GetCommandQueue(), WindowHandle);
    if (!Surface->Initialize())
    {
        VULKAN_ERROR("Failed to create Surface");
        return false;
    }
    
    // We need to start the context since that locks it to this thread
    InCommandContext->RHIStartContext();

    if (!CreateSwapChain(InCommandContext, GetWidth(), GetHeight()))
    {
        return false;
    }
    
    // Unlock the context from this thread
    InCommandContext->RHIFinishContext();
    InCommandContext->RHIFlush();

    FRHITextureInfo BackBufferInfo = FRHITextureInfo::CreateTexture2D(GetColorFormat(), GetWidth(), GetHeight(), 1, 1, ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable);
    BackBuffer = new FVulkanBackBufferTexture(GetDevice(), this, BackBufferInfo);
    if (!BackBuffer)
    {
        VULKAN_ERROR("Failed to create BackBuffer");
        return false;
    }

    return true;
}

bool FVulkanViewport::CreateSwapChain(FVulkanCommandContext* InCommandContext, uint32 InWidth, uint32 InHeight)
{
    FVulkanSwapChainCreateInfo SwapChainCreateInfo;
    SwapChainCreateInfo.ColorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    SwapChainCreateInfo.PreviousSwapChain = SwapChain.Get();
    SwapChainCreateInfo.Surface           = Surface.Get();
    SwapChainCreateInfo.BufferCount       = CVarBackbufferCount.GetValue();
    SwapChainCreateInfo.Extent.width      = InWidth;
    SwapChainCreateInfo.Extent.height     = InHeight;
    SwapChainCreateInfo.Format            = GetColorFormat();
    SwapChainCreateInfo.bVerticalSync     = CVarEnableVSync.GetValue();

    if (InWidth == 0 || InHeight == 0)
    {
        VULKAN_ERROR("Viewport Width or Height of zero is not supported");
        return false;
    }

    // NOTE: Create a temporary SwapChain, this is done in order to keep the old swapchain alive in case we recreate the swapchain
    FVulkanSwapChainRef NewSwapChain = new FVulkanSwapChain(GetDevice());
    if (!NewSwapChain->Initialize(SwapChainCreateInfo))
    {
        VULKAN_ERROR("Failed to create SwapChain");
        return false;
    }
    else
    {
        SwapChain = NewSwapChain;
    }

    // Update the description if the requested image size was not supported
    VkExtent2D SwapChainExtent = SwapChain->GetExtent();
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
    const uint32 BufferCount = SwapChain->GetBufferCount();
    if (BufferCount != static_cast<uint32>(BackBuffers.Size()))
    {
        ImageSemaphores.Resize(BufferCount);
        RenderSemaphores.Resize(BufferCount);
        BackBuffers.Resize(BufferCount);

        // Setup the info for each backbuffer, and ensure that we create the info with the actual image-size
        const ETextureUsageFlags UsageFlags = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable;
        FRHITextureInfo BackBufferInfo = FRHITextureInfo::CreateTexture2D(GetColorFormat(), SwapChainExtent.width, SwapChainExtent.height, 1, 1, UsageFlags);
        
        // Create a FVulkanTexture for each backbuffer
        for (uint32 Index = 0; Index < BufferCount; ++Index)
        {
            FVulkanSemaphoreRef NewImageSemaphore = new FVulkanSemaphore(GetDevice());
            if (NewImageSemaphore->Initialize())
            {
                NewImageSemaphore->SetDebugName("ImageSemaphore[" + TTypeToString<int32>::ToString(Index) + "]");
                ImageSemaphores[Index] = NewImageSemaphore;
            }
            else
            {
                return false;
            }

            FVulkanSemaphoreRef NewRenderSemaphore = new FVulkanSemaphore(GetDevice());
            if (NewRenderSemaphore->Initialize())
            {
                NewRenderSemaphore->SetDebugName("RenderSemaphore[" + TTypeToString<int32>::ToString(Index) + "]");
                RenderSemaphores[Index] = NewRenderSemaphore;
            }
            else
            {
                return false;
            }

            if (FVulkanTextureRef NewTexture = new FVulkanTexture(GetDevice(), BackBufferInfo))
            {
                BackBuffers[Index] = NewTexture;
            }
            else
            {
                return false;
            }
        }
        
        // Set a valid semaphore index
        SemaphoreIndex = 0;
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
        VkImageMemoryBarrier2 ImageBarrier;
        FMemory::Memzero(&ImageBarrier);

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
        BackBuffers[Index++]->SetVkImage(Image);
    }

    InCommandContext->SplitCommandBuffer(false, false);
    return true;
}

void FVulkanViewport::DestroySwapChain(FVulkanCommandContext* InCommandContext)
{
    // Ensure that all work is completed
    InCommandContext->GetCommandQueue().WaitForCompletion();

    // Destroy the swapchain
    SwapChain.Reset();
}

bool FVulkanViewport::Resize(FVulkanCommandContext* InCommandContext, uint32 InWidth, uint32 InHeight)
{
    if ((InWidth != Info.Width || InHeight != Info.Height) && InWidth > 0 && InHeight > 0)
    {
        CHECK(!InCommandContext->IsInsideRenderPass());
        CHECK(InCommandContext->IsRecording());
        
        // Ensure that all work is completed, if this function is called from
        // a RHICommandList the we do this "manually" since the context is already
        // started and we need to ensure that there is a valid CommandBuffer
        InCommandContext->SplitCommandBuffer(false, true);
        VULKAN_INFO("FVulkanViewport::Resize w=%d h=%d", InWidth, InHeight);
               
        if (!CreateSwapChain(InCommandContext, InWidth, InHeight))
        {
            VULKAN_WARNING("FVulkanViewport::Resize FAILED");
            return false;
        }

        Info.Width  = static_cast<uint16>(InWidth);
        Info.Height = static_cast<uint16>(InHeight);
        BackBuffer->ResizeBackBuffer(Info.Width, Info.Height);
    }

    return true;
}

bool FVulkanViewport::Present(FVulkanCommandContext* InCommandContext, bool bVerticalSync)
{
    // TODO: Recreate SwapChain based on V-Sync
    UNREFERENCED_VARIABLE(bVerticalSync);
   
    VkResult Result = VK_SUCCESS;
    if (BackBufferIndex == VULKAN_INVALID_BACK_BUFFER_INDEX)
    {
        Result = AquireNextImage(InCommandContext);
        if (Result == VK_SUBOPTIMAL_KHR)
        {
            LOG_WARNING("FVulkanViewport::Present [AquireNextImage] SwapChain is Suboptimal");
        }
        
        if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
        {
            LOG_WARNING("FVulkanViewport::Present [AquireNextImage] SwapChain is OutOfDate");
            return false;
        }
    }

    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];
    Result = SwapChain->Present(InCommandContext->GetCommandQueue(), RenderSemaphore.Get());
    if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR)
    {
        if (Result == VK_SUBOPTIMAL_KHR)
        {
            VULKAN_INFO("FVulkanViewport::Present [Present] SwapChain is Suboptimal");
        }
        else
        {
            VULKAN_INFO("FVulkanViewport::Present [Present] SwapChain is OutOfDate");
        }
        
        InCommandContext->SplitCommandBuffer(false, true);

        if (!CreateSwapChain(InCommandContext, GetWidth(), GetHeight()))
        {
            LOG_WARNING("FVulkanViewport::Present CreateSwapChain Failed");
            return false;
        }
    }

    AdvanceSemaphoreIndex();
    BackBufferIndex = VULKAN_INVALID_BACK_BUFFER_INDEX;
    return true;
}

void FVulkanViewport::SetDebugName(const FString& InName)
{
    // Name the swapchain object
    if (SwapChain)
    {
        FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), *InName, SwapChain->GetVkSwapChain(), VK_OBJECT_TYPE_SWAPCHAIN_KHR);
        BackBuffer->SetDebugName("BackBuffer Proxy");

        // Name all the images
        FString ImageName;
        for (uint32 Index = 0; FVulkanTextureRef Texture : BackBuffers)
        {
            ImageName = InName + FString::CreateFormatted(" BackBuffer Image[%d]", Index);
            Texture->SetDebugName(ImageName);
        }
    }
}

FVulkanTexture* FVulkanViewport::GetCurrentBackBuffer(FVulkanCommandContext* InCommandContext)
{
    if (BackBufferIndex == VULKAN_INVALID_BACK_BUFFER_INDEX)
    {
        VkResult Result = AquireNextImage(InCommandContext);
        if (GVulkanRecreateSuboptimalSwapChainOnAquire)
        {
            if (Result == VK_SUBOPTIMAL_KHR)
            {
                VULKAN_WARNING("FVulkanViewport::GetCurrentBackBuffer SwapChain is Suboptimal");
            }
        }
        
        if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
        {
            VULKAN_WARNING("FVulkanViewport::GetCurrentBackBuffer SwapChain is OutOfDate");
            return nullptr;
        }
    }
    
    return BackBuffers[BackBufferIndex].Get();
}

FRHITexture* FVulkanViewport::GetBackBuffer() const
{
    return BackBuffer.Get();
}

VkResult FVulkanViewport::AquireNextImage(FVulkanCommandContext* InCommandContext)
{
    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];
    FVulkanSemaphoreRef ImageSemaphore  = ImageSemaphores[SemaphoreIndex];

    VkResult Result = SwapChain->AquireNextImage(ImageSemaphore.Get());
    if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    {
        VULKAN_ERROR("Failed to aquire SwapChain image");
        return Result;
    }

    InCommandContext->GetCommandQueue().AddWaitSemaphore(ImageSemaphore->GetVkSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    InCommandContext->GetCommandQueue().AddSignalSemaphore(RenderSemaphore->GetVkSemaphore());

    // Update the BackBuffer index
    BackBufferIndex = SwapChain->GetBufferIndex();
    return Result;
}
