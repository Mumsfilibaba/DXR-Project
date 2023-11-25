#pragma once
#include "VulkanResourceViews.h"
#include "VulkanDeviceObject.h"
#include "VulkanRefCounted.h"
#include "RHI/RHIResources.h"
#include "Core/Containers/SharedRef.h"

class FVulkanViewport;

typedef TSharedRef<FVulkanViewport>                FVulkanViewportRef;
typedef TSharedRef<class FVulkanTexture>           FVulkanTextureRef;
typedef TSharedRef<class FVulkanBackBufferTexture> FVulkanBackBufferTextureRef;

class FVulkanTexture : public FRHITexture, public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanTexture(FVulkanDevice* InDevice, const FRHITextureDesc& InDesc);
    virtual ~FVulkanTexture();

    bool Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }

    virtual void* GetRHIBaseTexture() override { return reinterpret_cast<void*>(static_cast<FVulkanTexture*>(this)); }
    
    virtual void* GetRHIBaseResource() const override { return reinterpret_cast<void*>(GetVkImage()); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return ShaderResourceView.Get(); }
    
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final;
    
    virtual FString GetName() const override final;

    FVulkanImageView* GetOrCreateRenderTargetView(const FRHIRenderTargetView& RenderTargetView);

    FVulkanImageView* GetOrCreateDepthStencilView(const FRHIDepthStencilView& DepthStencilView);

    void DestroyRenderTargetViews() { RenderTargetViews.Clear(); }

    void DestroyDepthStencilViews() { DepthStencilViews.Clear(); }

    void SetVkImage(VkImage InImage);
    
    VkImage GetVkImage() const
    {
        return Image;
    }

    VkFormat GetVkFormat() const
    {
        return Format;
    }

protected:
    VkImage        Image;
    VkDeviceMemory DeviceMemory;
    VkFormat       Format;

    FVulkanShaderResourceViewRef  ShaderResourceView;
    FVulkanUnorderedAccessViewRef UnorderedAccessView;
    TArray<FVulkanImageViewRef>   RenderTargetViews;
    TArray<FVulkanImageViewRef>   DepthStencilViews;
    
    FString DebugName;
};


class FVulkanBackBufferTexture : public FVulkanTexture
{
public:
    FVulkanBackBufferTexture(FVulkanDevice* InDevice, FVulkanViewport* InViewport, const FRHITextureDesc& InDesc);
    virtual ~FVulkanBackBufferTexture();

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(GetCurrentBackBufferTexture()); }
    
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetVkImage()); }

    void ResizeBackBuffer(int32 InWidth, int32 InHeight)
    {
        FPlatformMisc::MemoryBarrier();

        Desc.Extent.x = InWidth;
        Desc.Extent.y = InHeight;
    }

    FVulkanTexture* GetCurrentBackBufferTexture();
    
    FVulkanViewport* GetViewport() const
    {
        return Viewport;
    }
    
    void SetViewport(FVulkanViewport* InViewport)
    {
        Viewport = InViewport;
    }

private:
    FVulkanViewport* Viewport;
};


FORCEINLINE FVulkanTexture* GetVulkanTexture(FRHITexture* Texture)
{
    if (Texture)
    {
        FVulkanTexture* VulkanTexture = nullptr;
        if (IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::Presentable))
        {
            FVulkanBackBufferTexture* BackBuffer = static_cast<FVulkanBackBufferTexture*>(Texture);
            VulkanTexture = BackBuffer->GetCurrentBackBufferTexture();
        }
        else
        {
            VulkanTexture = static_cast<FVulkanTexture*>(Texture);
        }

        return VulkanTexture;
    }

    return nullptr;
}

FORCEINLINE VkImage GetVkImage(FRHITexture* Texture)
{
    FVulkanTexture* VulkanTexture = GetVulkanTexture(Texture);
    return VulkanTexture ? VulkanTexture->GetVkImage() : VK_NULL_HANDLE;
}


struct FVulkanTextureHelper
{
    static uint32 CalculateTextureRowPitch(VkFormat Format, uint32 Width);
    
    static uint32 CalculateTextureNumRows(VkFormat Format, uint32 Height);
    
    static uint64 CalculateTextureUploadSize(VkFormat Format, uint32 Width, uint32 Height);
};
