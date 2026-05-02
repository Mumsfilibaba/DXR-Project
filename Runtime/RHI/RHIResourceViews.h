#pragma once
#include "Core/Containers/StaticArray.h"
#include "RHI/RHIResource.h"
#include "RHI/RHIPipelineState.h"

enum class EBufferSRVFormat : uint32
{
    None = 0,
    UInt32,
};

NODISCARD constexpr const CHAR* ToString(EBufferSRVFormat BufferSRVFormat)
{
    switch (BufferSRVFormat)
    {
        case EBufferSRVFormat::UInt32: return "UInt32";
        default: return "Unknown";
    }
}

enum class EBufferUAVFormat : uint32
{
    None = 0,
    UInt32,
};

NODISCARD constexpr const CHAR* ToString(EBufferUAVFormat BufferSRVFormat)
{
    switch (BufferSRVFormat)
    {
        case EBufferUAVFormat::UInt32: return "UInt32";
        default: return "Unknown";
    }
}
 
enum class EAttachmentLoadAction : uint8
{
    DontCare = 0, // Don't care
    Load,         // Use the stored data when RenderPass begin
    Clear,        // Clear data when RenderPass begin
};

NODISCARD constexpr const CHAR* ToString(EAttachmentLoadAction LoadAction)
{
    switch (LoadAction)
    {
        case EAttachmentLoadAction::DontCare: return "DontCare";
        case EAttachmentLoadAction::Load:     return "Load";
        case EAttachmentLoadAction::Clear:    return "Clear";

        default: return "Unknown";
    }
}

enum class EAttachmentStoreAction : uint8
{
    DontCare = 0, // Don't care
    Store,        // Store the data after the RenderPass is finished
};

NODISCARD constexpr const CHAR* ToString(EAttachmentStoreAction StoreAction)
{
    switch (StoreAction)
    {
        case EAttachmentStoreAction::DontCare: return "DontCare";
        case EAttachmentStoreAction::Store:    return "Store";

        default: return "Unknown";
    }
}

NODISCARD constexpr EFormat SafeGetFormat(FRHITexture* Texture)
{
    return Texture ? Texture->GetFormat() : EFormat::Unknown;
}

NODISCARD constexpr uint16 SafeGetFullResourceSliceCount(FRHITexture* Texture)
{
    if (!Texture)
    {
        return 1;
    }

    const FRHITextureDesc& Desc = Texture->GetDesc();
    switch (Desc.Dimension)
    {
        case ETextureDimension::Texture3D:
        {
            return static_cast<uint16>(Desc.Extent.Z);
        }
        
        case ETextureDimension::TextureCube:
        {
            return RHI_NUM_CUBE_FACES;
        }
        
        case ETextureDimension::TextureCubeArray:
        {
            return static_cast<uint16>(Desc.NumArraySlices * RHI_NUM_CUBE_FACES);
        }
        
        case ETextureDimension::Texture1DArray:
        case ETextureDimension::Texture2DArray:
        {
            return static_cast<uint16>(Desc.NumArraySlices);
        }
        
        default:
        {
            return 1;
        }
    }
}

struct FRHIShaderResourceViewDesc
{
public:
    enum class EType
    {
        Unknown = 0,
        BufferSRV,
        TextureSRV,
    };
    
    struct FBufferSRV
    {
        constexpr bool operator==(const FBufferSRV& Other) const noexcept = default;

        NODISCARD friend uint64 GetHashForType(const FBufferSRV& Value)
        {
            uint64 Hash = BitCast<UPTR_INT>(Value.Buffer);
            HashCombine(Hash, UnderlyingTypeValue(Value.Format));
            HashCombine(Hash, Value.FirstElement);
            HashCombine(Hash, Value.NumElements);
            return Hash;
        }

        FRHIBuffer*      Buffer;
        EBufferSRVFormat Format;
        uint32           FirstElement;
        uint32           NumElements;
    };

    struct FTextureSRV
    {
        constexpr bool operator==(const FTextureSRV& Other) const noexcept = default;

        NODISCARD friend uint64 GetHashForType(const FTextureSRV& Value)
        {
            uint64 Hash = BitCast<UPTR_INT>(Value.Texture);
            HashCombine(Hash, Value.MinLODClamp);
            HashCombine(Hash, UnderlyingTypeValue(Value.Format));
            HashCombine(Hash, Value.FirstMipLevel);
            HashCombine(Hash, Value.NumMips);
            HashCombine(Hash, Value.FirstArraySlice);
            HashCombine(Hash, Value.NumSlices);
            return Hash;
        }

