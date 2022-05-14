#pragma once
#include "RHIResources.h"

#include "Core/Memory/Memory.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIShaderResourceView>  RHIShaderResourceViewRef;
typedef TSharedRef<class CRHIUnorderedAccessView> RHIUnorderedAccessViewRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EBufferSRVFormat

enum class EBufferSRVFormat : uint32
{
    None   = 0,
    Uint32 = 1,
};

inline const char* ToString(EBufferSRVFormat BufferSRVFormat)
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

inline const char* ToString(EBufferUAVFormat BufferSRVFormat)
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

inline const char* ToString(EAttachmentLoadAction LoadAction)
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

inline const char* ToString(EAttachmentStoreAction StoreAction)
{
    switch (StoreAction)
    {
        case EAttachmentStoreAction::DontCare: return "DontCare";
        case EAttachmentStoreAction::Store:    return "Store";
        default:                               return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureSRVInitializer

class CRHITextureSRVInitializer
{
public:

    CRHITextureSRVInitializer()
        : Texture(nullptr)
        , MinLODClamp(0.0f)
        , Format(EFormat::Unknown)
        , FirstMipLevel(0)
        , NumMips(0)
        , FirstArraySlice(0)
        , NumSlices(0)
    { }

    CRHITextureSRVInitializer( CRHITexture* InTexture
                             , float InMinLODClamp
                             , EFormat InFormat
                             , uint8 InFirstMipLevel
                             , uint8 InNumMips
                             , uint16 InFirstArraySlice
                             , uint16 InNumSlices)
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

    bool operator==(const CRHITextureSRVInitializer& RHS) const
    {
        return (Texture         == RHS.Texture)
            && (MinLODClamp     == RHS.MinLODClamp)
            && (Format          == RHS.Format)
            && (FirstMipLevel   == RHS.FirstMipLevel)
            && (NumMips         == RHS.NumMips)
            && (FirstArraySlice == RHS.FirstArraySlice)
            && (NumSlices       == RHS.NumSlices);
    }

    bool operator!=(const CRHITextureSRVInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CRHITexture* Texture;

    float        MinLODClamp;

    EFormat      Format;

    uint8        FirstMipLevel;
    uint8        NumMips;

    uint16       FirstArraySlice;
    uint16       NumSlices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBufferSRVInitializer

class CRHIBufferSRVInitializer
{
public:

    CRHIBufferSRVInitializer()
        : Buffer(nullptr)
	    , Format(EBufferSRVFormat::None)
	    , FirstElement(0)
	    , NumElements(0)
    { }

    CRHIBufferSRVInitializer( CRHIBuffer* InBuffer
                            , uint32 InFirstElement
                            , uint32 InNumElements
                            , EBufferSRVFormat InFormat = EBufferSRVFormat::None)
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

    bool operator==(const CRHIBufferSRVInitializer& RHS) const
    {
        return (Buffer       == RHS.Buffer) 
            && (Format       == RHS.Format)
            && (FirstElement == RHS.FirstElement) 
            && (NumElements  == RHS.NumElements);
    }

    bool operator!=(const CRHIBufferSRVInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIBuffer*      Buffer;

    EBufferSRVFormat Format;

    uint32           FirstElement;
    uint32           NumElements;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureUAVInitializer

class CRHITextureUAVInitializer
{
public:

    CRHITextureUAVInitializer()
        : Texture(nullptr)
        , Format(EFormat::Unknown)
        , MipLevel(0)
        , FirstArraySlice(0)
        , NumSlices(0)
    { }

    CRHITextureUAVInitializer( CRHITexture* InTexture
                             , EFormat InFormat
                             , uint32 InMipLevel
                             , uint32 InFirstArraySlice
                             , uint32 InNumSlices)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(uint8(InMipLevel))
        , FirstArraySlice(uint16(InFirstArraySlice))
        , NumSlices(uint16(InNumSlices))
    { }

    CRHITextureUAVInitializer(CRHITexture* InTexture, EFormat InFormat, uint32 InMipLevel)
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

    bool operator==(const CRHITextureUAVInitializer& RHS) const
    {
        return (Texture         == RHS.Texture)
            && (Format          == RHS.Format)
            && (MipLevel        == RHS.MipLevel)
            && (FirstArraySlice == RHS.FirstArraySlice)
            && (NumSlices       == RHS.NumSlices);
    }

    bool operator!=(const CRHITextureUAVInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CRHITexture* Texture;

    EFormat      Format;

    uint8        MipLevel;

    uint16       FirstArraySlice;
    uint16       NumSlices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBufferUAVInitializer

class CRHIBufferUAVInitializer
{
public:

    CRHIBufferUAVInitializer()
        : Buffer(nullptr)
		, Format(EBufferUAVFormat::None)
		, FirstElement(0)
        , NumElements(0)
    { }

    CRHIBufferUAVInitializer( CRHIBuffer* InBuffer
                            , uint32 InFirstElement
                            , uint32 InNumElements
                            , EBufferUAVFormat InFormat = EBufferUAVFormat::None)
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

    bool operator==(const CRHIBufferUAVInitializer& RHS) const
    {
        return (Buffer       == RHS.Buffer) 
            && (Format       == RHS.Format)
            && (FirstElement == RHS.FirstElement) 
            && (NumElements  == RHS.NumElements);
    }

    bool operator!=(const CRHIBufferUAVInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIBuffer*      Buffer;

    EBufferUAVFormat Format;

    uint32           FirstElement;
    uint32           NumElements;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIResourceView

class CRHIResourceView : public CRHIResource
{
protected:

    explicit CRHIResourceView(CRHIResource* InResource)
        : CRHIResource()
        , Resource(InResource)
    { }

    ~CRHIResourceView() = default;

public:

    CRHIResource* GetResource() const { return Resource; }

protected:
    CRHIResource* Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderResourceView

class CRHIShaderResourceView : public CRHIResourceView
{
protected:

    explicit CRHIShaderResourceView(CRHIResource* InResource)
        : CRHIResourceView(InResource)
    { }

    ~CRHIShaderResourceView() = default;

public:

    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIUnorderedAccessView

class CRHIUnorderedAccessView : public CRHIResourceView
{
protected:

    explicit CRHIUnorderedAccessView(CRHIResource* InResource)
        : CRHIResourceView(InResource)
    { }

    ~CRHIUnorderedAccessView() = default;

public:

    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRenderTargetView

class CRHIRenderTargetView
{
public:

    CRHIRenderTargetView()
        : Texture(nullptr)
        , Format(EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(EAttachmentLoadAction::DontCare)
        , StoreAction(EAttachmentStoreAction::DontCare)
        , ClearValue()
    { }
    
    explicit CRHIRenderTargetView( CRHITexture* InTexture
                                 , EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear
                                 , EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store
                                 , const CFloatColor& InClearValue = CFloatColor(0.0f, 0.0f, 0.0f, 1.0f))
        : Texture(InTexture)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    explicit CRHIRenderTargetView( CRHITexture* InTexture
                                 , EFormat InFormat
                                 , uint32 InArrayIndex
                                 , uint32 InMipLevel
                                 , EAttachmentLoadAction InLoadAction
                                 , EAttachmentStoreAction InStoreAction
                                 , const CFloatColor& InClearValue)
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

    bool operator==(const CRHIRenderTargetView& RHS) const
    {
        return (Texture     == RHS.Texture)
            && (Format      == RHS.Format)
            && (ArrayIndex  == RHS.ArrayIndex)
            && (MipLevel    == RHS.MipLevel)
            && (LoadAction  == RHS.LoadAction)
            && (StoreAction == RHS.StoreAction)
            && (ClearValue  == RHS.ClearValue);
    }

    bool operator!=(const CRHIRenderTargetView& RHS) const
    {
        return !(*this == RHS);
    }

    CRHITexture*           Texture;

    EFormat                Format;

    uint16                 ArrayIndex;
    uint8                  MipLevel;

    EAttachmentLoadAction  LoadAction;
    EAttachmentStoreAction StoreAction;

    CFloatColor            ClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDepthStencilView

class CRHIDepthStencilView
{
public:

    CRHIDepthStencilView()
        : Texture(nullptr)
        , Format(EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(EAttachmentLoadAction::DontCare)
        , StoreAction(EAttachmentStoreAction::DontCare)
        , ClearValue()
    { }

    explicit CRHIDepthStencilView( CRHITexture* InTexture
                                 , EAttachmentLoadAction InLoadAction  = EAttachmentLoadAction::Clear
                                 , EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store
                                 , const CTextureDepthStencilValue& InClearValue = CTextureDepthStencilValue(1.0f, 0))
        : Texture(InTexture)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
        , ArrayIndex(0)
        , MipLevel(0)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    explicit CRHIDepthStencilView( CRHITexture* InTexture
                                 , uint16 InArrayIndex
                                 , uint8 InMipLevel
                                 , EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear
                                 , EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store
                                 , const CTextureDepthStencilValue& InClearValue = CTextureDepthStencilValue(1.0f, 0))
        : Texture(InTexture)
        , Format(InTexture ? InTexture->GetFormat() : EFormat::Unknown)
        , ArrayIndex(uint16(InArrayIndex))
        , MipLevel(uint8(InMipLevel))
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    explicit CRHIDepthStencilView( CRHITexture* InTexture
                                 , uint16 InArrayIndex
                                 , uint8 InMipLevel
                                 , EFormat InFormat
                                 , EAttachmentLoadAction InLoadAction = EAttachmentLoadAction::Clear
                                 , EAttachmentStoreAction InStoreAction = EAttachmentStoreAction::Store
                                 , const CTextureDepthStencilValue& InClearValue = CTextureDepthStencilValue(1.0f, 0))
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

    bool operator==(const CRHIDepthStencilView& RHS) const
    {
        return (Texture     == RHS.Texture)
            && (Format      == RHS.Format)
            && (ArrayIndex  == RHS.ArrayIndex)
            && (MipLevel    == RHS.MipLevel)
            && (LoadAction  == RHS.LoadAction)
            && (StoreAction == RHS.StoreAction)
            && (ClearValue  == RHS.ClearValue);
    }

    bool operator!=(const CRHIDepthStencilView& RHS) const
    {
        return !(*this == RHS);
    }

    CRHITexture*              Texture;

    EFormat                   Format;

    uint16                    ArrayIndex;
    uint8                     MipLevel;

    EAttachmentLoadAction     LoadAction;
    EAttachmentStoreAction    StoreAction;

    CTextureDepthStencilValue ClearValue;
};
