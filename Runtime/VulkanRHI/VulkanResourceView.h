#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanLoader.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"

#include "RHI/RHIResourceViews.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CVulkanImageView>           CVulkanImageViewRef;
typedef TSharedRef<class CVulkanShaderResourceView>  CVulkanShaderResourceViewRef;
typedef TSharedRef<class CVulkanDepthStencilView>    CVulkanDepthStencilViewRef;
typedef TSharedRef<class CVulkanRenderTargetView>    CVulkanRenderTargetViewRef;
typedef TSharedRef<class CVulkanUnorderedAccessView> CVulkanUnorderedAccessViewRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanImageView

class CVulkanImageView : public CVulkanDeviceObject, public CRefCounted
{
public:
    CVulkanImageView(CVulkanDevice* InDevice);
    ~CVulkanImageView();

    bool CreateView(VkImage InImage, VkImageViewType ViewType, VkFormat Format, VkImageViewCreateFlags Flags, const VkImageSubresourceRange& SubresoureRange);
    void DestroyView();

    FORCEINLINE bool IsValid() const
    {
        return ImageView != VK_NULL_HANDLE;
    }

    FORCEINLINE VkImage GetVkImage() const
    {
        return Image;
    }
    
    FORCEINLINE VkImageView GetVkImageView() const
    {
        return ImageView;
    }
    
private:
    VkImage     Image;
    VkImageView ImageView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanShaderResourceView

class CVulkanShaderResourceView : public CRHIShaderResourceView
{
public:
    CVulkanShaderResourceView() = default;
    ~CVulkanShaderResourceView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanUnorderedAccessView

class CVulkanUnorderedAccessView : public CRHIUnorderedAccessView
{
public:
    CVulkanUnorderedAccessView() = default;
    ~CVulkanUnorderedAccessView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanRenderTargetView

class CVulkanRenderTargetView : public CRHIRenderTargetView
{
public:
    CVulkanRenderTargetView() = default;
    ~CVulkanRenderTargetView() = default;

    virtual bool IsValid() const override
    {
        return (ImageView != nullptr);
    }
    
    void SetImageView(const CVulkanImageViewRef& NewImageView)
    {
        ImageView = NewImageView;
    }
    
    FORCEINLINE CVulkanImageView* GetImageView() const
    {
        return ImageView.Get();
    }
    
private:
    CVulkanImageViewRef ImageView;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanDepthStencilView

class CVulkanDepthStencilView : public CRHIDepthStencilView
{
public:
    CVulkanDepthStencilView() = default;
    ~CVulkanDepthStencilView() = default;

    virtual bool IsValid() const override
    {
        return true;
    }
};