        FRHITexture* Texture;
        float        MinLODClamp;
        EFormat      Format;
        uint8        FirstMipLevel;
        uint8        NumMips;
        uint16       FirstArraySlice;
        uint16       NumSlices;
    };

public:
    static FRHIShaderResourceViewDesc CreateBufferSRV(FRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements,
        EBufferSRVFormat InFormat = EBufferSRVFormat::None)
    {
        FRHIShaderResourceViewDesc ViewDesc;
        ViewDesc.Type                   = EType::BufferSRV;
        ViewDesc.BufferSRV.Buffer       = InBuffer;
        ViewDesc.BufferSRV.Format       = InFormat;
        ViewDesc.BufferSRV.FirstElement = InFirstElement;
        ViewDesc.BufferSRV.NumElements  = InNumElements;
        return ViewDesc;
    }

	static FRHIShaderResourceViewDesc CreateTextureSRV(FRHITexture* InTexture, EFormat InFormat, uint8 InFirstMipLevel,
		uint8 InNumMips, uint16 InFirstArraySlice, uint16 InNumSlices, float InMinLODClamp = 0.0f)
    {
        FRHIShaderResourceViewDesc ViewDesc;
        ViewDesc.Type                       = EType::TextureSRV;
        ViewDesc.TextureSRV.Texture         = InTexture;
        ViewDesc.TextureSRV.Format          = InFormat;
        ViewDesc.TextureSRV.MinLODClamp     = InMinLODClamp;
        ViewDesc.TextureSRV.FirstMipLevel   = InFirstMipLevel;
        ViewDesc.TextureSRV.NumMips         = InNumMips;
        ViewDesc.TextureSRV.FirstArraySlice = InFirstArraySlice;
        ViewDesc.TextureSRV.NumSlices       = InNumSlices;
        return ViewDesc;
    }

    NODISCARD constexpr bool IsBufferSRV()  const { return Type == EType::BufferSRV; }
    NODISCARD constexpr bool IsTextureSRV() const { return Type == EType::TextureSRV; }

    EType Type = EType::Unknown;
    union
    {
        FBufferSRV  BufferSRV;
        FTextureSRV TextureSRV;
    };
};

struct FRHIUnorderedAccessViewDesc
{
public:
    enum class EType
    {
        Unknown = 0,
        BufferUAV,
        TextureUAV,
    };

    struct FTextureUAV
    {
        constexpr bool operator==(const FTextureUAV& Other) const noexcept = default;

        NODISCARD friend uint64 GetHashForType(const FTextureUAV& Value)
        {
            uint64 Hash = BitCast<UPTR_INT>(Value.Texture);
            HashCombine(Hash, UnderlyingTypeValue(Value.Format));
            HashCombine(Hash, Value.MipLevel);
            HashCombine(Hash, Value.FirstArraySlice);
            HashCombine(Hash, Value.NumSlices);
            return Hash;
        }

        FRHITexture* Texture;
        EFormat      Format;
        uint8        MipLevel;
        uint16       FirstArraySlice;
        uint16       NumSlices;
    };

    struct FBufferUAV
    {
        constexpr bool operator==(const FBufferUAV& Other) const noexcept = default;

        NODISCARD friend uint64 GetHashForType(const FBufferUAV& Value)
        {
            uint64 Hash = BitCast<UPTR_INT>(Value.Buffer);
            HashCombine(Hash, UnderlyingTypeValue(Value.Format));
            HashCombine(Hash, Value.FirstElement);
            HashCombine(Hash, Value.NumElements);
            return Hash;
        }

        FRHIBuffer*      Buffer;
        EBufferUAVFormat Format;
        uint32           FirstElement;
        uint32           NumElements;
    };

public:
	static FRHIUnorderedAccessViewDesc CreateBufferUAV(FRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements, EBufferUAVFormat InFormat = EBufferUAVFormat::None)
	{
        FRHIUnorderedAccessViewDesc ViewDesc;
		ViewDesc.Type                   = EType::BufferUAV;
		ViewDesc.BufferUAV.Buffer       = InBuffer;
		ViewDesc.BufferUAV.Format       = InFormat;
		ViewDesc.BufferUAV.FirstElement = InFirstElement;
		ViewDesc.BufferUAV.NumElements  = InNumElements;
		return ViewDesc;
	}

	static FRHIUnorderedAccessViewDesc CreateTextureUAV(FRHITexture* InTexture, EFormat InFormat, uint8 InMipLevel, uint16 InFirstArraySlice, uint16 InNumSlices)
	{
        FRHIUnorderedAccessViewDesc ViewDesc;
		ViewDesc.Type                       = EType::TextureUAV;
		ViewDesc.TextureUAV.Texture         = InTexture;
		ViewDesc.TextureUAV.Format          = InFormat;
		ViewDesc.TextureUAV.MipLevel        = InMipLevel;
		ViewDesc.TextureUAV.FirstArraySlice = InFirstArraySlice;
		ViewDesc.TextureUAV.NumSlices       = InNumSlices;
		return ViewDesc;
	}

