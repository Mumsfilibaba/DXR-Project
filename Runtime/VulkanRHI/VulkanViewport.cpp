#include "VulkanViewport.h"
#include "VulkanCommandBuffer.h"

#include "CoreApplication/Interface/PlatformWindow.h"

#include "Core/Debug/Console/ConsoleManager.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ConsoleVariables

TAutoConsoleVariable<int32> GBackbufferCount("vulkan.BackbufferCount", NUM_BACK_BUFFERS);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanViewport

TSharedRef<CVulkanViewport> CVulkanViewport::CreateViewport(CVulkanDevice* InDevice, CVulkanQueue* InQueue, PlatformWindowHandle InWindowHandle, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight)
{
    TSharedRef<CVulkanViewport> NewViewport = dbg_new CVulkanViewport(InDevice, InQueue, InWindowHandle, InFormat, InWidth, InHeight);
    if (NewViewport && NewViewport->Initialize())
    {
        return NewViewport;
    }
    
    return nullptr;
}

CVulkanViewport::CVulkanViewport(CVulkanDevice* InDevice, CVulkanQueue* InQueue, PlatformWindowHandle InWindowHandle, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight)
    : CRHIViewport(InFormat, InWidth, InHeight)
    , CVulkanDeviceObject(InDevice)
    , WindowHandle(InWindowHandle)
    , Surface(nullptr)
    , SwapChain(nullptr)
    , Queue(::AddRef(InQueue))
    , BackBuffer(nullptr)
    , Images()
    , ImageViews()
    , ImageSemaphores()
    , RenderSemaphores()
{
}

CVulkanViewport::~CVulkanViewport()
{
    DestroySwapChain();
}

