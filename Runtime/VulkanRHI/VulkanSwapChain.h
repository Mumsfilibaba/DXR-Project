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
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanBackBufferProxies.h"

class FRHIRenderTargetView;

#define NUM_BACK_BUFFERS (3)

typedef TSharedRef<class FVulkanSwapChain>    FVulkanSwapChainRef;
typedef TSharedRef<class FVulkanSwapChainRHI> FVulkanSwapChainRHIRef;

class FVulkanCommandContext;

// Resolves the VulkanRHI.DefaultBackBufferFormat CVar into the EFormat used when the swap-chain
// is created with EFormat::Unknown. Read directly during backend init.
EFormat GetVulkanDefaultBackBufferFormat();

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
    FVulkanSurface*      Surface           = nullptr;
    FVulkanSwapChain*    PreviousSwapChain = nullptr;
    VkExtent2D           Extent            = { 0, 0 };
    EFormat              Format            = EFormat::B8G8R8A8_Unorm;
    VkColorSpaceKHR      ColorSpace        = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    ESwapChainUsageFlags Usage             = ESwapChainUsageFlags::RenderTarget;
    uint32               BufferCount       = 2;
    bool                 bVerticalSync     = true;
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
    FVulkanSwapChainRHI(FVulkanDevice* InDevice, FVulkanCommandContext* InCommandContext, const FRHISwapChainDesc& InSwapChainDesc);
    virtual ~FVulkanSwapChainRHI();

    // FRHISwapChain Interface
    virtual void* GetRHINativeHandle()                                             const override final;
    virtual void* GetRHINativeBackBufferResourceFromIndex(uint32 Index)            const override final;
    virtual void* GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 Index)    const override final;
    virtual void* GetRHINativeBackBufferUnorderedAccessViewFromIndex(uint32 Index) const override final;

    virtual FRHITexture* GetBackBuffer()                              const override final;
    virtual FRHITexture* GetBackBufferResourceFromIndex(uint32 Index) const override final;
    virtual uint32       GetNumBackBufferResources()                  const override final;

    virtual FRHIRenderTargetView*    GetBackBufferRenderTargetView()    const override final;
    virtual FRHIUnorderedAccessView* GetBackBufferUnorderedAccessView() const override final;

    virtual bool IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const override final;

    bool Initialize();
    
    bool Resize(uint32 InWidth, uint32 InHeight, EFormat NewFormat, EColorSpace NewColorSpace);
    bool Present(bool bVerticalSync);

    FVulkanTextureRHI*             GetCurrentBackBuffer() const;
    FVulkanRenderTargetViewRHI*    GetCurrentBackBufferRenderTargetView() const;
    FVulkanUnorderedAccessViewRHI* GetCurrentBackBufferUnorderedAccessView() const;

    void SetDebugName(const String& InName);

    FVulkanTextureRHI* GetBackBufferAtIndex(uint32 Index) const
    {
        return BackBuffers.IsValidIndex(Index) ? BackBuffers[Index].Texture.Get() : nullptr;
    }

    FVulkanRenderTargetViewRHI* GetBackBufferRenderTargetViewAtIndex(uint32 Index) const
    {
        return BackBuffers.IsValidIndex(Index) ? BackBuffers[Index].RenderTargetView.Get() : nullptr;
    }

    FVulkanUnorderedAccessViewRHI* GetBackBufferUnorderedAccessViewAtIndex(uint32 Index) const
    {
        return BackBuffers.IsValidIndex(Index) ? BackBuffers[Index].UnorderedAccessView.Get() : nullptr;
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

    FVulkanCommandContext* GetCommandContext() const
    {
        return CommandContext;
    }

private:
    VkResult AcquireNextImage();
    bool     RecreateSurface();
    bool     ValidateSurfaceAndSize(uint32& OutWidth, uint32& OutHeight);
    bool     CreateSwapChain(uint32 InWidth, uint32 InHeight);
    void     DestroySwapChain();

    void AdvanceSemaphoreIndex()
    {
        SemaphoreIndex = (SemaphoreIndex + 1) % ImageSemaphores.Size();
    }

private:
    typedef TArray<FVulkanFence*, TInlineArrayAllocator<FVulkanFence*, NUM_BACK_BUFFERS>>             FVulkanFenceArray;
    typedef TArray<FVulkanSemaphoreRef, TInlineArrayAllocator<FVulkanSemaphoreRef, NUM_BACK_BUFFERS>> FVulkanSemaphoreArray;

    struct FBackBufferData
    {
        FVulkanTextureRHIRef             Texture;
        FVulkanRenderTargetViewRHIRef    RenderTargetView;
        FVulkanUnorderedAccessViewRHIRef UnorderedAccessView;
    };

    void*                                           WindowHandle;
    FVulkanCommandContext*                          CommandContext;
    FVulkanSurfaceRef                               Surface;
    FVulkanSurfaceRef                               RetiredSurface;
    FVulkanSwapChainRef                             SwapChainResource;
    FVulkanBackBufferProxyTextureRHIRef             BackBufferProxy;
    FVulkanBackBufferProxyRenderTargetViewRHIRef    BackBufferProxyRenderTargetView;
    FVulkanBackBufferProxyUnorderedAccessViewRHIRef BackBufferProxyUnorderedAccessView;
    TArray<FBackBufferData>                         BackBuffers;
    FVulkanFenceArray                               ImageFences;
    FVulkanSemaphoreArray                           ImageSemaphores;
    FVulkanSemaphoreArray                           RenderSemaphores;
    EColorSpace                                     CurrentColorSpace;
    int32                                           SemaphoreIndex;
    uint32                                          BackBufferIndex;
    int32                                           ActiveBackBufferCount;
    bool                                            bActiveVSync;
};