    NODISCARD constexpr bool IsBufferUAV()  const { return Type == EType::BufferUAV; }
    NODISCARD constexpr bool IsTextureUAV() const { return Type == EType::TextureUAV; }

    EType Type = EType::Unknown;
    union
    {
        FBufferUAV  BufferUAV;
        FTextureUAV TextureUAV;
    };
};

class FRHIResourceView : public FRHIResource
{
protected:
    explicit FRHIResourceView(FRHIResource* InResource)
        : FRHIResource()
        , Resource(InResource)
    {
    }

    virtual ~FRHIResourceView() = default;

public:
    FRHIResource* GetResource() const
    {
        return Resource;
    }

private:
    FRHIResource* Resource;
};

class FRHIShaderResourceView : public FRHIResourceView
{
protected:
    explicit FRHIShaderResourceView(FRHIResource* InResource)
        : FRHIResourceView(InResource)
    {
    }

    virtual ~FRHIShaderResourceView() = default;

public:

    // D3D12: D3D12_CPU_DESCRIPTOR_HANDLE::ptr. Vulkan: VkImageView / VkBufferView / VkAccelerationStructureKHR. Metal/Null: nullptr.
    virtual void* GetRHINativeHandle() const = 0;

    virtual FRHIDescriptorHandle GetBindlessHandle() const = 0;
};

class FRHIUnorderedAccessView : public FRHIResourceView
{
protected:
    explicit FRHIUnorderedAccessView(FRHIResource* InResource)
        : FRHIResourceView(InResource)
    {
    }

    virtual ~FRHIUnorderedAccessView() = default;

public:

    // D3D12: D3D12_CPU_DESCRIPTOR_HANDLE::ptr. Vulkan: VkImageView / VkBufferView. Metal/Null: nullptr.
    virtual void* GetRHINativeHandle() const = 0;

    virtual FRHIDescriptorHandle GetBindlessHandle() const = 0;
};

struct FRHIRenderTargetViewDesc
{
    FRHIRenderTargetViewDesc() noexcept = default;

    explicit FRHIRenderTargetViewDesc(FRHITexture* InTexture) noexcept
        : Texture(InTexture)
        , Format(SafeGetFormat(InTexture))
        , MipLevel(0)
        , ArrayIndex(0)
        , NumArraySlices(SafeGetFullResourceSliceCount(InTexture))
    {
    }

    FRHIRenderTargetViewDesc(FRHITexture* InTexture, EFormat InFormat, uint8 InMipLevel, uint16 InArrayIndex, uint16 InNumArraySlices) noexcept
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(InMipLevel)
        , ArrayIndex(InArrayIndex)
        , NumArraySlices(InNumArraySlices)
    {
    }

    constexpr bool operator==(const FRHIRenderTargetViewDesc& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHIRenderTargetViewDesc& Value)
    {
        uint64 Hash = BitCast<UPTR_INT>(Value.Texture);
        HashCombine(Hash, UnderlyingTypeValue(Value.Format));
        HashCombine(Hash, Value.MipLevel);
        HashCombine(Hash, Value.ArrayIndex);
        HashCombine(Hash, Value.NumArraySlices);
        return Hash;
    }

    FRHITexture* Texture        = nullptr;
    EFormat      Format         = EFormat::Unknown;
    uint8        MipLevel       = 0;
    uint16       ArrayIndex     = 0;
    uint16       NumArraySlices = 1;
};

class FRHIRenderTargetView : public FRHIResourceView
{
protected:
    explicit FRHIRenderTargetView(FRHIResource* InResource)
        : FRHIResourceView(InResource)
    {
    }

    virtual ~FRHIRenderTargetView() = default;

public:

    // D3D12: D3D12_CPU_DESCRIPTOR_HANDLE::ptr. Vulkan: VkImageView. Metal/Null: nullptr.
    virtual void* GetRHINativeHandle() const = 0;
};

struct FRHIDepthStencilViewDesc
{
    FRHIDepthStencilViewDesc() noexcept = default;

    explicit FRHIDepthStencilViewDesc(FRHITexture* InTexture) noexcept
        : Texture(InTexture)
        , Format(InTexture && InTexture->GetDesc().ClearValue.Format != EFormat::Unknown
            ? InTexture->GetDesc().ClearValue.Format
            : SafeGetFormat(InTexture))
        , MipLevel(0)
        , ArrayIndex(0)
        , NumArraySlices(SafeGetFullResourceSliceCount(InTexture))
    {
    }

