#pragma once
#include "Core/Containers/StaticArray.h"
#include "RHI/RHIResource.h"
#include "RHI/RHIPipelineState.h"

enum class EBufferSRVFormat : uint32
{
    None   = 0,
    UInt32 = 1,
};

NODISCARD constexpr const CHAR* ToString(EBufferSRVFormat BufferSRVFormat)
{
    switch (BufferSRVFormat)
    {
        case EBufferSRVFormat::UInt32: return "UInt32";
        default:                       return "Unknown";
    }
}

enum class EBufferUAVFormat : uint32
{
    None   = 0,
    UInt32 = 1,
};

NODISCARD constexpr const CHAR* ToString(EBufferUAVFormat BufferSRVFormat)
{
    switch (BufferSRVFormat)
    {
        case EBufferUAVFormat::UInt32: return "UInt32";
        default:                       return "Unknown";
    }
}

enum class EAttachmentLoadAction : uint8
{
    DontCare = 0, // Don't care
    Load     = 1, // Use the stored data when RenderPass begin
    Clear    = 2, // Clear data when RenderPass begin
};

NODISCARD constexpr const CHAR* ToString(EAttachmentLoadAction LoadAction)
{
    switch (LoadAction)
    {
        case EAttachmentLoadAction::DontCare: return "DontCare";
        case EAttachmentLoadAction::Load:     return "Load";
        case EAttachmentLoadAction::Clear:    return "Clear";
        default:                              return "Unknown";
    }
}

enum class EAttachmentStoreAction : uint8
{
    DontCare = 0, // Don't care
    Store    = 1, // Store the data after the RenderPass is finished
};

NODISCARD constexpr const CHAR* ToString(EAttachmentStoreAction StoreAction)
{
    switch (StoreAction)
    {
        case EAttachmentStoreAction::DontCare: return "DontCare";
        case EAttachmentStoreAction::Store:    return "Store";
        default:                               return "Unknown";
    }
}

struct FRHITextureSRVInfo
{
    constexpr FRHITextureSRVInfo() noexcept = default;

    constexpr FRHITextureSRVInfo(
        FRHITexture* InTexture,
        float        InMinLODClamp,
        EFormat      InFormat,
        uint8        InFirstMipLevel,
        uint8        InNumMips,
        uint16       InFirstArraySlice,
        uint16       InNumSlices) noexcept
        : Texture(InTexture)
        , MinLODClamp(InMinLODClamp)
        , Format(InFormat)
        , FirstMipLevel(InFirstMipLevel)
        , NumMips(InNumMips)
        , FirstArraySlice(InFirstArraySlice)
        , NumSlices(InNumSlices)
    {
    }

    constexpr bool operator==(const FRHITextureSRVInfo& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHITextureSRVInfo& Value)
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

    FRHITexture* Texture         = nullptr;
    float        MinLODClamp     = 0.0f;
    EFormat      Format          = EFormat::Unknown;
    uint8        FirstMipLevel   = 0;
    uint8        NumMips         = 0;
    uint16       FirstArraySlice = 0;
    uint16       NumSlices       = 0;
};

struct FRHIBufferSRVInfo
{
    constexpr FRHIBufferSRVInfo() noexcept = default;

    constexpr FRHIBufferSRVInfo(FRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements, EBufferSRVFormat InFormat = EBufferSRVFormat::None) noexcept
        : Buffer(InBuffer)
        , Format(InFormat)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    {
    }

    constexpr bool operator==(const FRHIBufferSRVInfo& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHIBufferSRVInfo& Value)
    {
        uint64 Hash = BitCast<UPTR_INT>(Value.Buffer);
        HashCombine(Hash, UnderlyingTypeValue(Value.Format));
        HashCombine(Hash, Value.FirstElement);
        HashCombine(Hash, Value.NumElements);
        return Hash;
    }

    FRHIBuffer*      Buffer       = nullptr;
    EBufferSRVFormat Format       = EBufferSRVFormat::None;
    uint32           FirstElement = 0;
    uint32           NumElements  = 0;
};

struct FRHITextureUAVInfo
{
    constexpr FRHITextureUAVInfo() noexcept = default;

    constexpr FRHITextureUAVInfo(FRHITexture* InTexture, EFormat InFormat, uint32 InMipLevel, uint32 InFirstArraySlice, uint32 InNumSlices) noexcept
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(uint16(InFirstArraySlice))
        , NumSlices(uint16(InNumSlices))
    {
    }

    constexpr FRHITextureUAVInfo(FRHITexture* InTexture, EFormat InFormat, uint32 InMipLevel) noexcept
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(0)
        , NumSlices(1)
    {
    }

    constexpr bool operator==(const FRHITextureUAVInfo& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHITextureUAVInfo& Value)
    {
        uint64 Hash = BitCast<UPTR_INT>(Value.Texture);
        HashCombine(Hash, UnderlyingTypeValue(Value.Format));
        HashCombine(Hash, Value.MipLevel);
        HashCombine(Hash, Value.FirstArraySlice);
        HashCombine(Hash, Value.NumSlices);
        return Hash;
    }

