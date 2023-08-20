#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanLoader.h"
#include "VulkanRefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanImageView>           FVulkanImageViewRef;
typedef TSharedRef<class FVulkanShaderResourceView>  FVulkanShaderResourceViewRef;
typedef TSharedRef<class FVulkanDepthStencilView>    FVulkanDepthStencilViewRef;
typedef TSharedRef<class FVulkanRenderTargetView>    FVulkanRenderTargetViewRef;
typedef TSharedRef<class FVulkanUnorderedAccessView> FVulkanUnorderedAccessViewRef;

class FVulkanImageView : public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanImageView(FVulkanDevice* InDevice);
    ~FVulkanImageView();

    bool CreateView(VkImage InImage, VkImageViewType ViewType, VkFormat Format, VkImageViewCreateFlags Flags, const VkImageSubresourceRange& InSubresourceRange);
    
    void DestroyView();

    FORCEINLINE VkImage GetVkImage() const
    {
        return Image;
    }
    
    FORCEINLINE VkImageView GetVkImageView() const
    {
        return ImageView;
    }

    FORCEINLINE const VkImageSubresourceRange& GetSubresourceRange() const
    {
        return SubresourceRange;
    }
    
private:
    VkImageSubresourceRange SubresourceRange;
    VkImage                 Image;
    VkImageView             ImageView;
};

class FVulkanShaderResourceView : public FRHIShaderResourceView, public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanShaderResourceView(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanShaderResourceView() = default;

    bool Initialize() { return true; }

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }

    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
};

class FVulkanUnorderedAccessView : public FRHIUnorderedAccessView, public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanUnorderedAccessView(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanUnorderedAccessView() = default;

    bool Initialize() { return true; }
    
    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }

    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
};

class FVulkanRenderTargetView : public FVulkanRefCounted
{
public:
    FVulkanRenderTargetView() = default;
    virtual ~FVulkanRenderTargetView() = default;
    
    void SetImageView(const FVulkanImageViewRef& NewImageView)
    {
        ImageView = NewImageView;
    }
    
    FORCEINLINE FVulkanImageView* GetImageView() const
    {
        return ImageView.Get();
    }
    
private:
    FVulkanImageViewRef ImageView;
};

class FVulkanDepthStencilView : public FVulkanRefCounted
{
public:
    FVulkanDepthStencilView()  = default;
    virtual ~FVulkanDepthStencilView() = default;
};
