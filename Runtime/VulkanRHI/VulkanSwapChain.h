#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Misc/Debug.h"
#include "Core/RefCountedBase.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanSurface.h"
#include "VulkanRHI/VulkanQueue.h"
#include "VulkanRHI/VulkanSemaphore.h"
#include "VulkanRHI/VulkanTexture.h"

#define NUM_BACK_BUFFERS (3)

static constexpr int32 VULKAN_INVALID_BACK_BUFFER_INDEX = -1;

typedef TSharedRef<class FVulkanSwapChain>    FVulkanSwapChainRef;
typedef TSharedRef<class FVulkanSwapChainRHI> FVulkanSwapChainRHIRef;

class FVulkanCommandContext;

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
    FVulkanSurface*   Surface           = nullptr;
    FVulkanSwapChain* PreviousSwapChain = nullptr;
    VkExtent2D        Extent            = { 0, 0 };
    EFormat           Format            = EFormat::B8G8R8A8_Unorm;
    VkColorSpaceKHR   ColorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    uint32            BufferCount       = 2;
    bool              bVerticalSync     = true;
};

class FVulkanSwapChain : public FVulkanDeviceChild, public FRefCountedBase
{
public:
    FVulkanSwapChain(FVulkanDevice* InDevice);
    ~FVulkanSwapChain();

    bool     Initialize(const FVulkanSwapChainCreateInfo& CreateInfo);
    VkResult Present(FVulkanQueue& GraphicsQueue, FVulkanQueue* PresentQueue, FVulkanSemaphore* WaitSemaphore);
    VkResult AcquireNextImage(FVulkanSemaphore* AcquireSemaphore);
    bool     GetSwapChainImages(VkImage* OutImages);

    void ReleaseOwnershipForPresent(FVulkanCommandBuffer& GraphicsCmdBuffer, VkImage SwapChainImage);
    void AcquireOwnershipAfterPresent(FVulkanCommandBuffer& GraphicsCmdBuffer, VkImage SwapChainImage);
    
    VkResult           GetPresentResult()            const { return PresentResult; }
    VkSwapchainKHR     GetVkSwapChain()              const { return SwapChain; }
    VkExtent2D         GetExtent()                   const { return Extent; }
    VkSurfaceFormatKHR GetVkSurfaceFormat()          const { return Format; }
    uint32             GetBufferCount()              const { return BufferCount; }
    uint32             GetBufferIndex()              const { return BufferIndex; }
    bool               HasSeparatePresentQueue()     const { return GraphicsQueueFamilyIndex != PresentQueueFamilyIndex; }
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

class FVulkanSwapChainRHI final : public FRHISwapChain, public FVulkanDeviceChild
{
public:
    FVulkanSwapChainRHI(FVulkanDevice* InDevice, const FRHISwapChainDesc& InSwapChainDesc);
    virtual ~FVulkanSwapChainRHI();

    // FRHISwapChain Interface
    virtual FRHITexture* GetBackBuffer() const override final;
    virtual void* GetBackBufferRenderTargetView() override final;

    virtual void* GetNativeSwapChain() const override final
    {
        return reinterpret_cast<void*>(SwapChainResource->GetVkSwapChain());
    }

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

    FVulkanSwapChain* GetSwapChainHandle() const
    {
        return SwapChainResource.Get();
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

private:
    typedef TArray<FVulkanFence*, TInlineArrayAllocator<FVulkanFence*, NUM_BACK_BUFFERS>>             FVulkanFenceArray;
    typedef TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> FVulkanSemaphoreArray;

    void*                        WindowHandle;
    FVulkanSurfaceRef            Surface;
    FVulkanSwapChainRef          SwapChainResource;
    FVulkanBackBufferTextureRef  BackBuffer;
    TArray<FVulkanTextureRHIRef> BackBuffers;
    FVulkanFenceArray            ImageFences;
    FVulkanSemaphoreArray        ImageSemaphores;
    FVulkanSemaphoreArray        RenderSemaphores;
    int32                        SemaphoreIndex;
    int32                        BackBufferIndex;
    int32                        ActiveBackBufferCount;
    bool                         bActiveVSync;
};
