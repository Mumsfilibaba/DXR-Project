#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/UniquePtr.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanResourceViews.h"
#include "VulkanRHI/VulkanResourceState.h"
#include "VulkanRHI/VulkanMemory.h"

class FVulkanSwapChain;
class FVulkanSwapChainRHI;
class FVulkanCommandContext;

typedef TSharedRef<class FVulkanTextureRHI>           FVulkanTextureRHIRef;
typedef TSharedRef<class FVulkanBackBufferTexture> FVulkanBackBufferTextureRef;

struct VulkanTextureHelper
{
    static uint32 CalculateTextureRowPitch(VkFormat Format, uint32 Width);
    static uint32 CalculateTextureNumRows(VkFormat Format, uint32 Height);
    static uint64 CalculateTextureUploadSize(VkFormat Format, uint32 Width, uint32 Height);
};

class FVulkanTextureRHI : public FRHITexture, public FVulkanDeviceChild
{
public:
    FVulkanTextureRHI(FVulkanDevice* InDevice, const FRHITextureDesc& InTextureDesc);
    virtual ~FVulkanTextureRHI();

    bool Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const IRHITextureData* InInitialData);

    // FRHITexture Interface
    virtual void*                    GetRHINativeHandle()     const override { return reinterpret_cast<void*>(GetVkImage()); }
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final { return ShaderResourceView.Get(); }
    virtual FRHIDescriptorHandle     GetBindlessSRVHandle()   const override final { return FRHIDescriptorHandle(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return UnorderedAccessView.Get(); }
    virtual FRHIDescriptorHandle     GetBindlessUAVHandle()   const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    void EnableStateTracking(EResourceAccess InitialState);
    void DisableStateTracking(FVulkanCommandContext* CommandContext = nullptr);

    FVulkanResourceView* GetOrCreateImageView(const FVulkanHashableImageView& RenderTargetView);
    void DestroyImageViews();

    void SetVkImage(VkImage InImage);
    
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

    FVulkanImageLayoutState* GetImageLayoutState() const
    {
        return ImageLayoutState.Get();
    }
    
    // TODO: Solve in a cleaner way and remove this function
    void Resize(uint32 InWidth, uint32 InHeight)
    {
        Desc.Extent.X = InWidth;
        Desc.Extent.Y = InHeight;
    }

protected:
    VkImage                             Image;
    FVulkanMemoryAllocation             MemoryAllocation;
    VkImageCreateInfo                   CreateInfo;
    FVulkanShaderResourceViewRHIRef     ShaderResourceView;
    FVulkanUnorderedAccessViewRHIRef    UnorderedAccessView;
    TUniquePtr<FVulkanImageLayoutState> ImageLayoutState;
    TArray<FVulkanResourceView*>        ImageViews;
    FString                             DebugName;
    TMap<FVulkanHashableImageView, FVulkanResourceView*> ImageViewMap;
};

class FVulkanBackBufferTexture : public FVulkanTextureRHI
{
public:
    FVulkanBackBufferTexture(FVulkanDevice* InDevice, FVulkanSwapChainRHI* InSwapChain, const FRHITextureDesc& InTextureDesc);
    virtual ~FVulkanBackBufferTexture();

    void ResizeBackBuffer(int32 InWidth, int32 InHeight);
    FVulkanTextureRHI* GetCurrentBackBufferTexture(FVulkanCommandContext* InCommandContext);
    
    FVulkanSwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }
    
    void SetSwapChain(FVulkanSwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

private:
    FVulkanSwapChainRHI* SwapChain;
};
