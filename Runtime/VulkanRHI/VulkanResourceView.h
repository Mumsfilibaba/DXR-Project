#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanLoader.h"
#include "VulkanRefCounted.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanImageView>           FVulkanImageViewRef;
typedef TSharedRef<class FVulkanShaderResourceView>  FVulkanShaderResourceViewRef;
typedef TSharedRef<class FVulkanUnorderedAccessView> FVulkanUnorderedAccessViewRef;

class FVulkanImageView : public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanImageView(FVulkanDevice* InDevice);
    ~FVulkanImageView();

    bool CreateView(VkImage InImage, VkImageViewType ViewType, VkFormat InFormat, VkImageViewCreateFlags Flags, const VkImageSubresourceRange& InSubresourceRange);
    
    void DestroyView();

    VkImage GetVkImage() const
    {
        return Image;
    }
    
    VkImageView GetVkImageView() const
    {
        return ImageView;
    }

    const VkImageSubresourceRange& GetSubresourceRange() const
    {
        return SubresourceRange;
    }
    
    VkFormat GetVkFormat() const
    {
        return Format;
    }
    
private:
    VkImageSubresourceRange SubresourceRange;
    VkImage                 Image;
    VkImageView             ImageView;
    VkFormat                Format;
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
