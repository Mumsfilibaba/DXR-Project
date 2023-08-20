#pragma once
#include "VulkanTexture.h"
#include "VulkanQueue.h"
#include "VulkanSemaphore.h"
#include "VulkanSurface.h"
#include "VulkanSwapChain.h"
#include "RHI/RHIResources.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class FVulkanViewport> FVulkanViewportRef;

class FVulkanViewport final : public FRHIViewport, public FVulkanDeviceObject
{
public:
    FVulkanViewport(FVulkanDevice* InDevice, FVulkanQueue* InQueue, const FRHIViewportDesc& InDesc);
    ~FVulkanViewport();

    bool Initialize();

	bool Present(bool bVerticalSync);

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final;
    
    virtual FRHITexture* GetBackBuffer() const override final;

    void SetName(const FString& InName);
    
    FORCEINLINE FVulkanSwapChain* GetSwapChain() const 
    { 
        return SwapChain.Get();
    }

    FORCEINLINE FVulkanQueue* GetQueue() const 
    { 
        return Queue.Get();
    }

    FORCEINLINE FVulkanSurface* GetSurface() const 
    { 
        return Surface.Get();
    }

    FORCEINLINE VkImage GetImage(uint32 Index) const
    {
        return Images[Index];
    }

    FORCEINLINE FVulkanImageView* GetImageView(uint32 Index)
    {
        return ImageViews[Index].Get();
    }

private:
    bool CreateSwapChain();

    bool CreateRenderTargets();

    bool RecreateSwapchain();
    
    void DestroySwapChain();
    
    bool AquireNextImage();

    void AdvanceSemaphoreIndex()
    {
        SemaphoreIndex = (SemaphoreIndex + 1) % ImageSemaphores.Size();
    }
    
    void* WindowHandle;

    FVulkanSurfaceRef    Surface;
    FVulkanSwapChainRef  SwapChain;
    FVulkanBackBufferRef BackBuffer;
    FVulkanQueueRef      Queue;

    TArray<VkImage,             TInlineArrayAllocator<VkImage, NUM_BACK_BUFFERS>>             Images;
    TArray<FVulkanImageViewRef, TInlineArrayAllocator<FVulkanImageViewRef, NUM_BACK_BUFFERS>> ImageViews;
    
    TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> ImageSemaphores;
    TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> RenderSemaphores;

    uint32 SemaphoreIndex = 0;
};

