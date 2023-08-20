#include "VulkanViewport.h"
#include "VulkanCommandBuffer.h"
#include "Core/Misc/ConsoleManager.h"

TAutoConsoleVariable<int32> CVarBackbufferCount(
    "Vulkan.BackbufferCount",
    "The preferred number of backbuffers for the SwapChain",
    NUM_BACK_BUFFERS);

FVulkanViewport::FVulkanViewport(FVulkanDevice* InDevice, FVulkanQueue* InQueue, const FRHIViewportDesc& InDesc)
    : FRHIViewport(InDesc)
    , FVulkanDeviceObject(InDevice)
    , WindowHandle(InDesc.WindowHandle)
    , Surface(nullptr)
    , SwapChain(nullptr)
    , BackBuffer(nullptr)
    , Queue(MakeSharedRef<FVulkanQueue>(InQueue))
    , Images()
    , ImageViews()
    , ImageSemaphores()
    , RenderSemaphores()
{
}

FVulkanViewport::~FVulkanViewport()
{
    DestroySwapChain();
}

bool FVulkanViewport::Initialize()
{
    Surface = new FVulkanSurface(GetDevice(), GetQueue(), WindowHandle);
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
    BackBuffer = new FVulkanBackBuffer(GetDevice(), this, BackBufferDesc);
    if (!BackBuffer)
    {
        VULKAN_ERROR("Failed to create BackBuffer");
        return false;
    }

    const uint32 BufferCount = SwapChain->GetBufferCount();
    ImageSemaphores.Reserve(BufferCount);
    RenderSemaphores.Reserve(BufferCount);
    ImageViews.Reserve(BufferCount);
    
    for (uint32 Index = 0; Index < BufferCount; ++Index)
    {
        FVulkanSemaphoreRef NewImageSemaphore = new FVulkanSemaphore(GetDevice());
        if (NewImageSemaphore->Initialize())
        {
            NewImageSemaphore->SetName("ImageSemaphore[" + TTypeToString<int32>::ToString(Index) + "]");
            ImageSemaphores.Add(NewImageSemaphore);
        }
        else
        {
            return false;
        }

        FVulkanSemaphoreRef NewRenderSemaphore = new FVulkanSemaphore(GetDevice());
        if (NewRenderSemaphore->Initialize())
        {
            NewRenderSemaphore->SetName("RenderSemaphore[" + TTypeToString<int32>::ToString(Index) + "]");
            RenderSemaphores.Add(NewRenderSemaphore);
        }
        else
        {
            return false;
        }
        
        FVulkanImageViewRef NewImageView = new FVulkanImageView(GetDevice());
        if (NewImageView)
        {
            ImageViews.Add(NewImageView);
        }
        else
        {
            return false;
        }
    }
    
    if (!CreateRenderTargets())
    {
        return false;
    }
    
    if (!AquireNextImage())
    {
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

    VULKAN_ERROR_COND(Desc.Width  != 0, "Viewport-width of zero is not supported");
    VULKAN_ERROR_COND(Desc.Height != 0, "Viewport-height of zero is not supported");

    SwapChain = new FVulkanSwapChain(GetDevice());
    if (!SwapChain->Initialize(SwapChainCreateInfo))
    {
        VULKAN_ERROR("Failed to create SwapChain");
        return false;
    }
    
    // Retrieve the images
    TArray<VkImage> SwapChainImages(SwapChain->GetBufferCount());
    SwapChain->GetSwapChainImages(SwapChainImages.Data());

    TUniquePtr<FVulkanCommandBuffer> CommandBuffer = MakeUnique<FVulkanCommandBuffer>(GetDevice(), EVulkanCommandQueueType::Graphics);
    if (!CommandBuffer->Initialize(VK_COMMAND_BUFFER_LEVEL_PRIMARY))
    {
        return false;
    }

    // Transition images to the correct layout that is expected by the rendering engine (To be compatible with the other RHI modules)
    CommandBuffer->Begin();

    Images.Reserve(SwapChainImages.Size());
    Images.Clear();
    
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

        CommandBuffer->ImageLayoutTransitionBarrier(TransitionBarrier);
        
        Images.Add(Image);
    }

    CommandBuffer->End();

    Queue->ExecuteCommandBuffer(CommandBuffer.GetAddressOf(), 1, CommandBuffer->GetFence());

    CommandBuffer->WaitForFence();
    return true;
}

