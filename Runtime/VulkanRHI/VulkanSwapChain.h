#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanSemaphore.h"
#include "VulkanRHI/VulkanSurface.h"
#include "Core/Misc/Debug.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanQueue.h"

#define NUM_BACK_BUFFERS (3)
#define VULKAN_INVALID_BACK_BUFFER_INDEX (-1)

class FVulkanCommandContext;

typedef TSharedRef<class FVulkanSwapChain>    FVulkanSwapChainRef;
typedef TSharedRef<class FVulkanSwapChainRHI> FVulkanSwapChainRHIRef;

// Inline helper functions
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
    FVulkanSurface*         Surface           = nullptr;
    class FVulkanSwapChain* PreviousSwapChain = nullptr;
    VkExtent2D              Extent            = { 0, 0 };
    EFormat                 Format            = EFormat::B8G8R8A8_Unorm;
    VkColorSpaceKHR         ColorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    uint32                  BufferCount       = 2;
    bool                    bVerticalSync     = true;
};

class FVulkanSwapChain : public FVulkanDeviceChild, public FVulkanRefCounted
{
public:
    FVulkanSwapChain(FVulkanDevice* InDevice);
    ~FVulkanSwapChain();

    bool     Initialize(const FVulkanSwapChainCreateInfo& CreateInfo);
    VkResult Present(FVulkanQueue& Queue, FVulkanSemaphore* WaitSemaphore);
    VkResult AcquireNextImage(FVulkanSemaphore* AcquireSemaphore);
    bool     GetSwapChainImages(VkImage* OutImages);
    
    VkResult           GetPresentResult()   const { return PresentResult; }
    VkSwapchainKHR     GetVkSwapChain()     const { return SwapChain; }
    VkExtent2D         GetExtent()          const { return Extent; }
    VkSurfaceFormatKHR GetVkSurfaceFormat() const { return Format; }
    uint32             GetBufferCount()     const { return BufferCount; }
    uint32             GetBufferIndex()     const { return BufferIndex; }

private:
    VkResult           PresentResult;
    VkSwapchainKHR     SwapChain;
    VkExtent2D         Extent;
    uint32             BufferIndex;
    uint32             BufferCount;
    VkSurfaceFormatKHR Format;
};

class FVulkanSwapChainRHI final : public FRHISwapChain, public FVulkanDeviceChild
{
public:
    FVulkanSwapChainRHI(FVulkanDevice* InDevice, const FRHISwapChainDesc& InSwapChainDesc);
    virtual ~FVulkanSwapChainRHI();

    virtual FRHITexture* GetBackBuffer() const override final;

    bool Initialize(FVulkanCommandContext* InCommandContext);

    bool Resize(FVulkanCommandContext* InCommandContext, uint32 InWidth, uint32 InHeight);
    bool Present(FVulkanCommandContext* InCommandContext, bool bVerticalSync);
    FVulkanTextureRHI* GetCurrentBackBuffer(FVulkanCommandContext* InCommandContext);

    void SetDebugName(const FString& InName);

    FVulkanTextureRHI* GetBackBufferFromIndex(uint32 Index) const
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
    bool RecreateSurface(FVulkanCommandContext* InCommandContext);
    bool ValidateSurfaceAndSize(FVulkanCommandContext* InCommandContext, uint32& OutW, uint32& OutH);
    bool CreateSwapChain(FVulkanCommandContext* InCommandContext, uint32 InWidth, uint32 InHeight);
    void DestroySwapChain(FVulkanCommandContext* InCommandContext);
    VkResult AcquireNextImage(FVulkanCommandContext* InCommandContext);

    void AdvanceSemaphoreIndex()
    {
        SemaphoreIndex = (SemaphoreIndex + 1) % ImageSemaphores.Size();
    }

    typedef TArray<FVulkanFence*, TInlineArrayAllocator<FVulkanFence*, NUM_BACK_BUFFERS>> FVulkanFenceArray;
    typedef TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> FVulkanSemaphoreArray;

    void*                        WindowHandle;
    FVulkanSurfaceRef            Surface;
    FVulkanSwapChainRef          SwapChain;
    FVulkanBackBufferTextureRef  BackBuffer;
    TArray<FVulkanTextureRHIRef> BackBuffers;
    FVulkanFenceArray            ImageFences;
    FVulkanSemaphoreArray        ImageSemaphores;
    FVulkanSemaphoreArray        RenderSemaphores;
    int32                        SemaphoreIndex;
    int32                        BackBufferIndex;
};
