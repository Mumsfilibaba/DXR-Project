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

struct FRHIShaderResourceViewInfo
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
    static FRHIShaderResourceViewInfo CreateBufferSRV(FRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements,
        EBufferSRVFormat InFormat = EBufferSRVFormat::None)
    {
        FRHIShaderResourceViewInfo ViewInfo;
        ViewInfo.Type                   = EType::BufferSRV;
        ViewInfo.BufferSRV.Buffer       = InBuffer;
        ViewInfo.BufferSRV.Format       = InFormat;
        ViewInfo.BufferSRV.FirstElement = InFirstElement;
        ViewInfo.BufferSRV.NumElements  = InNumElements;
        return ViewInfo;
    }

	static FRHIShaderResourceViewInfo CreateTextureSRV(FRHITexture* InTexture, EFormat InFormat, uint8 InFirstMipLevel,
		uint8 InNumMips, uint16 InFirstArraySlice, uint16 InNumSlices, float InMinLODClamp = 0.0f)
    {
        FRHIShaderResourceViewInfo ViewInfo;
        ViewInfo.Type                       = EType::TextureSRV;
        ViewInfo.TextureSRV.Texture         = InTexture;
        ViewInfo.TextureSRV.Format          = InFormat;
        ViewInfo.TextureSRV.MinLODClamp     = InMinLODClamp;
        ViewInfo.TextureSRV.FirstMipLevel   = InFirstMipLevel;
        ViewInfo.TextureSRV.NumMips         = InNumMips;
        ViewInfo.TextureSRV.FirstArraySlice = InFirstArraySlice;
        ViewInfo.TextureSRV.NumSlices       = InNumSlices;
        return ViewInfo;
    }

    NODISCARD constexpr bool IsBufferSRV() const { return Type == EType::BufferSRV; }
    NODISCARD constexpr bool IsTextureSRV() const { return Type == EType::TextureSRV; }

    EType Type = EType::Unknown;
    union
    {
        FBufferSRV  BufferSRV;
        FTextureSRV TextureSRV;
    };
};

struct FRHIUnorderedAccessViewInfo
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
	static FRHIUnorderedAccessViewInfo CreateBufferUAV(FRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements, EBufferUAVFormat InFormat = EBufferUAVFormat::None)
	{
        FRHIUnorderedAccessViewInfo ViewInfo;
		ViewInfo.Type                   = EType::BufferUAV;
		ViewInfo.BufferUAV.Buffer       = InBuffer;
		ViewInfo.BufferUAV.Format       = InFormat;
		ViewInfo.BufferUAV.FirstElement = InFirstElement;
		ViewInfo.BufferUAV.NumElements  = InNumElements;
		return ViewInfo;
	}

	static FRHIUnorderedAccessViewInfo CreateTextureUAV(FRHITexture* InTexture, EFormat InFormat, uint8 InMipLevel, uint16 InFirstArraySlice, uint16 InNumSlices)
	{
        FRHIUnorderedAccessViewInfo ViewInfo;
		ViewInfo.Type                       = EType::TextureUAV;
		ViewInfo.TextureUAV.Texture         = InTexture;
		ViewInfo.TextureUAV.Format          = InFormat;
		ViewInfo.TextureUAV.MipLevel        = InMipLevel;
		ViewInfo.TextureUAV.FirstArraySlice = InFirstArraySlice;
		ViewInfo.TextureUAV.NumSlices       = InNumSlices;
		return ViewInfo;
	}

    NODISCARD constexpr bool IsBufferUAV() const { return Type == EType::BufferUAV; }
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
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
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
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};

struct FRHIRenderTargetView
{
    FRHIRenderTargetView() noexcept = default;

    FRHIRenderTargetView(FRHITexture* InTexture, EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store, const FFloatColor& InClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f)) noexcept
        : Texture(InTexture)
        , ClearValue(InClearValue)
        , ArrayIndex(0)
        , NumArraySlices(1)
        , Format(SafeGetFormat(InTexture))
        , MipLevel(0)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
    {
    }

    FRHIRenderTargetView(FRHITexture* InTexture, EFormat InFormat, uint32 InArrayIndex, uint32 InMipLevel, EAttachmentLoadAction InLoadAction,
        EAttachmentStoreAction InStoreAction, const FFloatColor& InClearValue) noexcept
        : Texture(InTexture)
        , ClearValue(InClearValue)
        , ArrayIndex(uint16(InArrayIndex))
        , NumArraySlices(1)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
    {
    }

    bool operator==(const FRHIRenderTargetView& Other) const noexcept = default;

    FRHITexture* Texture        = 0;
    FFloatColor  ClearValue     = { };
    uint16       ArrayIndex     = 0;
    uint16       NumArraySlices = 0;
    EFormat      Format         = EFormat::Unknown;
    uint8        MipLevel       = 0;

    EAttachmentLoadAction  LoadAction  = EAttachmentLoadAction::DontCare;
    EAttachmentStoreAction StoreAction = EAttachmentStoreAction::DontCare;
};

