#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanSemaphore.h"
#include "VulkanRHI/VulkanSurface.h"
#include "VulkanRHI/VulkanSwapChainHandle.h"

#define VULKAN_INVALID_BACK_BUFFER_INDEX (-1)

typedef TSharedRef<class FVulkanSwapChain> FVulkanSwapChainRef;

class FVulkanCommandContext;

class FVulkanSwapChain final : public FRHISwapChain, public FVulkanDeviceChild
{
public:
    FVulkanSwapChain(FVulkanDevice* InDevice, const FRHISwapChainInfo& InSwapChainInfo);
    virtual ~FVulkanSwapChain();

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

    FVulkanSwapChainHandle* GetSwapChainHandle() const
    {
        return SwapChainHandle.Get();
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

    typedef TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> FVulkanSemaphoreArray;

    void*                       WindowHandle;
    FVulkanSurfaceRef           Surface;
    FVulkanSwapChainHandleRef   SwapChainHandle;
    FVulkanBackBufferTextureRef BackBuffer;
    TArray<FVulkanTextureRef>   BackBuffers;
    FVulkanSemaphoreArray       ImageSemaphores;
    FVulkanSemaphoreArray       RenderSemaphores;
    int32                       SemaphoreIndex;
    int32                       BackBufferIndex;
};