    FRHITexture* Texture         = nullptr;
    EFormat      Format          = EFormat::Unknown;
    uint8        MipLevel        = 0;
    uint16       FirstArraySlice = 0;
    uint16       NumSlices       = 0;
};

struct FRHIBufferUAVInfo
{
    constexpr FRHIBufferUAVInfo() noexcept = default;

    constexpr FRHIBufferUAVInfo(FRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements, EBufferUAVFormat InFormat = EBufferUAVFormat::None) noexcept
        : Buffer(InBuffer)
        , Format(InFormat)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    {
    }

    constexpr bool operator==(const FRHIBufferUAVInfo& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHIBufferUAVInfo& Value)
    {
        uint64 Hash = BitCast<UPTR_INT>(Value.Buffer);
        HashCombine(Hash, UnderlyingTypeValue(Value.Format));
        HashCombine(Hash, Value.FirstElement);
        HashCombine(Hash, Value.NumElements);
        return Hash;
    }

    FRHIBuffer*      Buffer       = nullptr;
    EBufferUAVFormat Format       = EBufferUAVFormat::None;
    uint32           FirstElement = 0;
    uint32           NumElements  = 0;
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

protected:
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

NODISCARD constexpr EFormat SafeGetFormat(FRHITexture* Texture)
{
    return Texture ? Texture->GetFormat() : EFormat::Unknown;
}

struct FRHIRenderTargetView
{
    FRHIRenderTargetView() noexcept = default;

    FRHIRenderTargetView(
        FRHITexture*           InTexture,
        EAttachmentLoadAction  InLoadAction  = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store,
        const FFloatColor&     InClearValue  = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f)) noexcept
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

    FRHIRenderTargetView(
        FRHITexture*           InTexture,
        EFormat                InFormat,
        uint32                 InArrayIndex,
        uint32                 InMipLevel,
        EAttachmentLoadAction  InLoadAction,
        EAttachmentStoreAction InStoreAction,
        const FFloatColor&     InClearValue) noexcept
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

    FRHITexture*           Texture        = 0;
    FFloatColor            ClearValue     = { };
    uint16                 ArrayIndex     = 0;
    uint16                 NumArraySlices = 0;
    EFormat                Format         = EFormat::Unknown;
    uint8                  MipLevel       = 0;
    EAttachmentLoadAction  LoadAction     = EAttachmentLoadAction::DontCare;
    EAttachmentStoreAction StoreAction    = EAttachmentStoreAction::DontCare;
};

struct FRHIDepthStencilView
{
    FRHIDepthStencilView() noexcept = default;

    explicit FRHIDepthStencilView(
        FRHITexture*              InTexture,
        EAttachmentLoadAction     InLoadAction  = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction    InStoreAction = EAttachmentStoreAction::Store,
        const FDepthStencilValue& InClearValue  = FDepthStencilValue(1.0f, 0)) noexcept
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

    FRHIDepthStencilView(
        FRHITexture*              InTexture,
        uint16                    InArrayIndex,
        uint8                     InMipLevel,
        EAttachmentLoadAction     InLoadAction  = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction    InStoreAction = EAttachmentStoreAction::Store,
        const FDepthStencilValue& InClearValue  = FDepthStencilValue(1.0f, 0)) noexcept
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

    FRHIDepthStencilView(
        FRHITexture*              InTexture,
        uint16                    InArrayIndex,
        uint8                     InMipLevel,
        EFormat                   InFormat,
        EAttachmentLoadAction     InLoadAction  = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction    InStoreAction = EAttachmentStoreAction::Store,
        const FDepthStencilValue& InClearValue  = FDepthStencilValue(1.0f, 0)) noexcept
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

    FRHITexture*           Texture        = nullptr;
    FDepthStencilValue     ClearValue     = { };
    uint16                 ArrayIndex     = 0;
    uint16                 NumArraySlices = 0;
    EFormat                Format         = EFormat::Unknown;
    uint8                  MipLevel       = 0;
    EAttachmentLoadAction  LoadAction     = EAttachmentLoadAction::DontCare;
    EAttachmentStoreAction StoreAction    = EAttachmentStoreAction::DontCare;
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
        , ViewInstancingInfo()
    {
    }

    FRHIBeginRenderPassInfo(const FRenderTargetViews& InRenderTargets, uint32 InNumRenderTargets, FRHIDepthStencilView InDepthStencilView, FRHITexture* InShadingRateTexture = nullptr, EShadingRate InStaticShadingRate = EShadingRate::VRS_1x1) noexcept
        : ShadingRateTexture(InShadingRateTexture)
        , DepthStencilView(InDepthStencilView)
        , RenderTargets(InRenderTargets)
        , NumRenderTargets(InNumRenderTargets)
        , StaticShadingRate(InStaticShadingRate)
        , ViewInstancingInfo()
    {
    }

    bool operator==(const FRHIBeginRenderPassInfo& Other) const noexcept = default;

    FRHITexture*         ShadingRateTexture = nullptr;
    FRHIDepthStencilView DepthStencilView   = { };
    FRenderTargetViews   RenderTargets      = { };
    uint32               NumRenderTargets   = 0;
    EShadingRate         StaticShadingRate  = EShadingRate::VRS_1x1;
    FViewInstancingInfo  ViewInstancingInfo = { };
};
