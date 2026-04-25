#pragma once
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanResourceState.h"

class FVulkanSwapChainRHI;
class FVulkanCommandContext;
class FVulkanBackBufferProxyRenderTargetViewRHI;

typedef TSharedRef<FVulkanSwapChainRHI>                    FVulkanSwapChainRHIRef;
typedef TSharedRef<class FVulkanTextureRHI>                FVulkanTextureRHIRef;
typedef TSharedRef<class FVulkanBackBufferProxyTextureRHI> FVulkanBackBufferProxyTextureRHIRef;

class FVulkanTextureBase : public FRHITexture
{
protected:
    explicit FVulkanTextureBase(const FRHITextureDesc& InTextureDesc)
        : FRHITexture(InTextureDesc)
    {
    }

    virtual ~FVulkanTextureBase() = default;

public:
    virtual FVulkanTextureRHI* GetTextureInterface() const = 0;
};

class FVulkanTextureRHI : public FVulkanTextureBase, public FVulkanResource
{
public:
    FVulkanTextureRHI(FVulkanDevice* InDevice, const FRHITextureDesc& InTextureDesc);
    virtual ~FVulkanTextureRHI();

    // FVulkanTextureBase Interface
    virtual FVulkanTextureRHI* GetTextureInterface() const override;
    
    // FRHITexture Interface
    virtual void* GetRHINativeHandle() const override final;
    
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final;
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final;
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final;
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const override final;
    
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final;
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final;
    
    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;
    
    bool Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    void SetVkImage(VkImage InImage);
    
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
    
protected:
    FString                          DebugName;
    VkImage                          Image;
    VkImageCreateInfo                CreateInfo;
    FVulkanImageLayoutState          ImageLayoutState;
    FVulkanShaderResourceViewRHIRef  ShaderResourceView;
    FVulkanUnorderedAccessViewRHIRef UnorderedAccessView;
    FVulkanRenderTargetViewRHIRef    RenderTargetView;
    FVulkanDepthStencilViewRHIRef    DepthStencilView;
};

class FVulkanBackBufferProxyTextureRHI : public FVulkanTextureBase
{
public:
    FVulkanBackBufferProxyTextureRHI(FVulkanSwapChainRHI* InSwapChain, const FRHITextureDesc& InTextureDesc);
    virtual ~FVulkanBackBufferProxyTextureRHI();

    // FVulkanTextureBase Interface
    virtual FVulkanTextureRHI* GetTextureInterface() const override final;

    // FRHITexture Interface
    virtual void* GetRHINativeHandle() const override final;
    
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final;
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final;
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final;
    virtual FRHIDepthStencilView*    GetDepthStencilView()    const override final;
    
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final;
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final;
    
    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

    void Resize(uint32 InWidth, uint32 InHeight);

    void SetProxyRenderTargetView(FVulkanBackBufferProxyRenderTargetViewRHI* InProxyRenderTargetView);

    FVulkanSwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }

    void SetSwapChain(FVulkanSwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }
    
private:
    FVulkanSwapChainRHI*                                  SwapChain;
    TSharedRef<FVulkanBackBufferProxyRenderTargetViewRHI> ProxyRenderTargetView;
};
