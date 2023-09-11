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

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final;
    
    virtual FRHITexture* GetBackBuffer() const override final;
    
    bool Present(bool bVerticalSync);

    void SetName(const FString& InName);
    
    FVulkanTexture* GetCurrentBackBuffer() const
    {
        return BackBuffers[SemaphoreIndex].Get();
    }
    
    FVulkanSwapChain* GetSwapChain() const
    { 
        return SwapChain.Get();
    }

    FVulkanQueue* GetQueue() const
    { 
        return Queue.Get();
    }

    FVulkanSurface* GetSurface() const
    { 
        return Surface.Get();
    }

private:
    bool CreateSwapChain();
    
    void DestroySwapChain();
    
    bool AquireNextImage();

    void AdvanceSemaphoreIndex()
    {
        SemaphoreIndex = (SemaphoreIndex + 1) % ImageSemaphores.Size();
    }
    
    void* WindowHandle;

    FVulkanSurfaceRef           Surface;
    FVulkanSwapChainRef         SwapChain;
    FVulkanQueueRef             Queue;
    FVulkanBackBufferTextureRef BackBuffer;
    TArray<FVulkanTextureRef>   BackBuffers;
    
    TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> ImageSemaphores;
    TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> RenderSemaphores;
    
    uint32 SemaphoreIndex;
    uint32 BackBufferIndex;
};

