#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanSurface.h"
#include "VulkanQueue.h"
#include "VulkanSemaphore.h"

#include "Core/RefCounted.h"
#include "Core/Debug/Debug.h"

#define NUM_BACK_BUFFERS (3)

typedef TSharedRef<class CVulkanSwapChain> CVulkanSwapChainRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVulkanSwapChainCreateInfo

struct SVulkanSwapChainCreateInfo
{
    VkExtent2D       Extent;
    EFormat          Format;
    VkColorSpaceKHR  ColorSpace;
    uint32           BufferCount;
    bool             bVerticalSync;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSwapChain

class CVulkanSwapChain : public CVulkanDeviceObject, public CRefCounted
{
public:

    static CVulkanSwapChainRef CreateSwapChain(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CVulkanSurface* InSurface, const SVulkanSwapChainCreateInfo& CreateInfo);

    VkResult Present(CVulkanSemaphore** WaitSemaphores, uint32 NumWaitSemaphores, uint32 ImageIndex);

    FORCEINLINE VkResult GetPresentResult() const
    {
        return PresentResult;
    }

    FORCEINLINE VkImage GetImage(uint32 ImageIndex) const
    {
        return Images[ImageIndex];
    }

    FORCEINLINE uint32 GetImageCount() const
    {
        return Images.Size();
    }

    FORCEINLINE CVulkanSurface* GetSurface() const
    {
        return Surface.Get();
    }

    FORCEINLINE VkSwapchainKHR GetVkSwapChain() const
    {
        return SwapChain;
    }

private:
    CVulkanSwapChain(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CVulkanSurface* InSurface);
    ~CVulkanSwapChain();

    bool Initialize(const SVulkanSwapChainCreateInfo& CreateInfo);
    
    bool RetrieveSwapChainImages();
    
    VkResult AquireNextImage();

    FORCEINLINE void AquireNextBufferIndex()
    {
        SemaphoreIndex = (SemaphoreIndex + 1) % Images.Size();
    }

    VkResult          PresentResult;
    VkSwapchainKHR    SwapChain;

    CVulkanSurfaceRef Surface;
    CVulkanQueueRef   Queue;

    uint32            BufferIndex;
    uint32            SemaphoreIndex;

    TInlineArray<VkImage         , NUM_BACK_BUFFERS> Images;
    TInlineArray<CVulkanSemaphore, NUM_BACK_BUFFERS> ImageSemaphores;
};