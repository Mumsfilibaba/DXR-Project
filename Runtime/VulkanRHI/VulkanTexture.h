#pragma once
#include "VulkanResourceView.h"
#include "VulkanDeviceObject.h"
#include "VulkanRefCounted.h"
#include "RHI/RHIResources.h"
#include "Core/Containers/SharedRef.h"

class FVulkanViewport;

typedef TSharedRef<FVulkanViewport>         FVulkanViewportRef;
typedef TSharedRef<class FVulkanTexture>    FVulkanTextureRef;
typedef TSharedRef<class FVulkanBackBuffer> FVulkanBackBufferRef;

class FVulkanTexture : public FRHITexture, public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanTexture(FVulkanDevice* InDevice, const FRHITextureDesc& InDesc);
    virtual ~FVulkanTexture();

    bool Initialize(EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }

    virtual void* GetRHIBaseTexture() override final { return reinterpret_cast<void*>(static_cast<FVulkanTexture*>(this)); }
    
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetVkImage()); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return ShaderResourceView.Get(); }
    
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }

    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final;
    
    virtual FString GetName() const override final;

    FVulkanRenderTargetView* GetOrCreateRTV(const FRHIRenderTargetView& RenderTargetView);

    FVulkanDepthStencilView* GetOrCreateDSV(const FRHIDepthStencilView& DepthStencilView);

    void DestroyRTVs() { RenderTargetViews.Clear(); }

    void DestroyDSVs() { DepthStencilViews.Clear(); }

    FORCEINLINE VkImage GetVkImage() const
    {
        return Image;
    }

    FORCEINLINE VkFormat GetVkFormat() const
    {
        return Format;
    }

protected:
    VkImage        Image;
    bool           bIsImageOwner;
    VkDeviceMemory DeviceMemory;
    VkFormat       Format;

    FVulkanShaderResourceViewRef       ShaderResourceView;
    FVulkanUnorderedAccessViewRef      UnorderedAccessView;

    TArray<FVulkanRenderTargetViewRef> RenderTargetViews; 
    TArray<FVulkanDepthStencilViewRef> DepthStencilViews;
};


class FVulkanBackBuffer : public FVulkanTexture
{
public:
    FVulkanBackBuffer(FVulkanDevice* InDevice, FVulkanViewport* InViewport, const FRHITextureDesc& InDesc);
    ~FVulkanBackBuffer() = default;

    void AquireNextImage();
    
    FORCEINLINE FVulkanViewport* GetViewport() const
    {
        return Viewport.Get();
    }

private:
    FVulkanViewportRef Viewport;
};


FORCEINLINE FVulkanTexture* GetVulkanTexture(FRHITexture* Texture)
{
    if (Texture)
    {
        FVulkanTexture* VulkanTexture = nullptr;
        if (IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::Presentable))
        {
            VulkanTexture = static_cast<FVulkanBackBuffer*>(Texture);
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
