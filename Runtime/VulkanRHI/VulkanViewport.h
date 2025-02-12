#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanSemaphore.h"
#include "VulkanRHI/VulkanSurface.h"
#include "VulkanRHI/VulkanSwapChain.h"

#define VULKAN_INVALID_BACK_BUFFER_INDEX (-1)

typedef TSharedRef<class FVulkanViewport> FVulkanViewportRef;

class FVulkanCommandContext;

class FVulkanViewport final : public FRHIViewport, public FVulkanDeviceChild
{
public:
    FVulkanViewport(FVulkanDevice* InDevice, const FRHIViewportInfo& InViewportInfo);
    virtual ~FVulkanViewport();

    virtual FRHITexture* GetBackBuffer() const override final;

    bool Initialize(FVulkanCommandContext* InCommandContext);
    bool Resize(FVulkanCommandContext* InCommandContext, uint32 InWidth, uint32 InHeight);
    bool Present(FVulkanCommandContext* InCommandContext, bool bVerticalSync);
    FVulkanTexture* GetCurrentBackBuffer(FVulkanCommandContext* InCommandContext);
    
    void SetDebugName(const FString& InName);
    
    FVulkanTexture* GetBackBufferFromIndex(uint32 Index) const
    {
        CHECK(BackBuffers.IsValidIndex(Index));
        return BackBuffers[Index].Get();
    }
    
    uint32 GetNumBackBuffers() const
    {
        return BackBuffers.Size();
    }
    
    FVulkanSwapChain* GetSwapChain() const
    { 
        return SwapChain.Get();
    }

    FVulkanSurface* GetSurface() const
    { 
        return Surface.Get();
    }

private:
    bool CreateSwapChain(FVulkanCommandContext* InCommandContext, uint32 InWidth, uint32 InHeight);
    void DestroySwapChain(FVulkanCommandContext* InCommandContext);
    VkResult AquireNextImage(FVulkanCommandContext* InCommandContext);

    void AdvanceSemaphoreIndex()
    {
        SemaphoreIndex = (SemaphoreIndex + 1) % ImageSemaphores.Size();
    }

    void* WindowHandle;

    FVulkanSurfaceRef           Surface;
    FVulkanSwapChainRef         SwapChain;
    FVulkanBackBufferTextureRef BackBuffer;
    TArray<FVulkanTextureRef>   BackBuffers;

    TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> ImageSemaphores;
    TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> RenderSemaphores;

    int32 SemaphoreIndex;
    int32 BackBufferIndex;
};

