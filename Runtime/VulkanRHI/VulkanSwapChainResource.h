#pragma once
#include "Core/Misc/Debug.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanSurface.h"
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanSemaphore.h"

#define NUM_BACK_BUFFERS (3)

typedef TSharedRef<class FVulkanSwapChainResource> FVulkanSwapChainResourceRef;

inline bool IsUndefinedExtent(const VkSurfaceCapabilitiesKHR& Capabilities)
{
	return Capabilities.currentExtent.width == UINT32_MAX || Capabilities.currentExtent.height == UINT32_MAX;
}

inline bool IsExtentZero(const VkExtent2D& Extent)
{
	return Extent.width == 0 && Extent.height == 0;
}

struct FVulkanSwapChainCreateInfo
{
    FVulkanSurface*           Surface           = nullptr;
    FVulkanSwapChainResource* PreviousSwapChain = nullptr;
    VkExtent2D                Extent            = { 0, 0 };
    EFormat                   Format            = EFormat::B8G8R8A8_Unorm;
    VkColorSpaceKHR           ColorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    uint32                    BufferCount       = 2;
    bool                      bVerticalSync     = true;
};

class FVulkanSwapChainResource : public FVulkanDeviceChild, public FVulkanRefCounted
{
public:
    FVulkanSwapChainResource(FVulkanDevice* InDevice);
    ~FVulkanSwapChainResource();

    bool     Initialize(const FVulkanSwapChainCreateInfo& CreateInfo);
    VkResult Present(FVulkanQueue& GraphicsQueue, FVulkanQueue* PresentQueue, FVulkanSemaphore* WaitSemaphore);
    VkResult AcquireNextImage(FVulkanSemaphore* AcquireSemaphore);
    bool     GetSwapChainImages(VkImage* OutImages);

    void ReleaseOwnershipForPresent(FVulkanCommandBuffer& GraphicsCmdBuffer, VkImage SwapChainImage);
    void AcquireOwnershipAfterPresent(FVulkanCommandBuffer& GraphicsCmdBuffer, VkImage SwapChainImage);
    
    VkResult           GetPresentResult()          const { return PresentResult; }
    VkSwapchainKHR     GetVkSwapChain()            const { return SwapChain; }
    VkExtent2D         GetExtent()                 const { return Extent; }
    VkSurfaceFormatKHR GetVkSurfaceFormat()        const { return Format; }
    uint32             GetBufferCount()            const { return BufferCount; }
    uint32             GetBufferIndex()            const { return BufferIndex; }
    bool               HasSeparatePresentQueue()   const { return GraphicsQueueFamilyIndex != PresentQueueFamilyIndex; }
    uint32             GetGraphicsQueueFamilyIndex() const { return GraphicsQueueFamilyIndex; }
    uint32             GetPresentQueueFamilyIndex()  const { return PresentQueueFamilyIndex; }

private:
    VkResult           PresentResult;
    VkSwapchainKHR     SwapChain;
    VkExtent2D         Extent;
    uint32             BufferIndex;
    uint32             BufferCount;
    VkSurfaceFormatKHR Format;
    uint32             GraphicsQueueFamilyIndex = 0;
    uint32             PresentQueueFamilyIndex  = 0;
};
