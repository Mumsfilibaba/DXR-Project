#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanDeviceChild.h"

typedef TSharedRef<class FVulkanShaderResourceViewRHI>  FVulkanShaderResourceViewRHIRef;
typedef TSharedRef<class FVulkanUnorderedAccessViewRHI> FVulkanUnorderedAccessViewRHIRef;

class FVulkanResourceView : public FVulkanDeviceChild
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

    bool InitializeAsImageView(VkImage InImage, VkFormat InFormat, VkImageViewType InImageViewType, VkImageAspectFlags InAspectMask, uint32 InBaseArrayLayer, uint32 InLayerCount, uint32 InBaseMipLevel, uint32 InLevelCount);
    bool InitializeAsStructuredBufferView(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange);
    bool InitializeAsTypedBufferView(VkBuffer InBuffer, VkFormat InFormat, VkDeviceSize InOffset, VkDeviceSize InRange);
    bool InitializeAsAccelerationStructureView(VkAccelerationStructureKHR InAccelerationStructure);

    void SetDebugName(const FString& InName);

    EType GetType() const
    {
        return Type;
    }

    const FStructuredBufferView&      GetStructuredBufferInfo()      const { return StructuredBufferInfo; }
    const FTypedBufferView&           GetTypedBufferInfo()           const { return TypedBufferInfo; }
    const FImageView&                 GetImageViewInfo()             const { return ImageViewInfo; }
    const FAccelerationStructureView& GetAccelerationStructureInfo() const { return AccelerationStructureInfo; }

protected:
    EType Type;
    union
    {
        FStructuredBufferView      StructuredBufferInfo;
        FTypedBufferView           TypedBufferInfo;
        FImageView                 ImageViewInfo;
        FAccelerationStructureView AccelerationStructureInfo;
    };
};

class FVulkanShaderResourceViewRHI : public FRHIShaderResourceView, public FVulkanResourceView
{
public:
    FVulkanShaderResourceViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanShaderResourceViewRHI() = default;

    bool InitializeSRV(const FRHIShaderResourceViewDesc& InDesc);

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
};

class FVulkanUnorderedAccessViewRHI : public FRHIUnorderedAccessView, public FVulkanResourceView
{
public:
    FVulkanUnorderedAccessViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanUnorderedAccessViewRHI() = default;

    bool InitializeUAV(const FRHIUnorderedAccessViewDesc& InDesc);

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
};
