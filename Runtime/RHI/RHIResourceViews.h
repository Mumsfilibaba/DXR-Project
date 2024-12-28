#pragma once
#include "RHIResource.h"
#include "RHIPipelineState.h"
#include "Core/Containers/StaticArray.h"

enum class EBufferSRVFormat : uint32
{
    None   = 0,
    UInt32 = 1,
};

constexpr const CHAR* ToString(EBufferSRVFormat BufferSRVFormat)
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

constexpr const CHAR* ToString(EBufferUAVFormat BufferSRVFormat)
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

constexpr const CHAR* ToString(EAttachmentLoadAction LoadAction)
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

constexpr const CHAR* ToString(EAttachmentStoreAction StoreAction)
{
    switch (StoreAction)
    {
        case EAttachmentStoreAction::DontCare: return "DontCare";
        case EAttachmentStoreAction::Store:    return "Store";
        default:                               return "Unknown";
    }
}

struct FRHITextureSRVDesc
{
    FRHITextureSRVDesc()
        : Texture(nullptr)
        , MinLODClamp(0.0f)
        , Format(EFormat::Unknown)
        , FirstMipLevel(0)
        , NumMips(0)
        , FirstArraySlice(0)
        , NumSlices(0)
    {
    }

    FRHITextureSRVDesc(
        FRHITexture* InTexture,
        float        InMinLODClamp,
        EFormat      InFormat,
        uint8        InFirstMipLevel,
        uint8        InNumMips,
        uint16       InFirstArraySlice,
        uint16       InNumSlices)
        : Texture(InTexture)
        , MinLODClamp(InMinLODClamp)
        , Format(InFormat)
        , FirstMipLevel(InFirstMipLevel)
        , NumMips(InNumMips)
        , FirstArraySlice(InFirstArraySlice)
        , NumSlices(InNumSlices)
    {
    }

    bool operator==(const FRHITextureSRVDesc& Other) const
    {
        return Texture         == Other.Texture
            && MinLODClamp     == Other.MinLODClamp
            && Format          == Other.Format
            && FirstMipLevel   == Other.FirstMipLevel
            && NumMips         == Other.NumMips
            && FirstArraySlice == Other.FirstArraySlice
            && NumSlices       == Other.NumSlices;
    }

    bool operator!=(const FRHITextureSRVDesc& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FRHITextureSRVDesc& Value)
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

struct FRHIBufferSRVDesc
{
    FRHIBufferSRVDesc()
        : Buffer(nullptr)
        , Format(EBufferSRVFormat::None)
        , FirstElement(0)
        , NumElements(0)
    {
    }

    FRHIBufferSRVDesc(FRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements, EBufferSRVFormat InFormat = EBufferSRVFormat::None)
        : Buffer(InBuffer)
        , Format(InFormat)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    {
    }

    bool operator==(const FRHIBufferSRVDesc& Other) const
    {
        return Buffer == Other.Buffer && Format == Other.Format && FirstElement == Other.FirstElement && NumElements == Other.NumElements;
    }

    bool operator!=(const FRHIBufferSRVDesc& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FRHIBufferSRVDesc& Value)
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

struct FRHITextureUAVDesc
{
    FRHITextureUAVDesc()
        : Texture(nullptr)
        , Format(EFormat::Unknown)
        , MipLevel(0)
        , FirstArraySlice(0)
        , NumSlices(0)
    {
    }

    FRHITextureUAVDesc(FRHITexture* InTexture, EFormat InFormat, uint32 InMipLevel, uint32 InFirstArraySlice, uint32 InNumSlices)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(uint16(InFirstArraySlice))
        , NumSlices(uint16(InNumSlices))
    {
    }

    FRHITextureUAVDesc(FRHITexture* InTexture, EFormat InFormat, uint32 InMipLevel)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(0)
        , NumSlices(1)
    {
    }

    bool operator==(const FRHITextureUAVDesc& Other) const
    {
        return Texture == Other.Texture && Format == Other.Format && MipLevel == Other.MipLevel && FirstArraySlice == Other.FirstArraySlice && NumSlices == Other.NumSlices;
    }

    bool operator!=(const FRHITextureUAVDesc& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FRHITextureUAVDesc& Value)
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

struct FRHIBufferUAVDesc
{
    FRHIBufferUAVDesc()
        : Buffer(nullptr)
        , Format(EBufferUAVFormat::None)
        , FirstElement(0)
        , NumElements(0)
    {
    }

    FRHIBufferUAVDesc(FRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements, EBufferUAVFormat InFormat = EBufferUAVFormat::None)
        : Buffer(InBuffer)
        , Format(InFormat)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    {
    }

    bool operator==(const FRHIBufferUAVDesc& Other) const
    {
        return Buffer == Other.Buffer && Format == Other.Format && FirstElement == Other.FirstElement && NumElements  == Other.NumElements;
    }

    bool operator!=(const FRHIBufferUAVDesc& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FRHIBufferUAVDesc& Value)
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
    FORCEINLINE FRHIResource* GetResource() const
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

struct FRHIRenderTargetView
{
    FRHIRenderTargetView()
        : Texture(nullptr)
        , ClearValue()
        , ArrayIndex(0)
        , NumArraySlices(1)
        , Format(EFormat::Unknown)
        , MipLevel(0)
        , LoadAction(EAttachmentLoadAction::DontCare)
        , StoreAction(EAttachmentStoreAction::DontCare)
    {
    }

