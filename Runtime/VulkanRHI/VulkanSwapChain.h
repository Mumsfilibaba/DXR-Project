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

    void GetSwapChainImages(TArray<VkImage>& OutImages);

    VkResult Present();

    FORCEINLINE VkImage GetImage(uint32 ImageIndex) const { return Images[ImageIndex]; }

    FORCEINLINE uint32 GetImageCount() const { return Images.Size(); }

    FORCEINLINE uint32 GetBufferIndex()    const { return BufferIndex; }
    FORCEINLINE uint32 GetSemaphoreIndex() const { return SemaphoreIndex; }

    FORCEINLINE CVulkanSurface* GetSurface() const { return Surface.Get(); }

    FORCEINLINE VkResult       GetPresentResult() const { return PresentResult; }
    FORCEINLINE VkSwapchainKHR GetVkSwapChain()   const { return SwapChain; }

private:
    CVulkanSwapChain(CVulkanDevice* InDevice, CVulkanQueue* InQueue, CVulkanSurface* InSurface);
    ~CVulkanSwapChain();

    bool Initialize(const SVulkanSwapChainCreateInfo& CreateInfo);
    
    bool RetrieveSwapChainImages();
    
    VkResult AquireNextImage();

    FORCEINLINE void AquireNextSemaphoreIndex()
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
    TInlineArray<CVulkanSemaphore, NUM_BACK_BUFFERS> RenderSemaphores;
};