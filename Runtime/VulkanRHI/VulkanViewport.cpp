#include "VulkanViewport.h"
#include "VulkanCommandBuffer.h"

#include "CoreApplication/Interface/PlatformWindow.h"
#include "Core/Debug/Console/ConsoleManager.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ConsoleVariables

TAutoConsoleVariable<int32> GBackbufferCount("vulkan.BackbufferCount", 3);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanViewport

TSharedRef<CVulkanViewport> CVulkanViewport::CreateViewport(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CPlatformWindow* InWindow, EFormat InFormat, uint32 InWidth, uint32 InHeight)
{
    TSharedRef<CVulkanViewport> NewViewport = dbg_new CVulkanViewport(InDevice, InQueue, InWindow, InFormat, InWidth, InHeight);
    if (NewViewport && NewViewport->Initialize())
    {
        return NewViewport;
    }
    
    return nullptr;
}

CVulkanViewport::CVulkanViewport(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CPlatformWindow* InWindow, EFormat InFormat, uint32 InWidth, uint32 InHeight)
    : CRHIViewport(InFormat, InWidth, InHeight)
    , CVulkanDeviceObject(InDevice)
    , Window(::AddRef(InWindow))
    , Surface(nullptr)
    , SwapChain(nullptr)
    , Queue(::AddRef(InQueue))
    , BackBuffer(nullptr)
    , BackBufferView(nullptr)
{
}

CVulkanViewport::~CVulkanViewport()
{
}

bool CVulkanViewport::Initialize()
{
    Surface = CVulkanSurface::CreateSurface(GetDevice(), GetQueue(), Window->GetPlatformHandle());
    if (!Surface)
    {
        VULKAN_ERROR_ALWAYS("Failed to create surface");
        return false;
    }

    SVulkanSwapChainCreateInfo SwapChainCreateInfo;
    SwapChainCreateInfo.BufferCount   = GBackbufferCount.GetInt();
    SwapChainCreateInfo.Extent.width  = Window->GetWidth();
    SwapChainCreateInfo.Extent.width  = Window->GetHeight();
    SwapChainCreateInfo.ColorSpace    = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    SwapChainCreateInfo.Format        = GetColorFormat();
    SwapChainCreateInfo.bVerticalSync = false;

    SwapChain = CVulkanSwapChain::CreateSwapChain(GetDevice(), GetQueue(), GetSurface(), SwapChainCreateInfo);
    if (!SwapChain)
    {
        VULKAN_ERROR_ALWAYS("Failed to create SwapChain");
        return false;
    }

    // Transition images into present since the BackBuffer is assumed to be in present state when created
    {
        TArray<VkImage> SwapChainImages;
        SwapChain->GetSwapChainImages(SwapChainImages);

        TUniquePtr<CVulkanCommandBuffer> CommandBuffer = MakeUnique<CVulkanCommandBuffer>(GetDevice(), EVulkanCommandQueueType::Graphics);
        if (!(CommandBuffer && CommandBuffer->Initialize(VK_COMMAND_BUFFER_LEVEL_PRIMARY)))
        {
            return false;
        }

        CommandBuffer->Begin();

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
        }

        CommandBuffer->End();

        Queue->ExecuteCommandBuffer(CommandBuffer.GetAddressOf(), 1, CommandBuffer->GetFence());

        CommandBuffer->WaitForFence();
    }

    BackBuffer     = dbg_new TVulkanTexture<CVulkanTexture2D>(Format, Width, Height, 1, 1, 0, SClearValue());
    BackBufferView = dbg_new CVulkanRenderTargetView();

    return true;
}

bool CVulkanViewport::Resize(uint32 InWidth, uint32 InHeight)
{
    Width  = InWidth;
    Height = InHeight;
    return true;
}

bool CVulkanViewport::Present(bool bVerticalSync)
{
    VkResult Result = SwapChain->Present();
    if (Result == VK_ERROR_OUT_OF_DATE_KHR)
    {


        SwapChain = CVulkanSwapChain::CreateSwapChain(GetDevice(), GetQueue(), GetSurface(), )
    }

    VULKAN_CHECK_RESULT(Result, "Presentation Failed. Result=" + String(ToString(Result)) + '.');
    return true;
}

void CVulkanViewport::SetName(const String& InName)
{
    CRHIObject::SetName(InName);
}

CRHIRenderTargetView* CVulkanViewport::GetRenderTargetView() const
{
    return BackBufferView.Get();
}

CRHITexture2D* CVulkanViewport::GetBackBuffer() const
{
    return BackBuffer.Get();
}

bool CVulkanViewport::IsValid() const
{
    return true;
}
