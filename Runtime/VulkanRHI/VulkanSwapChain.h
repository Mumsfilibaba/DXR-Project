#pragma once
#include "Core/Misc/Debug.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanSurface.h"
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanSemaphore.h"

#define NUM_BACK_BUFFERS (3)

typedef TSharedRef<class FVulkanSwapChain> FVulkanSwapChainRef;

struct FVulkanSwapChainCreateInfo
{
    FVulkanSwapChain* PreviousSwapChain = nullptr;
    FVulkanSurface*   Surface           = nullptr;
    VkExtent2D        Extent            = { 0, 0 };
    EFormat           Format            = EFormat::B8G8R8A8_Unorm;
    VkColorSpaceKHR   ColorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    uint32            BufferCount       = 2;
    bool              bVerticalSync     = true;
};

class FVulkanSwapChain : public FVulkanDeviceChild, public FVulkanRefCounted
{
public:
    FVulkanSwapChain(FVulkanDevice* InDevice);
    ~FVulkanSwapChain();

    bool Initialize(const FVulkanSwapChainCreateInfo& CreateInfo);
    VkResult Present(FVulkanQueue& Queue, FVulkanSemaphore* WaitSemaphore);
    VkResult AquireNextImage(FVulkanSemaphore* AquireSemaphore);
    bool GetSwapChainImages(VkImage* OutImages);
    
    VkResult GetPresentResult() const { return PresentResult; }
    VkSwapchainKHR GetVkSwapChain() const { return SwapChain; }
    VkExtent2D GetExtent() const { return Extent; }
    VkSurfaceFormatKHR GetVkSurfaceFormat() const { return Format; }
    uint32 GetBufferCount() const { return BufferCount; }
    uint32 GetBufferIndex() const { return BufferIndex; }

private:
    VkResult           PresentResult;
    VkSwapchainKHR     SwapChain;
    VkExtent2D         Extent;
    uint32             BufferIndex;
    uint32             BufferCount;
    VkSurfaceFormatKHR Format;
};
