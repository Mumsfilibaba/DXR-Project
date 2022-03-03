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
    CVulkanSwapChain* PreviousSwapChain = nullptr;
    CVulkanSurface*   Surface           = nullptr;
    VkExtent2D        Extent            = { 0, 0 };
    EFormat           Format            = EFormat::B8G8R8A8_Unorm;
    VkColorSpaceKHR   ColorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    uint32            BufferCount       = 2;
    bool              bVerticalSync     = true;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSwapChain

class CVulkanSwapChain : public CVulkanDeviceObject, public CRefCounted
{
public:

    static CVulkanSwapChainRef CreateSwapChain(CVulkanDevice* InDevice, const SVulkanSwapChainCreateInfo& CreateInfo);

    VkResult Present(CVulkanQueue* Queue, CVulkanSemaphore* WaitSemaphore);

    VkResult AquireNextImage(CVulkanSemaphore* AquireSemaphore);
    
    bool GetSwapChainImages(VkImage* OutImages);
    
    FORCEINLINE VkResult GetPresentResult() const 
    { 
        return PresentResult; 
    }

    FORCEINLINE VkSwapchainKHR GetVkSwapChain() const 
    { 
        return SwapChain; 
    }
    
    FORCEINLINE uint32 GetBufferCount() const 
    { 
        return BufferCount; 
    }

    FORCEINLINE uint32 GetBufferIndex() const 
    { 
        return BufferIndex; 
    }

private:
    CVulkanSwapChain(CVulkanDevice* InDevice);
    ~CVulkanSwapChain();

    bool Initialize(const SVulkanSwapChainCreateInfo& CreateInfo);
	
    VkResult       PresentResult;
    VkSwapchainKHR SwapChain;
    uint32         BufferIndex;
    uint32         BufferCount;
};
