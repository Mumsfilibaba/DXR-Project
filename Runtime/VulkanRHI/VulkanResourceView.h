#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanLoader.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"

#include "RHI/RHIResourceViews.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CVulkanShaderResourceView>  CVulkanShaderResourceViewRef;
typedef TSharedRef<class CVulkanDepthStencilView>    CVulkanDepthStencilViewRef;
typedef TSharedRef<class CVulkanRenderTargetView>    CVulkanRenderTargetViewRef;
typedef TSharedRef<class CVulkanUnorderedAccessView> CVulkanUnorderedAccessViewRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanImageView

class CVulkanImageView
{
public:
    CVulkanImageView() = default;

    bool CreateView(CVulkanDevice* InDevice, VkImage InImage, VkImageViewType ViewType, VkFormat Format, VkImageViewCreateFlags Flags, const VkImageSubresourceRange& SubresoureRange);
    void DestroyView(CVulkanDevice* InDevice);

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
        return true;
    }
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