struct FRHIDepthStencilView
{
    FRHIDepthStencilView() noexcept = default;

    explicit FRHIDepthStencilView(FRHITexture* InTexture, EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store, const FDepthStencilValue& InClearValue = FDepthStencilValue(1.0f, 0)) noexcept
        : Texture(InTexture)
        , ClearValue(InClearValue)
        , ArrayIndex(0)
        , NumArraySlices(1)
        , Format(SafeGetFormat(InTexture))
        , MipLevel(0)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
    {
    }

    FRHIDepthStencilView(FRHITexture* InTexture, uint16 InArrayIndex, uint8 InMipLevel, EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store, const FDepthStencilValue& InClearValue = FDepthStencilValue(1.0f, 0)) noexcept
        : Texture(InTexture)
        , ClearValue(InClearValue)
        , ArrayIndex(uint16(InArrayIndex))
        , NumArraySlices(1)
        , Format(SafeGetFormat(InTexture))
        , MipLevel(uint8(InMipLevel))
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
    {
    }

    FRHIDepthStencilView(FRHITexture* InTexture, uint16 InArrayIndex, uint8 InMipLevel, EFormat InFormat, EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store, const FDepthStencilValue& InClearValue  = FDepthStencilValue(1.0f, 0)) noexcept
        : Texture(InTexture)
        , ClearValue(InClearValue)
        , ArrayIndex(uint16(InArrayIndex))
        , NumArraySlices(1)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
    {
    }

    bool operator==(const FRHIDepthStencilView& Other) const noexcept = default;

    FRHITexture*       Texture        = nullptr;
    FDepthStencilValue ClearValue     = { };
    uint16             ArrayIndex     = 0;
    uint16             NumArraySlices = 0;
    EFormat            Format         = EFormat::Unknown;
    uint8              MipLevel       = 0;

    EAttachmentLoadAction  LoadAction  = EAttachmentLoadAction::DontCare;
    EAttachmentStoreAction StoreAction = EAttachmentStoreAction::DontCare;
};

struct FRHIBeginRenderPassInfo
{
    typedef TStaticArray<FRHIRenderTargetView, RHI_MAX_RENDER_TARGETS> FRenderTargetViews;

    FRHIBeginRenderPassInfo() noexcept = default;

    FRHIBeginRenderPassInfo(const FRenderTargetViews& InRenderTargets, uint32 InNumRenderTargets) noexcept
        : ShadingRateTexture(nullptr)
        , DepthStencilView()
        , RenderTargets(InRenderTargets)
        , NumRenderTargets(InNumRenderTargets)
        , StaticShadingRate(EShadingRate::VRS_1x1)
        , ViewInstancingState()
    {
    }

    FRHIBeginRenderPassInfo(const FRenderTargetViews& InRenderTargets, uint32 InNumRenderTargets, FRHIDepthStencilView InDepthStencilView,
        FRHITexture* InShadingRateTexture = nullptr, EShadingRate InStaticShadingRate = EShadingRate::VRS_1x1) noexcept
        : ShadingRateTexture(InShadingRateTexture)
        , DepthStencilView(InDepthStencilView)
        , RenderTargets(InRenderTargets)
        , NumRenderTargets(InNumRenderTargets)
        , StaticShadingRate(InStaticShadingRate)
        , ViewInstancingState()
    {
    }

    bool operator==(const FRHIBeginRenderPassInfo& Other) const noexcept = default;

    FRHIDepthStencilView DepthStencilView = { };
    FRenderTargetViews RenderTargets = { };
    uint32 NumRenderTargets = 0;

    EShadingRate StaticShadingRate = EShadingRate::VRS_1x1;
    FRHITexture* ShadingRateTexture = nullptr;
    
    FRHIViewInstancingState ViewInstancingState = { };
};
