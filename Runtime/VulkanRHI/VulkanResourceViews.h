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
        VkDeviceSize ViewOffset;
    };

    struct FTypedBufferView
    {
        VkBuffer     Buffer;
        VkBufferView BufferView;
        VkFormat     Format;
        VkDeviceSize Range;
        VkDeviceSize ViewOffset;
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
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) override;
    
    bool InitializeImageView(VkImage InImage, VkFormat InFormat, VkImageViewType InImageViewType, VkImageAspectFlags InAspectMask, uint32 InBaseArrayLayer, uint32 InLayerCount, uint32 InBaseMipLevel, uint32 InLevelCount);
    bool InitializeStructuredBufferView(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange, VkDeviceSize InViewOffset);
    bool InitializeTypedBufferView(VkBuffer InBuffer, VkFormat InFormat, VkDeviceSize InOffset, VkDeviceSize InRange);
    bool InitializeAccelerationStructureView(VkAccelerationStructureKHR InAccelerationStructure);

    void RegisterToResource(FVulkanResource* InOwner);
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

    FVulkanResource* GetOwnerResource() const
    {
        return OwnerResource;
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

    EType            Type;
    FVulkanResource* OwnerResource;
    uint32           DescriptorVersion;

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
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) override;

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
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) override;

    bool Initialize(const FRHIUnorderedAccessViewInfo& InInfo);
};
