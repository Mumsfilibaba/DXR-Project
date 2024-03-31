#include "VulkanViewport.h"
#include "VulkanCommandBuffer.h"
#include "Core/Misc/ConsoleManager.h"

static TAutoConsoleVariable<int32> CVarBackbufferCount(
    "VulkanRHI.BackbufferCount",
    "The preferred number of backbuffers for the SwapChain",
    NUM_BACK_BUFFERS);

FVulkanViewport::FVulkanViewport(FVulkanDevice* InDevice, FVulkanCommandContext* InCmdContext, const FRHIViewportDesc& InDesc)
    : FRHIViewport(InDesc)
    , FVulkanDeviceChild(InDevice)
    , WindowHandle(InDesc.WindowHandle)
    , Surface(nullptr)
    , SwapChain(nullptr)
    , CommandContext(InCmdContext)
    , BackBuffer(nullptr)
    , BackBuffers()
    , ImageSemaphores()
    , RenderSemaphores()
    , SemaphoreIndex(0)
    , BackBufferIndex(0)
    , bAquireNextImage(false)
{
}

FVulkanViewport::~FVulkanViewport()
{
    DestroySwapChain();
}

bool FVulkanViewport::Initialize()
{
    Surface = new FVulkanSurface(GetDevice(), CommandContext->GetCommandQueue(), WindowHandle);
    if (!Surface->Initialize())
    {
        VULKAN_ERROR("Failed to create Surface");
        return false;
    }

    if (!CreateSwapChain())
    {
        return false;
    }

    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(GetColorFormat(), GetWidth(), GetHeight(), 1, 1, ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable);
    BackBuffer = new FVulkanBackBufferTexture(GetDevice(), this, BackBufferDesc);
    if (!BackBuffer)
    {
        VULKAN_ERROR("Failed to create BackBuffer");
        return false;
    }

    return true;
}

bool FVulkanViewport::CreateSwapChain()
{
    FVulkanSwapChainCreateInfo SwapChainCreateInfo;
    SwapChainCreateInfo.ColorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    SwapChainCreateInfo.PreviousSwapChain = GetSwapChain();
    SwapChainCreateInfo.Surface           = Surface.Get();
    SwapChainCreateInfo.BufferCount       = CVarBackbufferCount.GetValue();
    SwapChainCreateInfo.Extent.width      = Desc.Width;
    SwapChainCreateInfo.Extent.height     = Desc.Height;
    SwapChainCreateInfo.Format            = GetColorFormat();
    SwapChainCreateInfo.bVerticalSync     = false;

    if (Desc.Width == 0 || Desc.Height == 0)
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
    if (Desc.Width != SwapChainExtent.width || Desc.Height != SwapChainExtent.height)
    {
        VULKAN_WARNING("Requested size [w=%d, h=%d] was not supported, the actual size is [w=%d, h=%d]", Desc.Width, Desc.Height, SwapChainExtent.width, SwapChainExtent.height);
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
        ImageSemaphores.Resize(BufferCount);
        RenderSemaphores.Resize(BufferCount);
        BackBuffers.Resize(BufferCount);

        const uint32 BackBufferWidth  = GetWidth();
        const uint32 BackBufferheight = GetHeight();

        FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(GetColorFormat(), BackBufferWidth, BackBufferheight, 1, 1, ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable);
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

            if (FVulkanTextureRef NewTexture = new FVulkanTexture(GetDevice(), BackBufferDesc))
            {
                BackBuffers[Index] = NewTexture;
            }
            else
            {
                return false;
            }
        }
    }

    // Retrieve the images
    TArray<VkImage> SwapChainImages(SwapChain->GetBufferCount());
    SwapChain->GetSwapChainImages(SwapChainImages.Data());

    // If we are already recording, we just need to ensure that we have a CommandBuffer
    bool bWasRecording = CommandContext->IsRecording();
    if (bWasRecording)
    {
        // We might already have a CommandBuffer
        if (CommandContext->NeedsCommandBuffer())
        {
            CommandContext->ObtainCommandBuffer();
        }
    }
    else
    {
        CommandContext->RHIStartContext();
    }

    // We always needs to acquire the next image after we have created a swapchain
    bAquireNextImage = true;

    int32 Index = 0;
    for (VkImage Image : SwapChainImages)
    {
        FVulkanImageTransitionBarrier TransitionBarrier;
        TransitionBarrier.Image                           = Image;
        TransitionBarrier.PreviousLayout                  = VK_IMAGE_LAYOUT_UNDEFINED;
        TransitionBarrier.NewLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        TransitionBarrier.DependencyFlags                 = 0;
        TransitionBarrier.SrcAccessMask                   = VK_ACCESS_NONE;
        TransitionBarrier.DstAccessMask                   = VK_ACCESS_NONE;
        TransitionBarrier.SrcStageMask                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        TransitionBarrier.DstStageMask                    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        TransitionBarrier.SubresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        TransitionBarrier.SubresourceRange.baseArrayLayer = 0;
        TransitionBarrier.SubresourceRange.baseMipLevel   = 0;
        TransitionBarrier.SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        TransitionBarrier.SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

        CommandContext->GetCommandBuffer()->ImageLayoutTransitionBarrier(TransitionBarrier);
        BackBuffers[Index++]->SetVkImage(Image);
    }

    // If we are already recording then we just submit a commandbuffer and wait for it to complete
    if (bWasRecording)
    {
        CommandContext->FinishCommandBuffer(false);
        CommandContext->GetCommandQueue().WaitForCompletion();
        CommandContext->ObtainCommandBuffer();
    }
    else
    {
        CommandContext->RHIFinishContext();
        CommandContext->RHIFlush();
    }
    
    return true;
}

