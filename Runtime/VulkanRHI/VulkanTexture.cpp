#include "VulkanTexture.h"
#include "VulkanViewport.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanBackBuffer

CVulkanBackBufferRef CVulkanBackBuffer::CreateBackBuffer(CVulkanDevice* InDevice, CVulkanViewport* InViewport, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumSamples)
{
    CVulkanBackBufferRef NewBackBuffer = dbg_new CVulkanBackBuffer(InDevice, InViewport, InFormat, InWidth, InHeight, InNumSamples);
    if (NewBackBuffer)
    {
        return NewBackBuffer;
    }

    return nullptr;
}

CVulkanBackBuffer::CVulkanBackBuffer(CVulkanDevice* InDevice, CVulkanViewport* InViewport, ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumSamples)
    : CVulkanTexture2D(InDevice, InFormat, InWidth, InHeight, 1, InNumSamples, TextureFlag_RTV, SClearValue())
    , Viewport(MakeSharedRef<CVulkanViewport>(InViewport))
{
}

void CVulkanBackBuffer::AquireNextImage()
{
    CVulkanSwapChain* SwapChain = Viewport->GetSwapChain();
    
    Image = Viewport->GetImage(SwapChain->GetBufferIndex());
    Assert(Image != VK_NULL_HANDLE);
    
    CVulkanImageView* ImageView = Viewport->GetImageView(SwapChain->GetBufferIndex());
    Assert(ImageView != nullptr);
    
    Assert(RenderTargetView != nullptr);
    RenderTargetView->SetImageView(MakeSharedRef<CVulkanImageView>(ImageView));
}
