#pragma once
#include "VulkanTexture.h"
#include "VulkanSemaphore.h"
#include "VulkanSurface.h"
#include "VulkanSwapChain.h"
#include "RHI/RHIResources.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class FVulkanViewport> FVulkanViewportRef;

class FVulkanCommandContext;

class FVulkanViewport final : public FRHIViewport, public FVulkanDeviceChild
{
public:
    FVulkanViewport(FVulkanDevice* InDevice, FVulkanCommandContext* InCmdContext, const FRHIViewportDesc& InDesc);
    virtual ~FVulkanViewport();

    virtual FRHITexture* GetBackBuffer() const override final;

    bool Initialize();

    bool Resize(uint32 InWidth, uint32 InHeight);
    bool Present(bool bVerticalSync);

    void SetDebugName(const FString& InName);
    
    FVulkanTexture* GetCurrentBackBuffer();
    
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
    FVulkanCommandContext*      CommandContext;
    FVulkanBackBufferTextureRef BackBuffer;
    TArray<FVulkanTextureRef>   BackBuffers;

    TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> ImageSemaphores;
    TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> RenderSemaphores;

    uint32 SemaphoreIndex;
    uint32 BackBufferIndex;
    bool   bAquireNextImage;
};

