#pragma once
#include "RHITexture.h"
#include "RHIBuffer.h"

#include "Core/Memory/Memory.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"

class FRHITexture;
class FRHIBuffer;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class FRHIShaderResourceView>  FRHIShaderResourceViewRef;
typedef TSharedRef<class FRHIUnorderedAccessView> FRHIUnorderedAccessViewRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EBufferSRVFormat

enum class EBufferSRVFormat : uint32
{
    None   = 0,
    Uint32 = 1,
};

inline const CHAR* ToString(EBufferSRVFormat BufferSRVFormat)
{
    switch (BufferSRVFormat)
    {
        case EBufferSRVFormat::Uint32: return "Uint32";
        default:                       return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EBufferUAVFormat

enum class EBufferUAVFormat : uint32
{
    None   = 0,
    Uint32 = 1,
};

inline const CHAR* ToString(EBufferUAVFormat BufferSRVFormat)
{
    switch (BufferSRVFormat)
    {
        case EBufferUAVFormat::Uint32: return "Uint32";
        default:                       return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EAttachmentLoadAction

enum class EAttachmentLoadAction : uint8
{
    DontCare = 0, // Don't care
    Load     = 1, // Use the stored data when RenderPass begin
    Clear    = 2, // Clear data when RenderPass begin
};

inline const CHAR* ToString(EAttachmentLoadAction LoadAction)
{
    switch (LoadAction)
    {
        case EAttachmentLoadAction::DontCare: return "DontCare";
        case EAttachmentLoadAction::Load:     return "Load";
        case EAttachmentLoadAction::Clear:    return "Clear";
        default:                              return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EAttachmentStoreAction

enum class EAttachmentStoreAction : uint8
{
    DontCare = 0, // Don't care
    Store    = 1, // Store the data after the RenderPass is finished
};

inline const CHAR* ToString(EAttachmentStoreAction StoreAction)
{
    switch (StoreAction)
    {
        case EAttachmentStoreAction::DontCare: return "DontCare";
        case EAttachmentStoreAction::Store:    return "Store";
        default:                               return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITextureSRVInitializer

struct FRHITextureSRVInitializer
{
    FRHITextureSRVInitializer()
        : Texture(nullptr)
        , MinLODClamp(0.0f)
        , Format(EFormat::Unknown)
        , FirstMipLevel(0)
        , NumMips(0)
        , FirstArraySlice(0)
        , NumSlices(0)
    { }

    FRHITextureSRVInitializer(
        FRHITexture* InTexture,
        float InMinLODClamp,
        EFormat InFormat,
        uint8 InFirstMipLevel,
        uint8 InNumMips,
        uint16 InFirstArraySlice,
        uint16 InNumSlices)
        : Texture(InTexture)
        , MinLODClamp(InMinLODClamp)
        , Format(InFormat)
        , FirstMipLevel(InFirstMipLevel)
        , NumMips(InNumMips)
        , FirstArraySlice(InFirstArraySlice)
        , NumSlices(InNumSlices)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, MinLODClamp);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, FirstMipLevel);
        HashCombine(Hash, NumMips);
        HashCombine(Hash, FirstArraySlice);
        HashCombine(Hash, NumSlices);
        return Hash;
    }

    bool operator==(const FRHITextureSRVInitializer& RHS) const
    {
        return (Texture         == RHS.Texture)
            && (MinLODClamp     == RHS.MinLODClamp)
            && (Format          == RHS.Format)
            && (FirstMipLevel   == RHS.FirstMipLevel)
            && (NumMips         == RHS.NumMips)
            && (FirstArraySlice == RHS.FirstArraySlice)
            && (NumSlices       == RHS.NumSlices);
    }

    bool operator!=(const FRHITextureSRVInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FRHITexture* Texture;

    float        MinLODClamp;

    EFormat      Format;

    uint8        FirstMipLevel;
    uint8        NumMips;

    uint16       FirstArraySlice;
    uint16       NumSlices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIBufferSRVInitializer

struct FRHIBufferSRVInitializer
{
    FRHIBufferSRVInitializer()
        : Buffer(nullptr)
        , Format(EBufferSRVFormat::None)
        , FirstElement(0)
        , NumElements(0)
    { }

    FRHIBufferSRVInitializer(
        FRHIBuffer* InBuffer,
        uint32 InFirstElement,
        uint32 InNumElements,
        EBufferSRVFormat InFormat = EBufferSRVFormat::None)
        : Buffer(InBuffer)
        , Format(InFormat)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Buffer);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, FirstElement);
        HashCombine(Hash, NumElements);
        return Hash;
    }

    bool operator==(const FRHIBufferSRVInitializer& RHS) const
    {
        return (Buffer       == RHS.Buffer) 
            && (Format       == RHS.Format)
            && (FirstElement == RHS.FirstElement) 
            && (NumElements  == RHS.NumElements);
    }

    bool operator!=(const FRHIBufferSRVInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIBuffer*      Buffer;

    EBufferSRVFormat Format;

    uint32           FirstElement;
    uint32           NumElements;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHITextureUAVInitializer

struct FRHITextureUAVInitializer
{
    FRHITextureUAVInitializer()
        : Texture(nullptr)
        , Format(EFormat::Unknown)
        , MipLevel(0)
        , FirstArraySlice(0)
        , NumSlices(0)
    { }

    FRHITextureUAVInitializer(
        FRHITexture* InTexture,
        EFormat InFormat,
        uint32 InMipLevel,
        uint32 InFirstArraySlice,
        uint32 InNumSlices)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(uint16(InFirstArraySlice))
        , NumSlices(uint16(InNumSlices))
    { }

    FRHITextureUAVInitializer(FRHITexture* InTexture, EFormat InFormat, uint32 InMipLevel)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(0)
        , NumSlices(1)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, MipLevel);
        HashCombine(Hash, FirstArraySlice);
        HashCombine(Hash, NumSlices);
        return Hash;
    }

    bool operator==(const FRHITextureUAVInitializer& RHS) const
    {
        return (Texture         == RHS.Texture)
            && (Format          == RHS.Format)
            && (MipLevel        == RHS.MipLevel)
            && (FirstArraySlice == RHS.FirstArraySlice)
            && (NumSlices       == RHS.NumSlices);
    }

    bool operator!=(const FRHITextureUAVInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FRHITexture* Texture;

    EFormat      Format;

    uint8        MipLevel;

    uint16       FirstArraySlice;
    uint16       NumSlices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIBufferUAVInitializer

struct FRHIBufferUAVInitializer
{
    FRHIBufferUAVInitializer()
        : Buffer(nullptr)
        , Format(EBufferUAVFormat::None)
        , FirstElement(0)
        , NumElements(0)
    { }

    FRHIBufferUAVInitializer(
        FRHIBuffer* InBuffer,
        uint32 InFirstElement,
        uint32 InNumElements,
        EBufferUAVFormat InFormat = EBufferUAVFormat::None)
        : Buffer(InBuffer)
        , Format(InFormat)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Buffer);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, FirstElement);
        HashCombine(Hash, NumElements);
        return Hash;
    }

    bool operator==(const FRHIBufferUAVInitializer& RHS) const
    {
        return (Buffer       == RHS.Buffer) 
            && (Format       == RHS.Format)
            && (FirstElement == RHS.FirstElement) 
            && (NumElements  == RHS.NumElements);
    }

    bool operator!=(const FRHIBufferUAVInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIBuffer*      Buffer;

    EBufferUAVFormat Format;

    uint32           FirstElement;
    uint32           NumElements;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIResourceView

class FRHIResourceView 
    : public FRHIResource
{
protected:
    explicit FRHIResourceView(FRHIResource* InResource)
        : FRHIResource()
        , Resource(InResource)
    { }

    ~FRHIResourceView() = default;

public:
    FRHIResource* GetResource() const { return Resource; }

protected:
    FRHIResource* Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIShaderResourceView

class FRHIShaderResourceView 
    : public FRHIResourceView
{
protected:
    explicit FRHIShaderResourceView(FRHIResource* InResource)
        : FRHIResourceView(InResource)
    { }

    ~FRHIShaderResourceView() = default;

public:

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIUnorderedAccessView

class FRHIUnorderedAccessView 
    : public FRHIResourceView
{
protected:
    explicit FRHIUnorderedAccessView(FRHIResource* InResource)
        : FRHIResourceView(InResource)
    { }

    ~FRHIUnorderedAccessView() = default;

public:
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRenderTargetView

struct FRHIRenderTargetView
{
    FRHIRenderTargetView()
        : Texture(nullptr)
        , Format(EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(EAttachmentLoadAction::DontCare)
        , StoreAction(EAttachmentStoreAction::DontCare)
        , ClearValue()
    { }
    
    explicit FRHIRenderTargetView(
        FRHITexture* InTexture,
        EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store,
        const FFloatColor& InClearValue = FFloatColor(0.0f, 0.0f, 0.0f, 1.0f))
        : Texture(InTexture)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    explicit FRHIRenderTargetView(
        FRHITexture* InTexture,
        EFormat InFormat,
        uint32 InArrayIndex,
        uint32 InMipLevel,
        EAttachmentLoadAction InLoadAction,
        EAttachmentStoreAction InStoreAction,
        const FFloatColor& InClearValue)
        : Texture(InTexture)
        , Format(InFormat)
        , ArrayIndex(uint16(InArrayIndex))
        , MipLevel(uint8(InMipLevel))
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, ArrayIndex);
        HashCombine(Hash, MipLevel);
        HashCombine(Hash, ToUnderlying(LoadAction));
        HashCombine(Hash, ToUnderlying(StoreAction));
        HashCombine(Hash, ClearValue.GetHash());
        return Hash;
    }

    bool operator==(const FRHIRenderTargetView& RHS) const
    {
        return (Texture     == RHS.Texture)
            && (Format      == RHS.Format)
            && (ArrayIndex  == RHS.ArrayIndex)
            && (MipLevel    == RHS.MipLevel)
            && (LoadAction  == RHS.LoadAction)
            && (StoreAction == RHS.StoreAction)
            && (ClearValue  == RHS.ClearValue);
    }

    bool operator!=(const FRHIRenderTargetView& RHS) const
    {
        return !(*this == RHS);
    }

    FRHITexture*           Texture;

    EFormat                Format;

    uint16                 ArrayIndex;
    uint8                  MipLevel;

    EAttachmentLoadAction  LoadAction;
    EAttachmentStoreAction StoreAction;

    FFloatColor            ClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIDepthStencilView

struct FRHIDepthStencilView
{
    FRHIDepthStencilView()
        : Texture(nullptr)
        , Format(EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(EAttachmentLoadAction::DontCare)
        , StoreAction(EAttachmentStoreAction::DontCare)
        , ClearValue()
    { }

    explicit FRHIDepthStencilView(
        FRHITexture* InTexture,
        EAttachmentLoadAction InLoadAction  = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store,
        const FTextureDepthStencilValue& InClearValue = FTextureDepthStencilValue(1.0f, 0))
        : Texture(InTexture)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    explicit FRHIDepthStencilView(
        FRHITexture* InTexture,
        uint16 InArrayIndex,
        uint8 InMipLevel,
        EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear,
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store,
        const FTextureDepthStencilValue& InClearValue = FTextureDepthStencilValue(1.0f, 0))
        : Texture(InTexture)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
        , ArrayIndex(uint16(InArrayIndex))
        , MipLevel(uint8(InMipLevel))
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    explicit FRHIDepthStencilView(
        FRHITexture* InTexture,
        uint16 InArrayIndex,
        uint8 InMipLevel,
        EFormat InFormat,
        EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear, 
        EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store,
        const FTextureDepthStencilValue& InClearValue = FTextureDepthStencilValue(1.0f, 0))
        : Texture(InTexture)
        , Format(InFormat)
        , ArrayIndex(uint16(InArrayIndex))
        , MipLevel(uint8(InMipLevel))
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, ArrayIndex);
        HashCombine(Hash, MipLevel);
        HashCombine(Hash, ToUnderlying(LoadAction));
        HashCombine(Hash, ToUnderlying(StoreAction));
        HashCombine(Hash, ClearValue.GetHash());
        return Hash;
    }

    bool operator==(const FRHIDepthStencilView& RHS) const
    {
        return (Texture     == RHS.Texture)
            && (Format      == RHS.Format)
            && (ArrayIndex  == RHS.ArrayIndex)
            && (MipLevel    == RHS.MipLevel)
            && (LoadAction  == RHS.LoadAction)
            && (StoreAction == RHS.StoreAction)
            && (ClearValue  == RHS.ClearValue);
    }

    bool operator!=(const FRHIDepthStencilView& RHS) const
    {
        return !(*this == RHS);
    }

    FRHITexture*              Texture;

    EFormat                   Format;

    uint16                    ArrayIndex;
    uint8                     MipLevel;

    EAttachmentLoadAction     LoadAction;
    EAttachmentStoreAction    StoreAction;

    FTextureDepthStencilValue ClearValue;
};
