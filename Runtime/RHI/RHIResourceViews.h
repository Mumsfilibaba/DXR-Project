#pragma once
#include "RHIResources.h"

#include "Core/Memory/Memory.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIShaderResourceView>  RHIShaderResourceViewRef;
typedef TSharedRef<class CRHIUnorderedAccessView> RHIUnorderedAccessViewRef;
typedef TSharedRef<class CRHIRenderTargetView>    RHIRenderTargetViewRef;
typedef TSharedRef<class CRHIDepthStencilView>    RHIDepthStencilViewRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIUnorderedAccessViewInfo

struct SRHIUnorderedAccessViewInfo
{
    enum class EType
    {
        Texture2D        = 1,
        Texture2DArray   = 2,
        TextureCube      = 3,
        TextureCubeArray = 4,
        Texture3D        = 5,
        VertexBuffer     = 6,
        IndexBuffer      = 7,
        GenericBuffer    = 8,
    };

    FORCEINLINE SRHIUnorderedAccessViewInfo(EType InType)
        : Type(InType)
    { }

    EType Type;
    union
    {
        struct
        {
            CRHITexture2D* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
        } Texture2D;
        struct
        {
            CRHITexture2DArray* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
        } Texture2DArray;
        struct
        {
            CRHITextureCube* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
        } TextureCube;
        struct
        {
            CRHITextureCubeArray* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
        } TextureCubeArray;
        struct
        {
            CRHITexture3D* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  DepthSlice = 0;
            uint32  NumDepthSlices = 0;
        } Texture3D;
        struct
        {
            CRHIVertexBuffer* Buffer = nullptr;
            uint32 FirstVertex = 0;
            uint32 NumVertices = 0;
        } VertexBuffer;
        struct
        {
            CRHIIndexBuffer* Buffer = nullptr;
            uint32 FirstIndex = 0;
            uint32 NumIndices = 0;
        } IndexBuffer;
        struct
        {
            CRHIGenericBuffer* Buffer = nullptr;
            uint32 FirstElement = 0;
            uint32 NumElements = 0;
        } StructuredBuffer;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIRenderTargetViewInfo

struct SRHIRenderTargetViewInfo
{
    // TODO: Add support for texel buffers?
    enum class EType
    {
        Texture2D        = 1,
        Texture2DArray   = 2,
        TextureCube      = 3,
        TextureCubeArray = 4,
        Texture3D        = 5,
    };

    FORCEINLINE SRHIRenderTargetViewInfo(EType InType)
        : Type(InType)
    { }

    EType   Type;
    EFormat Format = EFormat::Unknown;
    union
    {
        struct
        {
            CRHITexture2D* Texture = nullptr;
            uint32 Mip = 0;
        } Texture2D;
        struct
        {
            CRHITexture2DArray* Texture = nullptr;
            uint32 Mip = 0;
            uint32 ArraySlice = 0;
            uint32 NumArraySlices = 0;
        } Texture2DArray;
        struct
        {
            CRHITextureCube* Texture = nullptr;
            ECubeFace CubeFace = ECubeFace::PosX;
            uint32    Mip = 0;
        } TextureCube;
        struct
        {
            CRHITextureCubeArray* Texture = nullptr;
            ECubeFace CubeFace = ECubeFace::PosX;
            uint32    Mip = 0;
            uint32    ArraySlice = 0;
        } TextureCubeArray;
        struct
        {
            CRHITexture3D* Texture = nullptr;
            uint32 Mip = 0;
            uint32 DepthSlice = 0;
            uint32 NumDepthSlices = 0;
        } Texture3D;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIDepthStencilViewInfo

struct SRHIDepthStencilViewInfo
{
    enum class EType
    {
        Texture2D        = 1,
        Texture2DArray   = 2,
        TextureCube      = 3,
        TextureCubeArray = 4,
    };

    FORCEINLINE SRHIDepthStencilViewInfo(EType InType)
        : Type(InType)
    { }

    EType   Type;
    EFormat Format = EFormat::Unknown;
    union
    {
        struct
        {
            CRHITexture2D* Texture = nullptr;
            uint32 Mip = 0;
        } Texture2D;
        struct
        {
            CRHITexture2DArray* Texture = nullptr;
            uint32 Mip = 0;
            uint32 ArraySlice = 0;
            uint32 NumArraySlices = 0;
        } Texture2DArray;
        struct
        {
            CRHITextureCube* Texture = nullptr;
            ECubeFace CubeFace = ECubeFace::PosX;
            uint32    Mip = 0;
        } TextureCube;

        struct
        {
            CRHITextureCubeArray* Texture = nullptr;
            ECubeFace CubeFace = ECubeFace::PosX;
            uint32    Mip = 0;
            uint32    ArraySlice = 0;
        } TextureCubeArray;
    };
};

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
        , FirstElement(0)
        , NumElements(0)
        , Format(EBufferSRVFormat::None)
    { }

    CRHIBufferSRVInitializer( CRHIBuffer* InBuffer
                            , uint32 InFirstElement
                            , uint32 InNumElements
                            , EBufferSRVFormat InFormat = EBufferSRVFormat::None)
        : Buffer(InBuffer)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
        , Format(InFormat)
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
        , FirstElement(0)
        , NumElements(0)
        , Format(EBufferUAVFormat::None)
    { }

    CRHIBufferUAVInitializer( CRHIBuffer* InBuffer
                            , uint32 InFirstElement
                            , uint32 InNumElements
                            , EBufferUAVFormat InFormat = EBufferUAVFormat::None)
        : Buffer(InBuffer)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
        , Format(InFormat)
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

    explicit CRHIUnorderedAccessView()
        : CRHIResourceView(nullptr)
    { }

    ~CRHIUnorderedAccessView() = default;

public:

    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRenderTargetView

class CRHIRenderTargetView : public CRHIResourceView
{
protected:

    explicit CRHIRenderTargetView()
        : CRHIResourceView(nullptr)
    { }

    ~CRHIRenderTargetView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDepthStencilView

using DepthStencilViewCube = TStaticArray<TSharedRef<CRHIDepthStencilView>, 6>;

class CRHIDepthStencilView : public CRHIResourceView
{
protected:

    explicit CRHIDepthStencilView()
        : CRHIResourceView(nullptr)
    { }

    ~CRHIDepthStencilView() = default;
};