    FRHIRenderTargetView(
        FRHITexture*           InTexture,
        EAttachmentLoadAction  InLoadAction  = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store,
        const FFloatColor&     InClearValue  = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f))
        : Texture(InTexture)
        , ClearValue(InClearValue)
        , ArrayIndex(0)
        , NumArraySlices(1)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
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
        const FFloatColor&     InClearValue)
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

    bool operator==(const FRHIRenderTargetView& Other) const
    {
        return Texture        == Other.Texture
            && Format         == Other.Format
            && ArrayIndex     == Other.ArrayIndex
            && NumArraySlices == Other.NumArraySlices
            && MipLevel       == Other.MipLevel
            && LoadAction     == Other.LoadAction
            && StoreAction    == Other.StoreAction
            && ClearValue     == Other.ClearValue;
    }

    bool operator!=(const FRHIRenderTargetView& Other) const
    {
        return !(*this == Other);
    }

    FRHITexture*           Texture;
    FFloatColor            ClearValue;
    uint16                 ArrayIndex;
    uint16                 NumArraySlices;
    EFormat                Format;
    uint8                  MipLevel;
    EAttachmentLoadAction  LoadAction;
    EAttachmentStoreAction StoreAction;
};

struct FRHIDepthStencilView
{
    FRHIDepthStencilView()
        : Texture(nullptr)
        , ClearValue()
        , ArrayIndex(0)
        , NumArraySlices(1)
        , Format(EFormat::Unknown)
        , MipLevel(0)
        , LoadAction(EAttachmentLoadAction::DontCare)
        , StoreAction(EAttachmentStoreAction::DontCare)
    {
    }

    explicit FRHIDepthStencilView(
        FRHITexture*              InTexture,
        EAttachmentLoadAction     InLoadAction  = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction    InStoreAction = EAttachmentStoreAction::Store,
        const FDepthStencilValue& InClearValue  = FDepthStencilValue(1.0f, 0))
        : Texture(InTexture)
        , ClearValue(InClearValue)
        , ArrayIndex(0)
        , NumArraySlices(1)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
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
        const FDepthStencilValue& InClearValue  = FDepthStencilValue(1.0f, 0))
        : Texture(InTexture)
        , ClearValue(InClearValue)
        , ArrayIndex(uint16(InArrayIndex))
        , NumArraySlices(1)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
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
        const FDepthStencilValue& InClearValue  = FDepthStencilValue(1.0f, 0))
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

    bool operator==(const FRHIDepthStencilView& Other) const
    {
        return Texture        == Other.Texture
            && Format         == Other.Format
            && ArrayIndex     == Other.ArrayIndex
            && NumArraySlices == Other.NumArraySlices
            && MipLevel       == Other.MipLevel
            && LoadAction     == Other.LoadAction
            && StoreAction    == Other.StoreAction
            && ClearValue     == Other.ClearValue;
    }

    bool operator!=(const FRHIDepthStencilView& Other) const
    {
        return !(*this == Other);
    }

    FRHITexture*           Texture;
    FDepthStencilValue     ClearValue;
    uint16                 ArrayIndex;
    uint16                 NumArraySlices;
    EFormat                Format;
    uint8                  MipLevel;
    EAttachmentLoadAction  LoadAction;
    EAttachmentStoreAction StoreAction;
};

struct FRHIBeginRenderPassInfo
{
    typedef TStaticArray<FRHIRenderTargetView, RHI_MAX_RENDER_TARGETS> FRenderTargetViews;
   
    FRHIBeginRenderPassInfo()
        : ShadingRateTexture(nullptr)
        , DepthStencilView()
        , RenderTargets()
        , NumRenderTargets(0)
        , StaticShadingRate(EShadingRate::VRS_1x1)
        , ViewInstancingInfo()
    {
    }

    FRHIBeginRenderPassInfo(const FRenderTargetViews& InRenderTargets, uint32 InNumRenderTargets)
        : ShadingRateTexture(nullptr)
        , DepthStencilView()
        , RenderTargets(InRenderTargets)
        , NumRenderTargets(InNumRenderTargets)
        , StaticShadingRate(EShadingRate::VRS_1x1)
        , ViewInstancingInfo()
    {
    }

    FRHIBeginRenderPassInfo(const FRenderTargetViews& InRenderTargets, uint32 InNumRenderTargets, FRHIDepthStencilView InDepthStencilView, FRHITexture* InShadingRateTexture = nullptr, EShadingRate InStaticShadingRate = EShadingRate::VRS_1x1)
        : ShadingRateTexture(InShadingRateTexture)
        , DepthStencilView(InDepthStencilView)
        , RenderTargets(InRenderTargets)
        , NumRenderTargets(InNumRenderTargets)
        , StaticShadingRate(InStaticShadingRate)
        , ViewInstancingInfo()
    {
    }

    bool operator==(const FRHIBeginRenderPassInfo& Other) const
    {
        return ShadingRateTexture == Other.ShadingRateTexture
            && DepthStencilView   == Other.DepthStencilView
            && RenderTargets      == Other.RenderTargets
            && NumRenderTargets   == Other.NumRenderTargets
            && StaticShadingRate  == Other.StaticShadingRate
            && ViewInstancingInfo == Other.ViewInstancingInfo;
    }

    bool operator!=(const FRHIBeginRenderPassInfo& Other) const
    {
        return !(*this == Other);
    }

    FRHITexture*         ShadingRateTexture;
    FRHIDepthStencilView DepthStencilView;
    FRenderTargetViews   RenderTargets;
    uint32               NumRenderTargets;
    EShadingRate         StaticShadingRate;
    FViewInstancingInfo  ViewInstancingInfo;
};
