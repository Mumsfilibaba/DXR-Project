#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanResource.h"

typedef TSharedRef<class FVulkanShaderResourceView>  FVulkanShaderResourceViewRef;
typedef TSharedRef<class FVulkanUnorderedAccessView> FVulkanUnorderedAccessViewRef;

class FVulkanResourceView : public FVulkanDeviceChild, public IVulkanResourceRelocationListener
{
public:
    enum class EType
    {
        None = 0,
        StructuredBufferView,
        TypedBufferView,
        ImageView,
        AccelerationStructureView,
    };

    struct FStructuredBufferView
    {
        VkBuffer     Buffer;
        VkDeviceSize Offset;
        VkDeviceSize Range;
    };

    struct FTypedBufferView
    {
        VkBuffer     Buffer;
        VkBufferView BufferView;
    };

    struct FImageView
    {
        VkImage                 Image;
        VkImageView             ImageView;
        VkImageViewType         ImageViewType;
        VkFormat                Format;
        VkImageViewCreateFlags  Flags;
        VkImageSubresourceRange SubresourceRange;
    };

    struct FAccelerationStructureView
    {
        VkAccelerationStructureKHR AccelerationStructure;
    };

public:
    FVulkanResourceView(FVulkanDevice* InDevice);
    virtual ~FVulkanResourceView();

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanGenericResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) override;
    
    bool InitializeImageView(VkImage InImage, VkFormat InFormat, VkImageViewType InImageViewType, VkImageAspectFlags InAspectMask, uint32 InBaseArrayLayer, uint32 InLayerCount, uint32 InBaseMipLevel, uint32 InLevelCount);
    bool InitializeStructuredBufferView(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange);
    bool InitializeTypedBufferView(VkBuffer InBuffer, VkFormat InFormat, VkDeviceSize InOffset, VkDeviceSize InRange);
    bool InitializeAccelerationStructureView(VkAccelerationStructureKHR InAccelerationStructure);

    void RegisterWithResource(FVulkanGenericResource* InOwner);
    void UnregisterFromResource();

    void SetDebugName(const FString& InName);
    
    const FStructuredBufferView& GetStructuredBufferInfo() const
    {
        CHECK(Type == EType::StructuredBufferView);
        return StructuredBufferInfo;
    }
    
    const FTypedBufferView& GetTypedBufferInfo() const
    {
        CHECK(Type == EType::TypedBufferView);
        return TypedBufferInfo;
    }

    const FImageView& GetImageViewInfo() const
    {
        CHECK(Type == EType::ImageView);
        return ImageViewInfo;
    }

    const FAccelerationStructureView& GetAccelerationStructureInfo() const
    {
        CHECK(Type == EType::AccelerationStructureView);
        return AccelerationStructureInfo;
    }

    EType GetType() const
    {
        return Type;
    }

    uint32 GetDescriptorVersion() const
    {
        return DescriptorVersion;
    }

protected:
    void IncrementDescriptorVersion()
    {
        ++DescriptorVersion;
    }

    EType                   Type;
    FVulkanGenericResource* OwnerResource;
    uint32                  DescriptorVersion;

    union
    {
        FStructuredBufferView      StructuredBufferInfo;
        FTypedBufferView           TypedBufferInfo;
        FImageView                 ImageViewInfo;
        FAccelerationStructureView AccelerationStructureInfo;
    };
};

class FVulkanShaderResourceView : public FRHIShaderResourceView, public FVulkanResourceView
{
public:
    FVulkanShaderResourceView(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanShaderResourceView() = default;

    // FRHIShaderResourceView Interface
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanGenericResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) override;

    bool Initialize(const FRHIShaderResourceViewInfo& InInfo);
};

class FVulkanUnorderedAccessView : public FRHIUnorderedAccessView, public FVulkanResourceView
{
public:
    FVulkanUnorderedAccessView(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanUnorderedAccessView() = default;

    // FRHIUnorderedAccessView Interface
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanGenericResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) override;

    bool Initialize(const FRHIUnorderedAccessViewInfo& InInfo);
};