bool CVulkanViewport::Initialize()
{
    Surface = CVulkanSurface::CreateSurface(GetDevice(), GetQueue(), WindowHandle);
    if (!Surface)
    {
        VULKAN_ERROR_ALWAYS("Failed to create Surface");
        return false;
    }

    if (!CreateSwapChain())
    {
        return false;
    }

    BackBuffer = CVulkanBackBuffer::CreateBackBuffer(GetDevice(), this, GetColorFormat(), GetWidth(), GetHeight(), 1);
    if (!BackBuffer)
    {
        VULKAN_ERROR_ALWAYS("Failed to create BackBuffer");
        return false;
    }

    const uint32 BufferCount = SwapChain->GetBufferCount();
    ImageSemaphores.Reserve(BufferCount);
    RenderSemaphores.Reserve(BufferCount);
    ImageViews.Reserve(BufferCount);
    
    for (uint32 Index = 0; Index < BufferCount; ++Index)
    {
        CVulkanSemaphoreRef NewImageSemaphore = CVulkanSemaphore::CreateSemaphore(GetDevice());
        if (NewImageSemaphore)
        {
            NewImageSemaphore->SetName("ImageSemaphore[" + ToString(Index) + "]");
            ImageSemaphores.Push(NewImageSemaphore);
        }
        else
        {
            return false;
        }

        CVulkanSemaphoreRef NewRenderSemaphore = CVulkanSemaphore::CreateSemaphore(GetDevice());
        if (NewRenderSemaphore)
        {
            NewRenderSemaphore->SetName("RenderSemaphore[" + ToString(Index) + "]");
            RenderSemaphores.Push(NewRenderSemaphore);
        }
        else
        {
            return false;
        }
        
        CVulkanImageViewRef NewImageView = dbg_new CVulkanImageView(GetDevice());
        if (NewImageView)
        {
            ImageViews.Push(NewImageView);
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

bool CVulkanViewport::CreateSwapChain()
{
    SVulkanSwapChainCreateInfo SwapChainCreateInfo;
    SwapChainCreateInfo.ColorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    SwapChainCreateInfo.PreviousSwapChain = GetSwapChain();
    SwapChainCreateInfo.Surface           = GetSurface();
    SwapChainCreateInfo.BufferCount       = GBackbufferCount.GetInt();
    SwapChainCreateInfo.Extent.width      = GetWidth();
    SwapChainCreateInfo.Extent.height     = GetHeight();
    SwapChainCreateInfo.Format            = GetColorFormat();
    SwapChainCreateInfo.bVerticalSync     = false;

    VULKAN_ERROR(Width  != 0, "Viewport-width of zero is not supported");
    VULKAN_ERROR(Height != 0, "Viewport-height of zero is not supported");

    SwapChain = CVulkanSwapChain::CreateSwapChain(GetDevice(), SwapChainCreateInfo);
    if (!SwapChain)
    {
        VULKAN_ERROR_ALWAYS("Failed to create SwapChain");
        return false;
    }
    
    // Retrieve the images
    TArray<VkImage> SwapChainImages(SwapChain->GetBufferCount());
    SwapChain->GetSwapChainImages(SwapChainImages.Data());

    TUniquePtr<CVulkanCommandBuffer> CommandBuffer = MakeUnique<CVulkanCommandBuffer>(GetDevice(), EVulkanCommandQueueType::Graphics);
    if (!(CommandBuffer && CommandBuffer->Initialize(VK_COMMAND_BUFFER_LEVEL_PRIMARY)))
    {
        return false;
    }

    CommandBuffer->Begin();

    Images.Reserve(SwapChainImages.Size());
    Images.Clear();
    
    for (VkImage Image : SwapChainImages)
    {
        SVulkanImageTransitionBarrier TransitionBarrier;
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
        
        Images.Push(Image);
    }

    CommandBuffer->End();

    Queue->ExecuteCommandBuffer(CommandBuffer.GetAddressOf(), 1, CommandBuffer->GetFence());

    CommandBuffer->WaitForFence();
    
    return true;
}

bool CVulkanViewport::CreateRenderTargets()
{
    for (int32 Index = 0; Index < Images.Size(); ++Index)
    {
        CVulkanImageViewRef ImageView = ImageViews[Index];
        if (ImageView->IsValid())
        {
            ImageView->DestroyView();
        }
        
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

bool CVulkanViewport::RecreateSwapchain()
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

void CVulkanViewport::DestroySwapChain()
{
    Queue->WaitForCompletion();
    SwapChain.Reset();
}

bool CVulkanViewport::Resize(uint32 InWidth, uint32 InHeight)
{
    if (((InWidth != Width) || (InHeight != Height)) && (InWidth > 0) && (InHeight > 0))
    {
        VULKAN_INFO("Swapchain Resize");

        Width  = InWidth;
        Height = InHeight;

        // Makes sure that the semaphores are waited/signaled
        Queue->AddWaitSemaphore(RenderSemaphores[SemaphoreIndex]->GetVkSemaphore(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        Queue->Flush();

        if (!RecreateSwapchain())
        {
            VULKAN_WARNING("Resize FAILED");
            return false;
        }

        AdvanceSemaphoreIndex();

        if (!AquireNextImage())
        {
            VULKAN_WARNING("Resize FAILED");
            return false;
        }
    }

    return true;
}

bool CVulkanViewport::Present(bool bVerticalSync)
{
    UNREFERENCED_VARIABLE(bVerticalSync);

    CVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];

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

void CVulkanViewport::SetName(const String& InName)
{
    CRHIObject::SetName(InName);
    
    String ImageName;
    
    uint32 Index = 0;
    for (VkImage Image : Images)
    {
        ImageName = InName + "BackBuffer Image[" + ToString(Index) + "]";
        CVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), ImageName.CStr(), Image, VK_OBJECT_TYPE_IMAGE);
    }
}

CRHIRenderTargetView* CVulkanViewport::GetRenderTargetView() const
{
    return BackBuffer ? BackBuffer->GetRenderTargetView() : nullptr;
}

CRHITexture2D* CVulkanViewport::GetBackBuffer() const
{
    return BackBuffer.Get();
}

bool CVulkanViewport::IsValid() const
{
    return (SwapChain != nullptr);
}

bool CVulkanViewport::AquireNextImage()
{
    CVulkanSemaphoreRef RenderSemaphore = RenderSemaphores[SemaphoreIndex];
    CVulkanSemaphoreRef ImageSemaphore  = ImageSemaphores[SemaphoreIndex];
    
    VkResult Result = SwapChain->AquireNextImage(ImageSemaphore.Get());
    VULKAN_CHECK_RESULT(Result, "Failed to aquire image");
    
    // TOOD: Maybe change this, if maybe is not always desirable to have a wait/signal
    Queue->AddWaitSemaphore(ImageSemaphore->GetVkSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    Queue->AddSignalSemaphore(RenderSemaphore->GetVkSemaphore());
    
    BackBuffer->AquireNextImage();
    return true;
}
