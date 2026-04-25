#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanResource.h"

class FVulkanSwapChainRHI;
class FVulkanBackBufferProxyTextureRHI;

typedef TSharedRef<class FVulkanShaderResourceViewRHI>              FVulkanShaderResourceViewRHIRef;
typedef TSharedRef<class FVulkanUnorderedAccessViewRHI>             FVulkanUnorderedAccessViewRHIRef;
typedef TSharedRef<class FVulkanRenderTargetViewRHI>                FVulkanRenderTargetViewRHIRef;
typedef TSharedRef<class FVulkanDepthStencilViewRHI>                FVulkanDepthStencilViewRHIRef;
typedef TSharedRef<class FVulkanBackBufferProxyRenderTargetViewRHI> FVulkanBackBufferProxyRenderTargetViewRHIRef;

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
    
    bool InitializeImageView(
        VkImage InImage, 
        VkFormat InFormat, 
        VkImageViewType InImageViewType, 
        VkImageAspectFlags InAspectMask, 
        uint32 InBaseArrayLayer, 
        uint32 InLayerCount, 
        uint32 InBaseMipLevel, 
        uint32 InLevelCount);

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

class FVulkanShaderResourceViewRHI : public FRHIShaderResourceView, public FVulkanResourceView
{
public:
    FVulkanShaderResourceViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanShaderResourceViewRHI() = default;

    // FRHIShaderResourceView Interface
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) override;

    bool Initialize(const FRHIShaderResourceViewDesc& InDesc);
};

class FVulkanUnorderedAccessViewRHI : public FRHIUnorderedAccessView, public FVulkanResourceView
{
public:
    FVulkanUnorderedAccessViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanUnorderedAccessViewRHI() = default;

    // FRHIUnorderedAccessView Interface
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) override;

    bool Initialize(const FRHIUnorderedAccessViewDesc& InDesc);
};

class FVulkanRenderTargetViewBase : public FRHIRenderTargetView
{
protected:
    explicit FVulkanRenderTargetViewBase(FRHIResource* InResource)
        : FRHIRenderTargetView(InResource)
    {
    }

    virtual ~FVulkanRenderTargetViewBase() = default;

public:
    virtual FVulkanRenderTargetViewRHI* GetRenderTargetViewInterface() const = 0;
};

class FVulkanRenderTargetViewRHI : public FVulkanRenderTargetViewBase, public FVulkanResourceView
{
public:
    FVulkanRenderTargetViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanRenderTargetViewRHI() = default;

    // FVulkanRenderTargetViewBase Interface
    virtual FVulkanRenderTargetViewRHI* GetRenderTargetViewInterface() const override final;

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) override;

    bool Initialize(const FRHIRenderTargetViewDesc& InDesc);
};

class FVulkanBackBufferProxyRenderTargetViewRHI : public FVulkanRenderTargetViewBase
{
public:
    FVulkanBackBufferProxyRenderTargetViewRHI(FVulkanSwapChainRHI* InSwapChain, FVulkanBackBufferProxyTextureRHI* InProxyTexture);
    virtual ~FVulkanBackBufferProxyRenderTargetViewRHI();

    // FVulkanRenderTargetViewBase Interface
    virtual FVulkanRenderTargetViewRHI* GetRenderTargetViewInterface() const override final;

    void SetSwapChain(FVulkanSwapChainRHI* InSwapChain)
    {
        SwapChain = InSwapChain;
    }

    FVulkanSwapChainRHI* GetSwapChain() const
    {
        return SwapChain;
    }

private:
    FVulkanSwapChainRHI* SwapChain;
};

class FVulkanDepthStencilViewRHI : public FRHIDepthStencilView, public FVulkanResourceView
{
public:
    FVulkanDepthStencilViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource);
    virtual ~FVulkanDepthStencilViewRHI() = default;

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryStorage* NewMemoryStorage) override;

    bool Initialize(const FRHIDepthStencilViewDesc& InDesc);
};