bool FVulkanViewport::CreateRenderTargets()
{
    for (int32 Index = 0; Index < Images.Size(); ++Index)
    {
        FVulkanImageViewRef ImageView = ImageViews[Index];
        ImageView->DestroyView();
        
        VkSurfaceFormatKHR SurfaceFormat = SwapChain->GetVkSurfaceFormat();
        
        VkImageSubresourceRange SubresourceRange;
        SubresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        SubresourceRange.baseArrayLayer = 0;
        SubresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        SubresourceRange.baseMipLevel   = 0;
        SubresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
        
        if (!ImageView->CreateView(Images[Index], VK_IMAGE_VIEW_TYPE_2D, SurfaceFormat.format, 0, SubresourceRange))
        {
            return false;
        }
    }
    
    return true;
}

bool FVulkanViewport::RecreateSwapchain()
{
    if (!CreateSwapChain())
    {
        return false;
    }

    if (!CreateRenderTargets())
    {
        return false;
    }

    return true;
}

void FVulkanViewport::DestroySwapChain()
{
    Queue->WaitForCompletion();
    SwapChain.Reset();
}

bool FVulkanViewport::Resize(uint32 InWidth, uint32 InHeight)
{
    if (((InWidth != Desc.Width) || (InHeight != Desc.Height)) && (InWidth > 0) && (InHeight > 0))
    {
        VULKAN_INFO("Swapchain Resize");

        Desc.Width  = InWidth;
        Desc.Height = InHeight;

        Queue->FlushWaitSemaphoresAndWait();

        if (!RecreateSwapchain())
        {
            VULKAN_WARNING("Resize FAILED");
            return false;
        }

        if (!AquireNextImage())
        {
            VULKAN_WARNING("Resize FAILED");
            return false;
        }
    }

    return true;
}

bool FVulkanViewport::Present(bool bVerticalSync)
{
    UNREFERENCED_VARIABLE(bVerticalSync);

    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];

    VkResult Result = SwapChain->Present(Queue.Get(), RenderSemaphore.Get());
    if (Result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        VULKAN_INFO("Swapchain OutOfDate");

        Queue->WaitForCompletion();

        if (!RecreateSwapchain())
        {
            return false;
        }
    }
    
    AdvanceSemaphoreIndex();
    
    if (!AquireNextImage())
    {
        return false;
    }

    return true;
}

void FVulkanViewport::SetName(const FString& InName)
{
    FString ImageName;
    
    uint32 Index = 0;
    for (VkImage Image : Images)
    {
        ImageName = InName + "BackBuffer Image[" + TTypeToString<int32>::ToString(Index) + "]";
        FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), ImageName.GetCString(), Image, VK_OBJECT_TYPE_IMAGE);
    }
}

FRHITexture* FVulkanViewport::GetBackBuffer() const
{
    return BackBuffer.Get();
}

bool FVulkanViewport::AquireNextImage()
{
    FVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];
    FVulkanSemaphoreRef ImageSemaphore  = ImageSemaphores[SemaphoreIndex];
    
    VkResult Result = SwapChain->AquireNextImage(ImageSemaphore.Get());
    VULKAN_CHECK_RESULT(Result, "Failed to aquire image");
    
    // TOOD: Maybe change this, if maybe is not always desirable to always have a wait/signal
    Queue->AddWaitSemaphore(ImageSemaphore->GetVkSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    Queue->AddSignalSemaphore(RenderSemaphore->GetVkSemaphore());
    
    BackBuffer->AquireNextImage();
    return true;
}