void FVulkanViewport::DestroySwapChain()
{
    // Ensure that all work is completed
    CommandContext->GetCommandQueue().WaitForCompletion();

    // Destroy the swapchain
    SwapChain.Reset();
}

bool FVulkanViewport::Resize(uint32 InWidth, uint32 InHeight)
{
    if ((InWidth != Desc.Width || InHeight != Desc.Height) && InWidth > 0 && InHeight > 0)
    {
        // Ensure that all work is completed, if this function is called from a RHICommandList
        // the we do this "manually" since the context is already started and we need to ensure that there
        // is a valid CommandBuffer
        if (CommandContext->IsRecording())
        {
            // TODO: What happens if we are in a renderpass?
            CommandContext->FinishCommandBuffer(false);
            CommandContext->GetCommandQueue().WaitForCompletion();
            CommandContext->ObtainCommandBuffer();
        }
        else
        {
            CommandContext->RHIClearState();
        }
        
        VULKAN_INFO("Swapchain Resize w=%d h=%d", InWidth, InHeight);

        if (!CreateSwapChain())
        {
            VULKAN_WARNING("Resize FAILED");
            return false;
        }

        Desc.Width  = static_cast<uint16>(InWidth);
        Desc.Height = static_cast<uint16>(InHeight);
        BackBuffer->ResizeBackBuffer(Desc.Width, Desc.Height);
    }

    return true;
}

bool FVulkanViewport::Present(bool bVerticalSync)
{
    UNREFERENCED_VARIABLE(bVerticalSync);

    if (bAquireNextImage)
    {
        if (!AquireNextImage())
        {
            return false;
        }
    }
    
    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];
    VkResult Result = SwapChain->Present(CommandContext->GetCommandQueue(), RenderSemaphore.Get());
    if (Result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        VULKAN_INFO("Swapchain OutOfDate");
        CommandContext->GetCommandQueue().FlushWaitSemaphoresAndWait();

        if (!CreateSwapChain())
        {
            return false;
        }
    }

    AdvanceSemaphoreIndex();
    bAquireNextImage = true;
    return true;
}

void FVulkanViewport::SetDebugName(const FString& InName)
{
    // Name the swapchain object
    if (SwapChain)
    {
        FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.GetCString(), SwapChain->GetVkSwapChain(), VK_OBJECT_TYPE_SWAPCHAIN_KHR);

        // Name the proxy
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

FVulkanTexture* FVulkanViewport::GetCurrentBackBuffer()
{
    if (bAquireNextImage)
    {
        if (!AquireNextImage())
        {
            return nullptr;
        }
        
        bAquireNextImage = false;
    }
    
    return BackBuffers[SemaphoreIndex].Get();
}

FRHITexture* FVulkanViewport::GetBackBuffer() const
{
    return BackBuffer.Get();
}

bool FVulkanViewport::AquireNextImage()
{
    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];
    FVulkanSemaphoreRef ImageSemaphore  = ImageSemaphores[SemaphoreIndex];

    // NOTE: For now we let suboptimal SwapChains pass
    VkResult Result = SwapChain->AquireNextImage(ImageSemaphore.Get());
    if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    {
        VULKAN_ERROR("Failed to aquire SwapChain image");
        return false;
    }

    // TOOD: Maybe change this, if maybe is not always desirable to always have a wait/signal
    CommandContext->GetCommandQueue().AddWaitSemaphore(ImageSemaphore->GetVkSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    CommandContext->GetCommandQueue().AddSignalSemaphore(RenderSemaphore->GetVkSemaphore());

    // Update the BackBuffer index
    BackBufferIndex = SwapChain->GetBufferIndex();
    return true;
}
