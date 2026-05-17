#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanResource.h"

class FVulkanSwapChainRHI;
class FVulkanShaderResourceViewRHI;
class FVulkanUnorderedAccessViewRHI;
class FVulkanRenderTargetViewRHI;

typedef TSharedRef<FVulkanShaderResourceViewRHI>     FVulkanShaderResourceViewRHIRef;
typedef TSharedRef<FVulkanUnorderedAccessViewRHI>    FVulkanUnorderedAccessViewRHIRef;
typedef TSharedRef<FVulkanRenderTargetViewRHI>       FVulkanRenderTargetViewRHIRef;
typedef TSharedRef<class FVulkanDepthStencilViewRHI> FVulkanDepthStencilViewRHIRef;

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
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryLocation* NewMemoryLocation) override;
    
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
    
    NODISCARD FRHIDescriptorHandle EnsureBindlessHandle(EDescriptorType InType, bool bWritable) const;
    void RefreshBindlessIfBound();

    void SetDebugName(const FString& InName);
    
    NODISCARD FORCEINLINE const FStructuredBufferView& GetStructuredBufferInfo() const
    {
        CHECK(Type == EType::StructuredBufferView);
        return StructuredBufferInfo;
    }
    
    NODISCARD FORCEINLINE const FTypedBufferView& GetTypedBufferInfo() const
    {
        CHECK(Type == EType::TypedBufferView);
        return TypedBufferInfo;
    }

    NODISCARD FORCEINLINE const FImageView& GetImageViewInfo() const
    {
        CHECK(Type == EType::ImageView);
        return ImageViewInfo;
    }

    NODISCARD FORCEINLINE const FAccelerationStructureView& GetAccelerationStructureInfo() const
    {
        CHECK(Type == EType::AccelerationStructureView);
        return AccelerationStructureInfo;
    }

    NODISCARD FORCEINLINE EType GetType() const
    {
        return Type;
    }

    NODISCARD FORCEINLINE void* GetRHINativeHandleForType() const
    {
        switch (Type)
        {
        case EType::ImageView:
            return reinterpret_cast<void*>(ImageViewInfo.ImageView);
        case EType::TypedBufferView:
            return reinterpret_cast<void*>(TypedBufferInfo.BufferView);
        case EType::AccelerationStructureView:
            return reinterpret_cast<void*>(AccelerationStructureInfo.AccelerationStructure);
        default:
            return nullptr;
        }
    }

    NODISCARD FORCEINLINE FVulkanResource* GetOwnerResource() const
    {
        return OwnerResource;
    }

    NODISCARD FORCEINLINE uint32 GetDescriptorVersion() const
    {
        return DescriptorVersion;
    }

protected:
    void IncrementDescriptorVersion()
    {
        ++DescriptorVersion;
        RefreshBindlessIfBound();
    }

    void FreeBindlessHandle();

    EType                        Type;
    FVulkanResource*             OwnerResource;
    uint32                       DescriptorVersion;
    mutable FRHIDescriptorHandle BindlessHandle;
    mutable bool                 bBindlessIsWritable;

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
    FVulkanShaderResourceViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc);
    virtual ~FVulkanShaderResourceViewRHI() = default;

    // FRHIShaderResourceView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryLocation* NewMemoryLocation) override;

    bool Initialize(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc);
};

class FVulkanUnorderedAccessViewBase : public FRHIUnorderedAccessView
{
protected:
    FVulkanUnorderedAccessViewBase(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
        : FRHIUnorderedAccessView(InResource, InDesc)
    {
    }

    virtual ~FVulkanUnorderedAccessViewBase() = default;

public:
    virtual FVulkanUnorderedAccessViewRHI* GetUnorderedAccessViewInterface() const = 0;
};

class FVulkanUnorderedAccessViewRHI : public FVulkanUnorderedAccessViewBase, public FVulkanResourceView
{
public:
    FVulkanUnorderedAccessViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc);
    virtual ~FVulkanUnorderedAccessViewRHI() = default;

    // FVulkanUnorderedAccessViewBase Interface
    virtual FVulkanUnorderedAccessViewRHI* GetUnorderedAccessViewInterface() const override final;

    // FRHIUnorderedAccessView Interface
    virtual void* GetRHINativeHandle() const override final;
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryLocation* NewMemoryLocation) override;

    bool Initialize(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc);
};

class FVulkanRenderTargetViewBase : public FRHIRenderTargetView
{
protected:
    FVulkanRenderTargetViewBase(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
        : FRHIRenderTargetView(InResource, InDesc)
    {
    }

    virtual ~FVulkanRenderTargetViewBase() = default;

public:
    virtual FVulkanRenderTargetViewRHI* GetRenderTargetViewInterface() const = 0;
};

class FVulkanRenderTargetViewRHI : public FVulkanRenderTargetViewBase, public FVulkanResourceView
{
public:
    FVulkanRenderTargetViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource, const FRHIRenderTargetViewDesc& InRHIDesc);
    virtual ~FVulkanRenderTargetViewRHI() = default;

    // FVulkanRenderTargetViewBase Interface
    virtual FVulkanRenderTargetViewRHI* GetRenderTargetViewInterface() const override final;

    // FRHIRenderTargetView Interface
    virtual void* GetRHINativeHandle() const override final;

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryLocation* NewMemoryLocation) override;

    bool Initialize(FRHITexture* InTexture, const FRHIRenderTargetViewDesc& InDesc);
};

class FVulkanDepthStencilViewRHI : public FRHIDepthStencilView, public FVulkanResourceView
{
public:
    FVulkanDepthStencilViewRHI(FVulkanDevice* InDevice, FRHIResource* InResource, const FRHIDepthStencilViewDesc& InRHIDesc);
    virtual ~FVulkanDepthStencilViewRHI() = default;

    // FRHIDepthStencilView Interface
    virtual void* GetRHINativeHandle() const override final;

    // IVulkanResourceRelocationListener Interface
    virtual void OnResourceRelocated(FVulkanResource* RelocatedResource, FVulkanMemoryLocation* NewMemoryLocation) override;

    bool Initialize(FRHITexture* InTexture, const FRHIDepthStencilViewDesc& InDesc);

    NODISCARD FORCEINLINE EDepthStencilViewFlags GetFlags() const
    {
        return Flags;
    }

    NODISCARD FORCEINLINE bool HasStencilFormat() const
    {
        return bHasStencil;
    }

    NODISCARD FORCEINLINE bool IsReadOnly() const
    {
        return IsDepthReadOnly() && (!HasStencilFormat() || IsStencilReadOnly());
    }

    NODISCARD FORCEINLINE bool IsDepthReadOnly() const
    {
        return IsEnumFlagSet(Flags, EDepthStencilViewFlags::ReadOnlyDepth);
    }

    NODISCARD FORCEINLINE bool IsStencilReadOnly() const
    {
        return IsEnumFlagSet(Flags, EDepthStencilViewFlags::ReadOnlyStencil);
    }

private:
    EDepthStencilViewFlags Flags;
    bool                   bHasStencil;
};
