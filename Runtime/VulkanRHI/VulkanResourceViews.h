#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanDeviceChild.h"

typedef TSharedRef<class FVulkanShaderResourceView>  FVulkanShaderResourceViewRef;
typedef TSharedRef<class FVulkanUnorderedAccessView> FVulkanUnorderedAccessViewRef;

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

class FVulkanShaderResourceView : public FRHIShaderResourceView, public FVulkanResourceView
{
public:
    FVulkanShaderResourceView(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanShaderResourceView() = default;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    bool InitializeTextureSRV(const FRHITextureSRVInfo& InInfo);
    bool InitializeBufferSRV(const FRHIBufferSRVInfo& InInfo);
};

class FVulkanUnorderedAccessView : public FRHIUnorderedAccessView, public FVulkanResourceView
{
public:
    FVulkanUnorderedAccessView(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanUnorderedAccessView() = default;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    bool InitializeTextureUAV(const FRHITextureUAVInfo& InInfo);
    bool InitializeBufferUAV(const FRHIBufferUAVInfo& InInfo);
};
