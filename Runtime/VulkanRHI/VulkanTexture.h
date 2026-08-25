#pragma once
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanResourceState.h"

class FVulkanSwapChainRHI;
class FVulkanCommandContext;

typedef TSharedRef<FVulkanSwapChainRHI>     FVulkanSwapChainRHIRef;
typedef TSharedRef<class FVulkanTextureRHI> FVulkanTextureRHIRef;

class FVulkanTextureRHI : public FRHITexture, public FVulkanResource
{
    friend class FVulkanSwapChainRHI;

public:
    FVulkanTextureRHI(FVulkanDevice* InDevice, const FRHITextureDesc& InTextureDesc);
    virtual ~FVulkanTextureRHI();

    // FRHITexture Interface
    virtual void* GetRHINativeResource() const override final;
    
    virtual FRHIDescriptorHandle     GetBindlessSRVHandle()   const override final;
    virtual FRHIDescriptorHandle     GetBindlessUAVHandle()   const override final;
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final;
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final;
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final;
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const override final;
    
    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;
    
    bool Initialize(FVulkanCommandContext* InCommandContext, ERHIResourceState InInitialAccess, const IRHITextureData* InInitialData);

    void SetResourceStateTrackingMode(ERHIResourceStateTrackingMode InTrackingMode)
    {
        Desc.TrackingMode = InTrackingMode;
    }

    void SetVkImage(VkImage InImage, VkImageLayout InLayout);

    FVulkanImageLayoutState&       GetImageLayoutState()       { return ImageLayoutState; }
    const FVulkanImageLayoutState& GetImageLayoutState() const { return ImageLayoutState; }

    VkImage GetVkImage() const
    {
        return Image;
    }
    
    const VkImageCreateInfo& GetVkImageCreateInfo() const
    {
        return CreateInfo;
    }

    VkFormat GetVkFormat() const
    {
        return CreateInfo.format;
    }

private:
    bool InitializeSwapChainTexture();

    void SetSwapChainImage(VkImage InImage, EFormat InFormat, uint32 InWidth, uint32 InHeight);

    void UpdateImage(VkImage InImage)
    {
        Image = InImage;
    }

    VkImage                          Image;
    VkImageCreateInfo                CreateInfo;
    FVulkanImageLayoutState          ImageLayoutState;
    FVulkanShaderResourceViewRHIRef  ShaderResourceView;
    FVulkanUnorderedAccessViewRHIRef UnorderedAccessView;
    FVulkanRenderTargetViewRHIRef    RenderTargetView;
    FVulkanDepthStencilViewRHIRef    DepthStencilView;
#if VULKAN_STORE_DEBUG_NAMES
    String                           DebugName;
#endif
};