    FRHIDepthStencilViewDesc(FRHITexture* InTexture, EFormat InFormat, uint8 InMipLevel, uint16 InArrayIndex, uint16 InNumArraySlices) noexcept
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(InMipLevel)
        , ArrayIndex(InArrayIndex)
        , NumArraySlices(InNumArraySlices)
    {
    }

    constexpr bool operator==(const FRHIDepthStencilViewDesc& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHIDepthStencilViewDesc& Value)
    {
        uint64 Hash = BitCast<UPTR_INT>(Value.Texture);
        HashCombine(Hash, UnderlyingTypeValue(Value.Format));
        HashCombine(Hash, Value.MipLevel);
        HashCombine(Hash, Value.ArrayIndex);
        HashCombine(Hash, Value.NumArraySlices);
        return Hash;
    }

    FRHITexture* Texture        = nullptr;
    EFormat      Format         = EFormat::Unknown;
    uint8        MipLevel       = 0;
    uint16       ArrayIndex     = 0;
    uint16       NumArraySlices = 1;
};

class FRHIDepthStencilView : public FRHIResourceView
{
protected:
    explicit FRHIDepthStencilView(FRHIResource* InResource)
        : FRHIResourceView(InResource)
    {
    }

    virtual ~FRHIDepthStencilView() = default;

public:

    // D3D12: D3D12_CPU_DESCRIPTOR_HANDLE::ptr. Vulkan: VkImageView. Metal/Null: nullptr.
    virtual void* GetRHINativeHandle() const = 0;
};

struct FRHIRenderPassAttachment
{
    FRHIRenderPassAttachment() noexcept = default;

    explicit FRHIRenderPassAttachment(FRHIRenderTargetView* InView, EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store, const FFloatColor& InClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f)) noexcept
        : View(InView)
        , ClearValue(InClearValue)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
    {
    }

    bool operator==(const FRHIRenderPassAttachment& Other) const noexcept = default;

    FRHIRenderTargetView*  View        = nullptr;
    FFloatColor            ClearValue  = { };
    EAttachmentLoadAction  LoadAction  = EAttachmentLoadAction::DontCare;
    EAttachmentStoreAction StoreAction = EAttachmentStoreAction::DontCare;
};

struct FRHIDepthStencilAttachment
{
    FRHIDepthStencilAttachment() noexcept = default;

    explicit FRHIDepthStencilAttachment(FRHIDepthStencilView* InView, EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store, const FDepthStencilValue& InClearValue = FDepthStencilValue(1.0f, 0)) noexcept
        : View(InView)
        , ClearValue(InClearValue)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
    {
    }

    bool operator==(const FRHIDepthStencilAttachment& Other) const noexcept = default;

    FRHIDepthStencilView*  View        = nullptr;
    FDepthStencilValue     ClearValue  = { };
    EAttachmentLoadAction  LoadAction  = EAttachmentLoadAction::DontCare;
    EAttachmentStoreAction StoreAction = EAttachmentStoreAction::DontCare;
};

struct FRHIBeginRenderPassDesc
{
    typedef TStaticArray<FRHIRenderPassAttachment, RHI_MAX_RENDER_TARGETS> FRenderTargetAttachments;

    FRHIBeginRenderPassDesc() noexcept = default;

    FRHIBeginRenderPassDesc(const FRenderTargetAttachments& InRenderTargets, uint32 InNumRenderTargets) noexcept
        : ShadingRateTexture(nullptr)
        , DepthStencilAttachment()
        , RenderTargets(InRenderTargets)
        , NumRenderTargets(InNumRenderTargets)
        , StaticShadingRate(EShadingRate::VRS_1x1)
        , ViewInstancingState()
    {
    }

    FRHIBeginRenderPassDesc(const FRenderTargetAttachments& InRenderTargets, uint32 InNumRenderTargets, FRHIDepthStencilAttachment InDepthStencilAttachment,
        FRHITexture* InShadingRateTexture = nullptr, EShadingRate InStaticShadingRate = EShadingRate::VRS_1x1) noexcept
        : ShadingRateTexture(InShadingRateTexture)
        , DepthStencilAttachment(InDepthStencilAttachment)
        , RenderTargets(InRenderTargets)
        , NumRenderTargets(InNumRenderTargets)
        , StaticShadingRate(InStaticShadingRate)
        , ViewInstancingState()
    {
    }

    bool operator==(const FRHIBeginRenderPassDesc& Other) const noexcept = default;

    FRHIViewInstancingState    ViewInstancingState    = { };
    FRHIDepthStencilAttachment DepthStencilAttachment = { };
    FRenderTargetAttachments   RenderTargets          = { };
    uint32                     NumRenderTargets       = 0;
    EShadingRate               StaticShadingRate      = EShadingRate::VRS_1x1;
    FRHITexture*               ShadingRateTexture     = nullptr;
};
