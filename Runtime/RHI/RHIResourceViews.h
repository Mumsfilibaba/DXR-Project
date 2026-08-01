#pragma once
#include "Core/Containers/StaticArray.h"
#include "RHI/RHIResource.h"
#include "RHI/RHIPipelineState.h"
#include "RHI/RHITexture.h"

enum class EBufferViewType : uint8
{
    Unknown = 0,  // Unknown
    Structured,   // StructuredBuffer<T>, RWStructuredBuffer<T>
    ByteAddress,  // ByteAddressBuffer, RWByteAddressBuffer
    Typed,        // Buffer<T>, RWBuffer<T>
};

NODISCARD constexpr const CHAR* ToString(EBufferViewType ViewType)
{
    switch (ViewType)
    {
        case EBufferViewType::Structured:  return "Structured";
        case EBufferViewType::ByteAddress: return "ByteAddress";
        case EBufferViewType::Typed:       return "Typed";
        default:                           return "Unknown";
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

enum class EViewDimension : uint8
{
    None = 0,
    Buffer,
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    TextureCube,
    TextureCubeArray,
    Texture3D,
    AccelerationStructure,
};

enum class EDepthStencilViewFlags : uint8
{
    None            = 0,
    ReadOnlyDepth   = 1 << 0,
    ReadOnlyStencil = 1 << 1,
    ReadOnlyAll     = ReadOnlyDepth | ReadOnlyStencil,
};

ENUM_CLASS_OPERATORS(EDepthStencilViewFlags);

NODISCARD constexpr const CHAR* ToString(EViewDimension Dimension)
{
    switch (Dimension)
    {
        case EViewDimension::Buffer:                return "Buffer";
        case EViewDimension::Texture1D:             return "Texture1D";
        case EViewDimension::Texture1DArray:        return "Texture1DArray";
        case EViewDimension::Texture2D:             return "Texture2D";
        case EViewDimension::Texture2DArray:        return "Texture2DArray";
        case EViewDimension::TextureCube:           return "TextureCube";
        case EViewDimension::TextureCubeArray:      return "TextureCubeArray";
        case EViewDimension::Texture3D:             return "Texture3D";
        case EViewDimension::AccelerationStructure: return "AccelerationStructure";
        default:                                    return "None";
    }
}

NODISCARD constexpr bool IsBufferViewDimension(EViewDimension Dimension)
{
    return Dimension == EViewDimension::Buffer;
}

NODISCARD constexpr bool IsTextureViewDimension(EViewDimension Dimension)
{
    return Dimension != EViewDimension::None && Dimension != EViewDimension::Buffer && Dimension != EViewDimension::AccelerationStructure;
}

NODISCARD constexpr bool IsCubeViewDimension(EViewDimension Dimension)
{
    return Dimension == EViewDimension::TextureCube || Dimension == EViewDimension::TextureCubeArray;
}

NODISCARD constexpr bool IsAccelerationStructureViewDimension(EViewDimension Dimension)
{
    return Dimension == EViewDimension::AccelerationStructure;
}

NODISCARD inline bool IsViewDimensionCompatible(ETextureDimension TextureDimension, EViewDimension ViewDimension)
{
    if (ViewDimension == EViewDimension::None || ViewDimension == EViewDimension::Buffer)
    {
        return false;
    }

    switch (TextureDimension)
    {
        case ETextureDimension::Texture1D:
            return ViewDimension == EViewDimension::Texture1D;
        case ETextureDimension::Texture1DArray:
            return ViewDimension == EViewDimension::Texture1D || ViewDimension == EViewDimension::Texture1DArray;
        case ETextureDimension::Texture2D:
            return ViewDimension == EViewDimension::Texture2D;
        case ETextureDimension::Texture2DArray:
            return ViewDimension == EViewDimension::Texture2D || ViewDimension == EViewDimension::Texture2DArray;
        case ETextureDimension::TextureCube:
            return ViewDimension == EViewDimension::TextureCube || ViewDimension == EViewDimension::Texture2DArray || ViewDimension == EViewDimension::Texture2D;
        case ETextureDimension::TextureCubeArray:
            return ViewDimension == EViewDimension::TextureCubeArray || ViewDimension == EViewDimension::TextureCube ||
                   ViewDimension == EViewDimension::Texture2DArray || ViewDimension == EViewDimension::Texture2D;
        case ETextureDimension::Texture3D:
            return ViewDimension == EViewDimension::Texture3D;
        default:
            return false;
    }
}

NODISCARD constexpr EFormat SafeGetFormat(FRHITexture* Texture)
{
    return Texture ? Texture->GetDesc().Format : EFormat::Unknown;
}

NODISCARD constexpr uint16 SafeGetFullResourceSliceCount(FRHITexture* Texture, EViewDimension ViewDimension)
{
    if (!Texture)
    {
        return 1;
    }

    const FRHITextureDesc& Desc = Texture->GetDesc();
    switch (ViewDimension)
    {
        case EViewDimension::Texture3D:
        {
            return static_cast<uint16>(Desc.Extent.Z);
        }

        case EViewDimension::TextureCube:
        {
            return 1;
        }

        case EViewDimension::TextureCubeArray:
        {
            return static_cast<uint16>(Desc.NumArraySlices);
        }

        case EViewDimension::Texture1DArray:
        case EViewDimension::Texture2DArray:
        {
            return static_cast<uint16>(RHIDimensionArrayLayers(Desc.Dimension, Desc.NumArraySlices));
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
    struct FBufferSRV
    {
        constexpr bool operator==(const FBufferSRV& Other) const noexcept = default;

        EBufferViewType Type;
        uint32          FirstElement;
        uint32          NumElements;
        EFormat         Format;
    };

    struct FTexture1DSRV
    {
        constexpr bool operator==(const FTexture1DSRV& Other) const noexcept = default;

        EFormat Format;
        float   MinLODClamp;
        uint8   FirstMipLevel;
        uint8   NumMips;
    };

    struct FTexture1DArraySRV
    {
        constexpr bool operator==(const FTexture1DArraySRV& Other) const noexcept = default;

        EFormat Format;
        float   MinLODClamp;
        uint8   FirstMipLevel;
        uint8   NumMips;
        uint16  FirstArraySlice;
        uint16  NumSlices;
    };

    struct FTexture2DSRV
    {
        constexpr bool operator==(const FTexture2DSRV& Other) const noexcept = default;

        EFormat Format;
        float   MinLODClamp;
        uint8   FirstMipLevel;
        uint8   NumMips;
        uint8   PlaneSlice;
    };

    struct FTexture2DArraySRV
    {
        constexpr bool operator==(const FTexture2DArraySRV& Other) const noexcept = default;

        EFormat Format;
        float   MinLODClamp;
        uint8   FirstMipLevel;
        uint8   NumMips;
        uint8   PlaneSlice;
        uint16  FirstArraySlice;
        uint16  NumSlices;
    };

    struct FTextureCubeSRV
    {
        constexpr bool operator==(const FTextureCubeSRV& Other) const noexcept = default;

        EFormat Format;
        float   MinLODClamp;
        uint8   FirstMipLevel;
        uint8   NumMips;
    };

    struct FTextureCubeArraySRV
    {
        constexpr bool operator==(const FTextureCubeArraySRV& Other) const noexcept = default;

        EFormat Format;
        float   MinLODClamp;
        uint8   FirstMipLevel;
        uint8   NumMips;
        uint16  FirstCube;
        uint16  NumCubes;
    };

    struct FTexture3DSRV
    {
        constexpr bool operator==(const FTexture3DSRV& Other) const noexcept = default;

        EFormat Format;
        float   MinLODClamp;
        uint8   FirstMipLevel;
        uint8   NumMips;
    };

    struct FAccelerationStructureSRV
    {
        constexpr bool operator==(const FAccelerationStructureSRV& Other) const noexcept = default;
    };

public:
    NODISCARD static FRHIShaderResourceViewDesc CreateBuffer(uint32 InFirstElement, uint32 InNumElements, EBufferViewType InType = EBufferViewType::Structured)
    {
        FRHIShaderResourceViewDesc Desc;
        Desc.ViewDimension       = EViewDimension::Buffer;
        Desc.Buffer.Type         = InType;
        Desc.Buffer.FirstElement = InFirstElement;
        Desc.Buffer.NumElements  = InNumElements;
        Desc.Buffer.Format       = EFormat::Unknown;
        return Desc;
    }

    NODISCARD static FRHIShaderResourceViewDesc CreateTypedBuffer(uint32 InFirstElement, uint32 InNumElements, EFormat InFormat)
    {
        FRHIShaderResourceViewDesc Desc;
        Desc.ViewDimension       = EViewDimension::Buffer;
        Desc.Buffer.Type         = EBufferViewType::Typed;
        Desc.Buffer.FirstElement = InFirstElement;
        Desc.Buffer.NumElements  = InNumElements;
        Desc.Buffer.Format       = InFormat;
        return Desc;
    }

    NODISCARD static FRHIShaderResourceViewDesc CreateTexture1D(EFormat InFormat, uint8 InFirstMip, uint8 InNumMips, float InMinLODClamp = 0.0f)
    {
        FRHIShaderResourceViewDesc Desc;
        Desc.ViewDimension           = EViewDimension::Texture1D;
        Desc.Texture1D.Format        = InFormat;
        Desc.Texture1D.MinLODClamp   = InMinLODClamp;
        Desc.Texture1D.FirstMipLevel = InFirstMip;
        Desc.Texture1D.NumMips       = InNumMips;
        return Desc;
    }

    NODISCARD static FRHIShaderResourceViewDesc CreateTexture1DArray(EFormat InFormat, uint8 InFirstMip, uint8 InNumMips, uint16 InFirstArraySlice, uint16 InNumSlices, float InMinLODClamp = 0.0f)
    {
        FRHIShaderResourceViewDesc Desc;
        Desc.ViewDimension                  = EViewDimension::Texture1DArray;
        Desc.Texture1DArray.Format          = InFormat;
        Desc.Texture1DArray.MinLODClamp     = InMinLODClamp;
        Desc.Texture1DArray.FirstMipLevel   = InFirstMip;
        Desc.Texture1DArray.NumMips         = InNumMips;
        Desc.Texture1DArray.FirstArraySlice = InFirstArraySlice;
        Desc.Texture1DArray.NumSlices       = InNumSlices;
        return Desc;
    }

    NODISCARD static FRHIShaderResourceViewDesc CreateTexture2D(EFormat InFormat, uint8 InFirstMip, uint8 InNumMips, uint8 InPlaneSlice = 0, float InMinLODClamp = 0.0f)
    {
        FRHIShaderResourceViewDesc Desc;
        Desc.ViewDimension           = EViewDimension::Texture2D;
        Desc.Texture2D.Format        = InFormat;
        Desc.Texture2D.MinLODClamp   = InMinLODClamp;
        Desc.Texture2D.FirstMipLevel = InFirstMip;
        Desc.Texture2D.NumMips       = InNumMips;
        Desc.Texture2D.PlaneSlice    = InPlaneSlice;
        return Desc;
    }

    NODISCARD static FRHIShaderResourceViewDesc CreateTexture2DArray(EFormat InFormat, uint8 InFirstMip, uint8 InNumMips, uint16 InFirstArraySlice, uint16 InNumSlices, uint8 InPlaneSlice = 0, float InMinLODClamp = 0.0f)
    {
        FRHIShaderResourceViewDesc Desc;
        Desc.ViewDimension                  = EViewDimension::Texture2DArray;
        Desc.Texture2DArray.Format          = InFormat;
        Desc.Texture2DArray.MinLODClamp     = InMinLODClamp;
        Desc.Texture2DArray.FirstMipLevel   = InFirstMip;
        Desc.Texture2DArray.NumMips         = InNumMips;
        Desc.Texture2DArray.PlaneSlice      = InPlaneSlice;
        Desc.Texture2DArray.FirstArraySlice = InFirstArraySlice;
        Desc.Texture2DArray.NumSlices       = InNumSlices;
        return Desc;
    }

    NODISCARD static FRHIShaderResourceViewDesc CreateTextureCube(EFormat InFormat, uint8 InFirstMip, uint8 InNumMips, float InMinLODClamp = 0.0f)
    {
        FRHIShaderResourceViewDesc Desc;
        Desc.ViewDimension             = EViewDimension::TextureCube;
        Desc.TextureCube.Format        = InFormat;
        Desc.TextureCube.MinLODClamp   = InMinLODClamp;
        Desc.TextureCube.FirstMipLevel = InFirstMip;
        Desc.TextureCube.NumMips       = InNumMips;
        return Desc;
    }

    NODISCARD static FRHIShaderResourceViewDesc CreateTextureCubeArray(EFormat InFormat, uint8 InFirstMip, uint8 InNumMips, uint16 InFirstCube, uint16 InNumCubes, float InMinLODClamp = 0.0f)
    {
        FRHIShaderResourceViewDesc Desc;
        Desc.ViewDimension                  = EViewDimension::TextureCubeArray;
        Desc.TextureCubeArray.Format        = InFormat;
        Desc.TextureCubeArray.MinLODClamp   = InMinLODClamp;
        Desc.TextureCubeArray.FirstMipLevel = InFirstMip;
        Desc.TextureCubeArray.NumMips       = InNumMips;
        Desc.TextureCubeArray.FirstCube     = InFirstCube;
        Desc.TextureCubeArray.NumCubes      = InNumCubes;
        return Desc;
    }

    NODISCARD static FRHIShaderResourceViewDesc CreateTexture3D(EFormat InFormat, uint8 InFirstMip, uint8 InNumMips, float InMinLODClamp = 0.0f)
    {
        FRHIShaderResourceViewDesc Desc;
        Desc.ViewDimension           = EViewDimension::Texture3D;
        Desc.Texture3D.Format        = InFormat;
        Desc.Texture3D.MinLODClamp   = InMinLODClamp;
        Desc.Texture3D.FirstMipLevel = InFirstMip;
        Desc.Texture3D.NumMips       = InNumMips;
        return Desc;
    }

    NODISCARD static FRHIShaderResourceViewDesc CreateAccelerationStructure()
    {
        FRHIShaderResourceViewDesc Desc;
        Desc.ViewDimension         = EViewDimension::AccelerationStructure;
        Desc.AccelerationStructure = {};
        return Desc;
    }

public:
    FRHIShaderResourceViewDesc() noexcept
        : ViewDimension(EViewDimension::None)
        , Buffer{}
    {
    }

    NODISCARD constexpr bool IsBufferSRV()                const { return IsBufferViewDimension(ViewDimension); }
    NODISCARD constexpr bool IsTextureSRV()               const { return IsTextureViewDimension(ViewDimension); }
    NODISCARD constexpr bool IsAccelerationStructureSRV() const { return IsAccelerationStructureViewDimension(ViewDimension); }

    NODISCARD FORCEINLINE EFormat GetFormat() const noexcept
    {
        switch (ViewDimension)
        {
            case EViewDimension::Texture1D:
                return Texture1D.Format;
            case EViewDimension::Texture1DArray:
                return Texture1DArray.Format;
            case EViewDimension::Texture2D:
                return Texture2D.Format;
            case EViewDimension::Texture2DArray:
                return Texture2DArray.Format;
            case EViewDimension::TextureCube:
                return TextureCube.Format;
            case EViewDimension::TextureCubeArray:
                return TextureCubeArray.Format;
            case EViewDimension::Texture3D:
                return Texture3D.Format;
            default:
                return EFormat::Unknown;
        }
    }

    NODISCARD bool operator==(const FRHIShaderResourceViewDesc& Other) const noexcept
    {
        if (ViewDimension != Other.ViewDimension)
        {
            return false;
        }

        switch (ViewDimension)
        {
            case EViewDimension::Buffer:
                return Buffer == Other.Buffer;
            case EViewDimension::Texture1D: 
                return Texture1D == Other.Texture1D;
            case EViewDimension::Texture1DArray:
                return Texture1DArray == Other.Texture1DArray;
            case EViewDimension::Texture2D:
                return Texture2D == Other.Texture2D;
            case EViewDimension::Texture2DArray:
                return Texture2DArray == Other.Texture2DArray;
            case EViewDimension::TextureCube:
                return TextureCube == Other.TextureCube;
            case EViewDimension::TextureCubeArray:
                return TextureCubeArray == Other.TextureCubeArray;
            case EViewDimension::Texture3D:
                return Texture3D == Other.Texture3D;
            case EViewDimension::AccelerationStructure:
                return AccelerationStructure == Other.AccelerationStructure;
            default:
                return true;
        }
    }

    EViewDimension ViewDimension;

    union
    {
        FBufferSRV                Buffer;
        FTexture1DSRV             Texture1D;
        FTexture1DArraySRV        Texture1DArray;
        FTexture2DSRV             Texture2D;
        FTexture2DArraySRV        Texture2DArray;
        FTextureCubeSRV           TextureCube;
        FTextureCubeArraySRV      TextureCubeArray;
        FTexture3DSRV             Texture3D;
        FAccelerationStructureSRV AccelerationStructure;
    };
};

template<>
struct THash<FRHIShaderResourceViewDesc::FBufferSRV>
{
    NODISCARD static uint64 GetHash(const FRHIShaderResourceViewDesc::FBufferSRV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Type);
        HashCombine(Result, Value.FirstElement);
        HashCombine(Result, Value.NumElements);
        return Result;
    }
};

template<>
struct THash<FRHIShaderResourceViewDesc::FTexture1DSRV>
{
    NODISCARD static uint64 GetHash(const FRHIShaderResourceViewDesc::FTexture1DSRV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MinLODClamp);
        HashCombine(Result, Value.FirstMipLevel);
        HashCombine(Result, Value.NumMips);
        return Result;
    }
};

template<>
struct THash<FRHIShaderResourceViewDesc::FTexture1DArraySRV>
{
    NODISCARD static uint64 GetHash(const FRHIShaderResourceViewDesc::FTexture1DArraySRV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MinLODClamp);
        HashCombine(Result, Value.FirstMipLevel);
        HashCombine(Result, Value.NumMips);
        HashCombine(Result, Value.FirstArraySlice);
        HashCombine(Result, Value.NumSlices);
        return Result;
    }
};

template<>
struct THash<FRHIShaderResourceViewDesc::FTexture2DSRV>
{
    NODISCARD static uint64 GetHash(const FRHIShaderResourceViewDesc::FTexture2DSRV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MinLODClamp);
        HashCombine(Result, Value.FirstMipLevel);
        HashCombine(Result, Value.NumMips);
        HashCombine(Result, Value.PlaneSlice);
        return Result;
    }
};

template<>
struct THash<FRHIShaderResourceViewDesc::FTexture2DArraySRV>
{
    NODISCARD static uint64 GetHash(const FRHIShaderResourceViewDesc::FTexture2DArraySRV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MinLODClamp);
        HashCombine(Result, Value.FirstMipLevel);
        HashCombine(Result, Value.NumMips);
        HashCombine(Result, Value.PlaneSlice);
        HashCombine(Result, Value.FirstArraySlice);
        HashCombine(Result, Value.NumSlices);
        return Result;
    }
};

template<>
struct THash<FRHIShaderResourceViewDesc::FTextureCubeSRV>
{
    NODISCARD static uint64 GetHash(const FRHIShaderResourceViewDesc::FTextureCubeSRV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MinLODClamp);
        HashCombine(Result, Value.FirstMipLevel);
        HashCombine(Result, Value.NumMips);
        return Result;
    }
};

template<>
struct THash<FRHIShaderResourceViewDesc::FTextureCubeArraySRV>
{
    NODISCARD static uint64 GetHash(const FRHIShaderResourceViewDesc::FTextureCubeArraySRV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MinLODClamp);
        HashCombine(Result, Value.FirstMipLevel);
        HashCombine(Result, Value.NumMips);
        HashCombine(Result, Value.FirstCube);
        HashCombine(Result, Value.NumCubes);
        return Result;
    }
};

template<>
struct THash<FRHIShaderResourceViewDesc::FTexture3DSRV>
{
    NODISCARD static uint64 GetHash(const FRHIShaderResourceViewDesc::FTexture3DSRV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MinLODClamp);
        HashCombine(Result, Value.FirstMipLevel);
        HashCombine(Result, Value.NumMips);
        return Result;
    }
};

template<>
struct THash<FRHIShaderResourceViewDesc::FAccelerationStructureSRV>
{
    NODISCARD static uint64 GetHash(const FRHIShaderResourceViewDesc::FAccelerationStructureSRV& /*Value*/)
    {
        return 0;
    }
};

template<>
struct THash<FRHIShaderResourceViewDesc>
{
    NODISCARD static uint64 GetHash(const FRHIShaderResourceViewDesc& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.ViewDimension);
        switch (Value.ViewDimension)
        {
            case EViewDimension::Buffer:
                HashCombine(Result, THash<FRHIShaderResourceViewDesc::FBufferSRV>::GetHash(Value.Buffer));
                break;
            case EViewDimension::Texture1D:
                HashCombine(Result, THash<FRHIShaderResourceViewDesc::FTexture1DSRV>::GetHash(Value.Texture1D));
                break;
            case EViewDimension::Texture1DArray:
                HashCombine(Result, THash<FRHIShaderResourceViewDesc::FTexture1DArraySRV>::GetHash(Value.Texture1DArray));
                break;
            case EViewDimension::Texture2D:
                HashCombine(Result, THash<FRHIShaderResourceViewDesc::FTexture2DSRV>::GetHash(Value.Texture2D));
                break;
            case EViewDimension::Texture2DArray:
                HashCombine(Result, THash<FRHIShaderResourceViewDesc::FTexture2DArraySRV>::GetHash(Value.Texture2DArray));
                break;
            case EViewDimension::TextureCube:
                HashCombine(Result, THash<FRHIShaderResourceViewDesc::FTextureCubeSRV>::GetHash(Value.TextureCube));
                break;
            case EViewDimension::TextureCubeArray:
                HashCombine(Result, THash<FRHIShaderResourceViewDesc::FTextureCubeArraySRV>::GetHash(Value.TextureCubeArray));
                break;
            case EViewDimension::Texture3D:
                HashCombine(Result, THash<FRHIShaderResourceViewDesc::FTexture3DSRV>::GetHash(Value.Texture3D));
                break;
            case EViewDimension::AccelerationStructure:
                HashCombine(Result, THash<FRHIShaderResourceViewDesc::FAccelerationStructureSRV>::GetHash(Value.AccelerationStructure));
                break;
            default:
                break;
        }

        return Result;
    }
};

struct FRHIUnorderedAccessViewDesc
{
public:
    struct FBufferUAV
    {
        constexpr bool operator==(const FBufferUAV& Other) const noexcept = default;

        EBufferViewType Type;
        uint32          FirstElement;
        uint32          NumElements;
        EFormat         Format; // Only used when Type == EBufferViewType::Typed
    };

    struct FTexture1DUAV
    {
        constexpr bool operator==(const FTexture1DUAV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
    };

    struct FTexture1DArrayUAV
    {
        constexpr bool operator==(const FTexture1DArrayUAV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint16  FirstArraySlice;
        uint16  NumSlices;
    };

    struct FTexture2DUAV
    {
        constexpr bool operator==(const FTexture2DUAV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint8   PlaneSlice;
    };

    struct FTexture2DArrayUAV
    {
        constexpr bool operator==(const FTexture2DArrayUAV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint8   PlaneSlice;
        uint16  FirstArraySlice;
        uint16  NumSlices;
    };

    struct FTextureCubeUAV
    {
        constexpr bool operator==(const FTextureCubeUAV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
    };

    struct FTextureCubeArrayUAV
    {
        constexpr bool operator==(const FTextureCubeArrayUAV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint16  FirstCube;
        uint16  NumCubes;
    };

    struct FTexture3DUAV
    {
        constexpr bool operator==(const FTexture3DUAV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint16  FirstWSlice;
        uint16  WSize;
    };

public:
    NODISCARD static FRHIUnorderedAccessViewDesc CreateBuffer(uint32 InFirstElement, uint32 InNumElements, EBufferViewType InType = EBufferViewType::Structured)
    {
        FRHIUnorderedAccessViewDesc Desc;
        Desc.ViewDimension       = EViewDimension::Buffer;
        Desc.Buffer.Type         = InType;
        Desc.Buffer.FirstElement = InFirstElement;
        Desc.Buffer.NumElements  = InNumElements;
        Desc.Buffer.Format       = EFormat::Unknown;
        return Desc;
    }

    NODISCARD static FRHIUnorderedAccessViewDesc CreateTypedBuffer(uint32 InFirstElement, uint32 InNumElements, EFormat InFormat)
    {
        FRHIUnorderedAccessViewDesc Desc;
        Desc.ViewDimension       = EViewDimension::Buffer;
        Desc.Buffer.Type         = EBufferViewType::Typed;
        Desc.Buffer.FirstElement = InFirstElement;
        Desc.Buffer.NumElements  = InNumElements;
        Desc.Buffer.Format       = InFormat;
        return Desc;
    }

    NODISCARD static FRHIUnorderedAccessViewDesc CreateTexture1D(EFormat InFormat, uint8 InMipLevel)
    {
        FRHIUnorderedAccessViewDesc Desc;
        Desc.ViewDimension      = EViewDimension::Texture1D;
        Desc.Texture1D.Format   = InFormat;
        Desc.Texture1D.MipLevel = InMipLevel;
        return Desc;
    }

    NODISCARD static FRHIUnorderedAccessViewDesc CreateTexture1DArray(EFormat InFormat, uint8 InMipLevel, uint16 InFirstArraySlice, uint16 InNumSlices)
    {
        FRHIUnorderedAccessViewDesc Desc;
        Desc.ViewDimension                  = EViewDimension::Texture1DArray;
        Desc.Texture1DArray.Format          = InFormat;
        Desc.Texture1DArray.MipLevel        = InMipLevel;
        Desc.Texture1DArray.FirstArraySlice = InFirstArraySlice;
        Desc.Texture1DArray.NumSlices       = InNumSlices;
        return Desc;
    }

    NODISCARD static FRHIUnorderedAccessViewDesc CreateTexture2D(EFormat InFormat, uint8 InMipLevel, uint8 InPlaneSlice = 0)
    {
        FRHIUnorderedAccessViewDesc Desc;
        Desc.ViewDimension        = EViewDimension::Texture2D;
        Desc.Texture2D.Format     = InFormat;
        Desc.Texture2D.MipLevel   = InMipLevel;
        Desc.Texture2D.PlaneSlice = InPlaneSlice;
        return Desc;
    }

    NODISCARD static FRHIUnorderedAccessViewDesc CreateTexture2DArray(EFormat InFormat, uint8 InMipLevel, uint16 InFirstArraySlice, uint16 InNumSlices, uint8 InPlaneSlice = 0)
    {
        FRHIUnorderedAccessViewDesc Desc;
        Desc.ViewDimension                  = EViewDimension::Texture2DArray;
        Desc.Texture2DArray.Format          = InFormat;
        Desc.Texture2DArray.MipLevel        = InMipLevel;
        Desc.Texture2DArray.PlaneSlice      = InPlaneSlice;
        Desc.Texture2DArray.FirstArraySlice = InFirstArraySlice;
        Desc.Texture2DArray.NumSlices       = InNumSlices;
        return Desc;
    }

    NODISCARD static FRHIUnorderedAccessViewDesc CreateTextureCube(EFormat InFormat, uint8 InMipLevel)
    {
        FRHIUnorderedAccessViewDesc Desc;
        Desc.ViewDimension        = EViewDimension::TextureCube;
        Desc.TextureCube.Format   = InFormat;
        Desc.TextureCube.MipLevel = InMipLevel;
        return Desc;
    }

    NODISCARD static FRHIUnorderedAccessViewDesc CreateTextureCubeArray(EFormat InFormat, uint8 InMipLevel, uint16 InFirstCube, uint16 InNumCubes)
    {
        FRHIUnorderedAccessViewDesc Desc;
        Desc.ViewDimension              = EViewDimension::TextureCubeArray;
        Desc.TextureCubeArray.Format    = InFormat;
        Desc.TextureCubeArray.MipLevel  = InMipLevel;
        Desc.TextureCubeArray.FirstCube = InFirstCube;
        Desc.TextureCubeArray.NumCubes  = InNumCubes;
        return Desc;
    }

    NODISCARD static FRHIUnorderedAccessViewDesc CreateTexture3D(EFormat InFormat, uint8 InMipLevel, uint16 InFirstWSlice, uint16 InWSize)
    {
        FRHIUnorderedAccessViewDesc Desc;
        Desc.ViewDimension         = EViewDimension::Texture3D;
        Desc.Texture3D.Format      = InFormat;
        Desc.Texture3D.MipLevel    = InMipLevel;
        Desc.Texture3D.FirstWSlice = InFirstWSlice;
        Desc.Texture3D.WSize       = InWSize;
        return Desc;
    }

public:
    FRHIUnorderedAccessViewDesc() noexcept
        : ViewDimension(EViewDimension::None)
        , Buffer{}
    {
    }

    NODISCARD constexpr bool IsBufferUAV()  const { return IsBufferViewDimension(ViewDimension); }
    NODISCARD constexpr bool IsTextureUAV() const { return IsTextureViewDimension(ViewDimension); }

    NODISCARD FORCEINLINE EFormat GetFormat() const noexcept
    {
        switch (ViewDimension)
        {
            case EViewDimension::Texture1D:
                return Texture1D.Format;
            case EViewDimension::Texture1DArray:
                return Texture1DArray.Format;
            case EViewDimension::Texture2D:
                return Texture2D.Format;
            case EViewDimension::Texture2DArray:
                return Texture2DArray.Format;
            case EViewDimension::TextureCube:
                return TextureCube.Format;
            case EViewDimension::TextureCubeArray:
                return TextureCubeArray.Format;
            case EViewDimension::Texture3D:
                return Texture3D.Format;
            default:
                return EFormat::Unknown;
        }
    }

    NODISCARD bool operator==(const FRHIUnorderedAccessViewDesc& Other) const noexcept
    {
        if (ViewDimension != Other.ViewDimension)
        {
            return false;
        }

        switch (ViewDimension)
        {
            case EViewDimension::Buffer:
                return Buffer == Other.Buffer;
            case EViewDimension::Texture1D:
                return Texture1D == Other.Texture1D;
            case EViewDimension::Texture1DArray:
                return Texture1DArray == Other.Texture1DArray;
            case EViewDimension::Texture2D:
                return Texture2D == Other.Texture2D;
            case EViewDimension::Texture2DArray:
                return Texture2DArray == Other.Texture2DArray;
            case EViewDimension::TextureCube:
                return TextureCube == Other.TextureCube;
            case EViewDimension::TextureCubeArray:
                return TextureCubeArray == Other.TextureCubeArray;
            case EViewDimension::Texture3D:
                return Texture3D == Other.Texture3D;
            default:
                return true;
        }
    }

    EViewDimension ViewDimension;

    union
    {
        FBufferUAV           Buffer;
        FTexture1DUAV        Texture1D;
        FTexture1DArrayUAV   Texture1DArray;
        FTexture2DUAV        Texture2D;
        FTexture2DArrayUAV   Texture2DArray;
        FTextureCubeUAV      TextureCube;
        FTextureCubeArrayUAV TextureCubeArray;
        FTexture3DUAV        Texture3D;
    };
};

template<>
struct THash<FRHIUnorderedAccessViewDesc::FBufferUAV>
{
    NODISCARD static uint64 GetHash(const FRHIUnorderedAccessViewDesc::FBufferUAV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Type);
        HashCombine(Result, Value.FirstElement);
        HashCombine(Result, Value.NumElements);
        return Result;
    }
};

template<>
struct THash<FRHIUnorderedAccessViewDesc::FTexture1DUAV>
{
    NODISCARD static uint64 GetHash(const FRHIUnorderedAccessViewDesc::FTexture1DUAV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        return Result;
    }
};

template<>
struct THash<FRHIUnorderedAccessViewDesc::FTexture1DArrayUAV>
{
    NODISCARD static uint64 GetHash(const FRHIUnorderedAccessViewDesc::FTexture1DArrayUAV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.FirstArraySlice);
        HashCombine(Result, Value.NumSlices);
        return Result;
    }
};

template<>
struct THash<FRHIUnorderedAccessViewDesc::FTexture2DUAV>
{
    NODISCARD static uint64 GetHash(const FRHIUnorderedAccessViewDesc::FTexture2DUAV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.PlaneSlice);
        return Result;
    }
};

template<>
struct THash<FRHIUnorderedAccessViewDesc::FTexture2DArrayUAV>
{
    NODISCARD static uint64 GetHash(const FRHIUnorderedAccessViewDesc::FTexture2DArrayUAV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.PlaneSlice);
        HashCombine(Result, Value.FirstArraySlice);
        HashCombine(Result, Value.NumSlices);
        return Result;
    }
};

template<>
struct THash<FRHIUnorderedAccessViewDesc::FTextureCubeUAV>
{
    NODISCARD static uint64 GetHash(const FRHIUnorderedAccessViewDesc::FTextureCubeUAV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        return Result;
    }
};

template<>
struct THash<FRHIUnorderedAccessViewDesc::FTextureCubeArrayUAV>
{
    NODISCARD static uint64 GetHash(const FRHIUnorderedAccessViewDesc::FTextureCubeArrayUAV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.FirstCube);
        HashCombine(Result, Value.NumCubes);
        return Result;
    }
};

template<>
struct THash<FRHIUnorderedAccessViewDesc::FTexture3DUAV>
{
    NODISCARD static uint64 GetHash(const FRHIUnorderedAccessViewDesc::FTexture3DUAV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.FirstWSlice);
        HashCombine(Result, Value.WSize);
        return Result;
    }
};

template<>
struct THash<FRHIUnorderedAccessViewDesc>
{
    NODISCARD static uint64 GetHash(const FRHIUnorderedAccessViewDesc& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.ViewDimension);
        switch (Value.ViewDimension)
        {
            case EViewDimension::Buffer:
                HashCombine(Result, THash<FRHIUnorderedAccessViewDesc::FBufferUAV>::GetHash(Value.Buffer));
                break;
            case EViewDimension::Texture1D:
                HashCombine(Result, THash<FRHIUnorderedAccessViewDesc::FTexture1DUAV>::GetHash(Value.Texture1D));
                break;
            case EViewDimension::Texture1DArray:
                HashCombine(Result, THash<FRHIUnorderedAccessViewDesc::FTexture1DArrayUAV>::GetHash(Value.Texture1DArray));
                break;
            case EViewDimension::Texture2D:
                HashCombine(Result, THash<FRHIUnorderedAccessViewDesc::FTexture2DUAV>::GetHash(Value.Texture2D));
                break;
            case EViewDimension::Texture2DArray:
                HashCombine(Result, THash<FRHIUnorderedAccessViewDesc::FTexture2DArrayUAV>::GetHash(Value.Texture2DArray));
                break;
            case EViewDimension::TextureCube:
                HashCombine(Result, THash<FRHIUnorderedAccessViewDesc::FTextureCubeUAV>::GetHash(Value.TextureCube));
                break;
            case EViewDimension::TextureCubeArray:
                HashCombine(Result, THash<FRHIUnorderedAccessViewDesc::FTextureCubeArrayUAV>::GetHash(Value.TextureCubeArray));
                break;
            case EViewDimension::Texture3D:
                HashCombine(Result, THash<FRHIUnorderedAccessViewDesc::FTexture3DUAV>::GetHash(Value.Texture3D));
                break;
            default:
                break;
        }

        return Result;
    }
};

struct FRHIRenderTargetViewDesc
{
public:
    struct FTexture1DRTV
    {
        constexpr bool operator==(const FTexture1DRTV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
    };

    struct FTexture1DArrayRTV
    {
        constexpr bool operator==(const FTexture1DArrayRTV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint16  FirstArraySlice;
        uint16  NumSlices;
    };

    struct FTexture2DRTV
    {
        constexpr bool operator==(const FTexture2DRTV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint8   PlaneSlice;
    };

    struct FTexture2DArrayRTV
    {
        constexpr bool operator==(const FTexture2DArrayRTV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint8   PlaneSlice;
        uint16  FirstArraySlice;
        uint16  NumSlices;
    };

    struct FTextureCubeRTV
    {
        constexpr bool operator==(const FTextureCubeRTV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
    };

    struct FTextureCubeArrayRTV
    {
        constexpr bool operator==(const FTextureCubeArrayRTV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint16  FirstCube;
        uint16  NumCubes;
    };

    struct FTexture3DRTV
    {
        constexpr bool operator==(const FTexture3DRTV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint16  FirstWSlice;
        uint16  WSize;
    };

public:
    NODISCARD static FRHIRenderTargetViewDesc CreateTexture1D(EFormat InFormat, uint8 InMipLevel)
    {
        FRHIRenderTargetViewDesc Desc;
        Desc.ViewDimension      = EViewDimension::Texture1D;
        Desc.Texture1D.Format   = InFormat;
        Desc.Texture1D.MipLevel = InMipLevel;
        return Desc;
    }

    NODISCARD static FRHIRenderTargetViewDesc CreateTexture1DArray(EFormat InFormat, uint8 InMipLevel, uint16 InFirstArraySlice, uint16 InNumSlices)
    {
        FRHIRenderTargetViewDesc Desc;
        Desc.ViewDimension                  = EViewDimension::Texture1DArray;
        Desc.Texture1DArray.Format          = InFormat;
        Desc.Texture1DArray.MipLevel        = InMipLevel;
        Desc.Texture1DArray.FirstArraySlice = InFirstArraySlice;
        Desc.Texture1DArray.NumSlices       = InNumSlices;
        return Desc;
    }

    NODISCARD static FRHIRenderTargetViewDesc CreateTexture2D(EFormat InFormat, uint8 InMipLevel, uint8 InPlaneSlice = 0)
    {
        FRHIRenderTargetViewDesc Desc;
        Desc.ViewDimension        = EViewDimension::Texture2D;
        Desc.Texture2D.Format     = InFormat;
        Desc.Texture2D.MipLevel   = InMipLevel;
        Desc.Texture2D.PlaneSlice = InPlaneSlice;
        return Desc;
    }

    NODISCARD static FRHIRenderTargetViewDesc CreateTexture2DArray(EFormat InFormat, uint8 InMipLevel, uint16 InFirstArraySlice, uint16 InNumSlices, uint8 InPlaneSlice = 0)
    {
        FRHIRenderTargetViewDesc Desc;
        Desc.ViewDimension                  = EViewDimension::Texture2DArray;
        Desc.Texture2DArray.Format          = InFormat;
        Desc.Texture2DArray.MipLevel        = InMipLevel;
        Desc.Texture2DArray.PlaneSlice      = InPlaneSlice;
        Desc.Texture2DArray.FirstArraySlice = InFirstArraySlice;
        Desc.Texture2DArray.NumSlices       = InNumSlices;
        return Desc;
    }

    NODISCARD static FRHIRenderTargetViewDesc CreateTextureCube(EFormat InFormat, uint8 InMipLevel)
    {
        FRHIRenderTargetViewDesc Desc;
        Desc.ViewDimension        = EViewDimension::TextureCube;
        Desc.TextureCube.Format   = InFormat;
        Desc.TextureCube.MipLevel = InMipLevel;
        return Desc;
    }

    NODISCARD static FRHIRenderTargetViewDesc CreateTextureCubeArray(EFormat InFormat, uint8 InMipLevel, uint16 InFirstCube, uint16 InNumCubes)
    {
        FRHIRenderTargetViewDesc Desc;
        Desc.ViewDimension              = EViewDimension::TextureCubeArray;
        Desc.TextureCubeArray.Format    = InFormat;
        Desc.TextureCubeArray.MipLevel  = InMipLevel;
        Desc.TextureCubeArray.FirstCube = InFirstCube;
        Desc.TextureCubeArray.NumCubes  = InNumCubes;
        return Desc;
    }

    NODISCARD static FRHIRenderTargetViewDesc CreateTexture3D(EFormat InFormat, uint8 InMipLevel, uint16 InFirstWSlice, uint16 InWSize)
    {
        FRHIRenderTargetViewDesc Desc;
        Desc.ViewDimension         = EViewDimension::Texture3D;
        Desc.Texture3D.Format      = InFormat;
        Desc.Texture3D.MipLevel    = InMipLevel;
        Desc.Texture3D.FirstWSlice = InFirstWSlice;
        Desc.Texture3D.WSize       = InWSize;
        return Desc;
    }

public:
    FRHIRenderTargetViewDesc() noexcept
        : ViewDimension(EViewDimension::None)
        , Texture2D{}
    {
    }

    NODISCARD FORCEINLINE EFormat GetFormat() const noexcept
    {
        switch (ViewDimension)
        {
            case EViewDimension::Texture1D:
                return Texture1D.Format;
            case EViewDimension::Texture1DArray:
                return Texture1DArray.Format;
            case EViewDimension::Texture2D:
                return Texture2D.Format;
            case EViewDimension::Texture2DArray:
                return Texture2DArray.Format;
            case EViewDimension::TextureCube:
                return TextureCube.Format;
            case EViewDimension::TextureCubeArray:
                return TextureCubeArray.Format;
            case EViewDimension::Texture3D:
                return Texture3D.Format;
            default:
                return EFormat::Unknown;
        }
    }

    NODISCARD bool operator==(const FRHIRenderTargetViewDesc& Other) const noexcept
    {
        if (ViewDimension != Other.ViewDimension)
        {
            return false;
        }

        switch (ViewDimension)
        {
            case EViewDimension::Texture1D:
                return Texture1D == Other.Texture1D;
            case EViewDimension::Texture1DArray:
                return Texture1DArray == Other.Texture1DArray;
            case EViewDimension::Texture2D:
                return Texture2D == Other.Texture2D;
            case EViewDimension::Texture2DArray:
                return Texture2DArray == Other.Texture2DArray;
            case EViewDimension::TextureCube:
                return TextureCube == Other.TextureCube;
            case EViewDimension::TextureCubeArray:
                return TextureCubeArray == Other.TextureCubeArray;
            case EViewDimension::Texture3D:
                return Texture3D == Other.Texture3D;
            default:
                return true;
        }
    }

    EViewDimension ViewDimension;

    union
    {
        FTexture1DRTV        Texture1D;
        FTexture1DArrayRTV   Texture1DArray;
        FTexture2DRTV        Texture2D;
        FTexture2DArrayRTV   Texture2DArray;
        FTextureCubeRTV      TextureCube;
        FTextureCubeArrayRTV TextureCubeArray;
        FTexture3DRTV        Texture3D;
    };
};

template<>
struct THash<FRHIRenderTargetViewDesc::FTexture1DRTV>
{
    NODISCARD static uint64 GetHash(const FRHIRenderTargetViewDesc::FTexture1DRTV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        return Result;
    }
};

template<>
struct THash<FRHIRenderTargetViewDesc::FTexture1DArrayRTV>
{
    NODISCARD static uint64 GetHash(const FRHIRenderTargetViewDesc::FTexture1DArrayRTV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.FirstArraySlice);
        HashCombine(Result, Value.NumSlices);
        return Result;
    }
};

template<>
struct THash<FRHIRenderTargetViewDesc::FTexture2DRTV>
{
    NODISCARD static uint64 GetHash(const FRHIRenderTargetViewDesc::FTexture2DRTV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.PlaneSlice);
        return Result;
    }
};

template<>
struct THash<FRHIRenderTargetViewDesc::FTexture2DArrayRTV>
{
    NODISCARD static uint64 GetHash(const FRHIRenderTargetViewDesc::FTexture2DArrayRTV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.PlaneSlice);
        HashCombine(Result, Value.FirstArraySlice);
        HashCombine(Result, Value.NumSlices);
        return Result;
    }
};

template<>
struct THash<FRHIRenderTargetViewDesc::FTextureCubeRTV>
{
    NODISCARD static uint64 GetHash(const FRHIRenderTargetViewDesc::FTextureCubeRTV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        return Result;
    }
};

template<>
struct THash<FRHIRenderTargetViewDesc::FTextureCubeArrayRTV>
{
    NODISCARD static uint64 GetHash(const FRHIRenderTargetViewDesc::FTextureCubeArrayRTV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.FirstCube);
        HashCombine(Result, Value.NumCubes);
        return Result;
    }
};

template<>
struct THash<FRHIRenderTargetViewDesc::FTexture3DRTV>
{
    NODISCARD static uint64 GetHash(const FRHIRenderTargetViewDesc::FTexture3DRTV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.FirstWSlice);
        HashCombine(Result, Value.WSize);
        return Result;
    }
};

template<>
struct THash<FRHIRenderTargetViewDesc>
{
    NODISCARD static uint64 GetHash(const FRHIRenderTargetViewDesc& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.ViewDimension);
        switch (Value.ViewDimension)
        {
            case EViewDimension::Texture1D:
                HashCombine(Result, THash<FRHIRenderTargetViewDesc::FTexture1DRTV>::GetHash(Value.Texture1D));
                break;
            case EViewDimension::Texture1DArray:
                HashCombine(Result, THash<FRHIRenderTargetViewDesc::FTexture1DArrayRTV>::GetHash(Value.Texture1DArray));
                break;
            case EViewDimension::Texture2D:
                HashCombine(Result, THash<FRHIRenderTargetViewDesc::FTexture2DRTV>::GetHash(Value.Texture2D));
                break;
            case EViewDimension::Texture2DArray:
                HashCombine(Result, THash<FRHIRenderTargetViewDesc::FTexture2DArrayRTV>::GetHash(Value.Texture2DArray));
                break;
            case EViewDimension::TextureCube:
                HashCombine(Result, THash<FRHIRenderTargetViewDesc::FTextureCubeRTV>::GetHash(Value.TextureCube));
                break;
            case EViewDimension::TextureCubeArray:
                HashCombine(Result, THash<FRHIRenderTargetViewDesc::FTextureCubeArrayRTV>::GetHash(Value.TextureCubeArray));
                break;
            case EViewDimension::Texture3D:
                HashCombine(Result, THash<FRHIRenderTargetViewDesc::FTexture3DRTV>::GetHash(Value.Texture3D));
                break;
            default:
                break;
        }

        return Result;
    }
};

struct FRHIDepthStencilViewDesc
{
public:
    struct FTexture1DDSV
    {
        constexpr bool operator==(const FTexture1DDSV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
    };

    struct FTexture1DArrayDSV
    {
        constexpr bool operator==(const FTexture1DArrayDSV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint16  FirstArraySlice;
        uint16  NumSlices;
    };

    struct FTexture2DDSV
    {
        constexpr bool operator==(const FTexture2DDSV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
    };

    struct FTexture2DArrayDSV
    {
        constexpr bool operator==(const FTexture2DArrayDSV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint16  FirstArraySlice;
        uint16  NumSlices;
    };

    struct FTextureCubeDSV
    {
        constexpr bool operator==(const FTextureCubeDSV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
    };

    struct FTextureCubeArrayDSV
    {
        constexpr bool operator==(const FTextureCubeArrayDSV& Other) const noexcept = default;

        EFormat Format;
        uint8   MipLevel;
        uint16  FirstCube;
        uint16  NumCubes;
    };

public:
    NODISCARD static FRHIDepthStencilViewDesc CreateTexture1D(EFormat InFormat, uint8 InMipLevel,
        EDepthStencilViewFlags InFlags = EDepthStencilViewFlags::None)
    {
        FRHIDepthStencilViewDesc Desc;
        Desc.ViewDimension      = EViewDimension::Texture1D;
        Desc.Flags              = InFlags;
        Desc.Texture1D.Format   = InFormat;
        Desc.Texture1D.MipLevel = InMipLevel;
        return Desc;
    }

    NODISCARD static FRHIDepthStencilViewDesc CreateTexture1DArray(EFormat InFormat, uint8 InMipLevel, uint16 InFirstArraySlice, uint16 InNumSlices,
        EDepthStencilViewFlags InFlags = EDepthStencilViewFlags::None)
    {
        FRHIDepthStencilViewDesc Desc;
        Desc.ViewDimension                  = EViewDimension::Texture1DArray;
        Desc.Flags                          = InFlags;
        Desc.Texture1DArray.Format          = InFormat;
        Desc.Texture1DArray.MipLevel        = InMipLevel;
        Desc.Texture1DArray.FirstArraySlice = InFirstArraySlice;
        Desc.Texture1DArray.NumSlices       = InNumSlices;
        return Desc;
    }

    NODISCARD static FRHIDepthStencilViewDesc CreateTexture2D(EFormat InFormat, uint8 InMipLevel,
        EDepthStencilViewFlags InFlags = EDepthStencilViewFlags::None)
    {
        FRHIDepthStencilViewDesc Desc;
        Desc.ViewDimension      = EViewDimension::Texture2D;
        Desc.Flags              = InFlags;
        Desc.Texture2D.Format   = InFormat;
        Desc.Texture2D.MipLevel = InMipLevel;
        return Desc;
    }

    NODISCARD static FRHIDepthStencilViewDesc CreateTexture2DArray(EFormat InFormat, uint8 InMipLevel, uint16 InFirstArraySlice, uint16 InNumSlices,
        EDepthStencilViewFlags InFlags = EDepthStencilViewFlags::None)
    {
        FRHIDepthStencilViewDesc Desc;
        Desc.ViewDimension                  = EViewDimension::Texture2DArray;
        Desc.Flags                          = InFlags;
        Desc.Texture2DArray.Format          = InFormat;
        Desc.Texture2DArray.MipLevel        = InMipLevel;
        Desc.Texture2DArray.FirstArraySlice = InFirstArraySlice;
        Desc.Texture2DArray.NumSlices       = InNumSlices;
        return Desc;
    }

    NODISCARD static FRHIDepthStencilViewDesc CreateTextureCube(EFormat InFormat, uint8 InMipLevel,
        EDepthStencilViewFlags InFlags = EDepthStencilViewFlags::None)
    {
        FRHIDepthStencilViewDesc Desc;
        Desc.ViewDimension        = EViewDimension::TextureCube;
        Desc.Flags                = InFlags;
        Desc.TextureCube.Format   = InFormat;
        Desc.TextureCube.MipLevel = InMipLevel;
        return Desc;
    }

    NODISCARD static FRHIDepthStencilViewDesc CreateTextureCubeArray(EFormat InFormat, uint8 InMipLevel, uint16 InFirstCube, uint16 InNumCubes,
        EDepthStencilViewFlags InFlags = EDepthStencilViewFlags::None)
    {
        FRHIDepthStencilViewDesc Desc;
        Desc.ViewDimension              = EViewDimension::TextureCubeArray;
        Desc.Flags                      = InFlags;
        Desc.TextureCubeArray.Format    = InFormat;
        Desc.TextureCubeArray.MipLevel  = InMipLevel;
        Desc.TextureCubeArray.FirstCube = InFirstCube;
        Desc.TextureCubeArray.NumCubes  = InNumCubes;
        return Desc;
    }

public:
    FRHIDepthStencilViewDesc() noexcept
        : ViewDimension(EViewDimension::None)
        , Flags(EDepthStencilViewFlags::None)
        , Texture2D{}
    {
    }

    NODISCARD FORCEINLINE EFormat GetFormat() const noexcept
    {
        switch (ViewDimension)
        {
            case EViewDimension::Texture1D:
                return Texture1D.Format;
            case EViewDimension::Texture1DArray:
                return Texture1DArray.Format;
            case EViewDimension::Texture2D:
                return Texture2D.Format;
            case EViewDimension::Texture2DArray:
                return Texture2DArray.Format;
            case EViewDimension::TextureCube:
                return TextureCube.Format;
            case EViewDimension::TextureCubeArray:
                return TextureCubeArray.Format;
            default:
                return EFormat::Unknown;
        }
    }

    NODISCARD bool operator==(const FRHIDepthStencilViewDesc& Other) const noexcept
    {
        if (ViewDimension != Other.ViewDimension || Flags != Other.Flags)
        {
            return false;
        }

        switch (ViewDimension)
        {
            case EViewDimension::Texture1D:
                return Texture1D == Other.Texture1D;
            case EViewDimension::Texture1DArray:
                return Texture1DArray == Other.Texture1DArray;
            case EViewDimension::Texture2D:
                return Texture2D == Other.Texture2D;
            case EViewDimension::Texture2DArray:
                return Texture2DArray == Other.Texture2DArray;
            case EViewDimension::TextureCube:
                return TextureCube == Other.TextureCube;
            case EViewDimension::TextureCubeArray:
                return TextureCubeArray == Other.TextureCubeArray;
            default:
                return true;
        }
    }

    NODISCARD FORCEINLINE bool HasStencilFormat() const noexcept
    {
        return IsStencilFormat(GetFormat());
    }

    NODISCARD FORCEINLINE bool IsDepthReadOnly() const noexcept
    {
        return IsEnumFlagSet(Flags, EDepthStencilViewFlags::ReadOnlyDepth);
    }

    NODISCARD FORCEINLINE bool IsStencilReadOnly() const noexcept
    {
        return IsEnumFlagSet(Flags, EDepthStencilViewFlags::ReadOnlyStencil);
    }

    NODISCARD FORCEINLINE bool IsReadOnly() const noexcept
    {
        return IsDepthReadOnly() && (!HasStencilFormat() || IsStencilReadOnly());
    }

    EViewDimension         ViewDimension;
    EDepthStencilViewFlags Flags;

    union
    {
        FTexture1DDSV        Texture1D;
        FTexture1DArrayDSV   Texture1DArray;
        FTexture2DDSV        Texture2D;
        FTexture2DArrayDSV   Texture2DArray;
        FTextureCubeDSV      TextureCube;
        FTextureCubeArrayDSV TextureCubeArray;
    };
};

template<>
struct THash<FRHIDepthStencilViewDesc::FTexture1DDSV>
{
    NODISCARD static uint64 GetHash(const FRHIDepthStencilViewDesc::FTexture1DDSV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        return Result;
    }
};

template<>
struct THash<FRHIDepthStencilViewDesc::FTexture1DArrayDSV>
{
    NODISCARD static uint64 GetHash(const FRHIDepthStencilViewDesc::FTexture1DArrayDSV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.FirstArraySlice);
        HashCombine(Result, Value.NumSlices);
        return Result;
    }
};

template<>
struct THash<FRHIDepthStencilViewDesc::FTexture2DDSV>
{
    NODISCARD static uint64 GetHash(const FRHIDepthStencilViewDesc::FTexture2DDSV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        return Result;
    }
};

template<>
struct THash<FRHIDepthStencilViewDesc::FTexture2DArrayDSV>
{
    NODISCARD static uint64 GetHash(const FRHIDepthStencilViewDesc::FTexture2DArrayDSV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.FirstArraySlice);
        HashCombine(Result, Value.NumSlices);
        return Result;
    }
};

template<>
struct THash<FRHIDepthStencilViewDesc::FTextureCubeDSV>
{
    NODISCARD static uint64 GetHash(const FRHIDepthStencilViewDesc::FTextureCubeDSV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        return Result;
    }
};

template<>
struct THash<FRHIDepthStencilViewDesc::FTextureCubeArrayDSV>
{
    NODISCARD static uint64 GetHash(const FRHIDepthStencilViewDesc::FTextureCubeArrayDSV& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.Format);
        HashCombine(Result, Value.MipLevel);
        HashCombine(Result, Value.FirstCube);
        HashCombine(Result, Value.NumCubes);
        return Result;
    }
};

template<>
struct THash<FRHIDepthStencilViewDesc>
{
    NODISCARD static uint64 GetHash(const FRHIDepthStencilViewDesc& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.ViewDimension);
        HashCombine(Result, UnderlyingTypeValue(Value.Flags));

        switch (Value.ViewDimension)
        {
            case EViewDimension::Texture1D:
                HashCombine(Result, THash<FRHIDepthStencilViewDesc::FTexture1DDSV>::GetHash(Value.Texture1D));
                break;
            case EViewDimension::Texture1DArray:
                HashCombine(Result, THash<FRHIDepthStencilViewDesc::FTexture1DArrayDSV>::GetHash(Value.Texture1DArray));
                break;
            case EViewDimension::Texture2D:
                HashCombine(Result, THash<FRHIDepthStencilViewDesc::FTexture2DDSV>::GetHash(Value.Texture2D));
                break;
            case EViewDimension::Texture2DArray:
                HashCombine(Result, THash<FRHIDepthStencilViewDesc::FTexture2DArrayDSV>::GetHash(Value.Texture2DArray));
                break;
            case EViewDimension::TextureCube:
                HashCombine(Result, THash<FRHIDepthStencilViewDesc::FTextureCubeDSV>::GetHash(Value.TextureCube));
                break;
            case EViewDimension::TextureCubeArray:
                HashCombine(Result, THash<FRHIDepthStencilViewDesc::FTextureCubeArrayDSV>::GetHash(Value.TextureCubeArray));
                break;
            default:
                break;
        }

        return Result;
    }
};

class FRHIResourceView : public FRHIResource
{
protected:
    FRHIResourceView(ERHIResourceType InResourceType, FRHIResource* InResource)
        : FRHIResource(InResourceType)
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
    FRHIShaderResourceView(FRHIResource* InResource, const FRHIShaderResourceViewDesc& InDesc)
        : FRHIResourceView(ERHIResourceType::ShaderResourceView, InResource)
        , Desc(InDesc)
    {
    }

    virtual ~FRHIShaderResourceView() = default;

public:

    /** @return D3D12: D3D12_CPU_DESCRIPTOR_HANDLE::ptr. Vulkan: VkImageView / VkBufferView / VkAccelerationStructureKHR. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeHandle() const = 0;

    virtual FRHIDescriptorHandle GetBindlessHandle() const = 0;

    /** @brief Returns the descriptor used to create this view. */
    NODISCARD const FRHIShaderResourceViewDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHIShaderResourceViewDesc Desc;
};

class FRHIUnorderedAccessView : public FRHIResourceView
{
protected:
    FRHIUnorderedAccessView(FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InDesc)
        : FRHIResourceView(ERHIResourceType::UnorderedAccessView, InResource)
        , Desc(InDesc)
    {
    }

    virtual ~FRHIUnorderedAccessView() = default;

public:

    /** @return D3D12: D3D12_CPU_DESCRIPTOR_HANDLE::ptr. Vulkan: VkImageView / VkBufferView. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeHandle() const = 0;

    virtual FRHIDescriptorHandle GetBindlessHandle() const = 0;

    /** @brief Returns the descriptor used to create this view. */
    NODISCARD const FRHIUnorderedAccessViewDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHIUnorderedAccessViewDesc Desc;
};

class FRHIRenderTargetView : public FRHIResourceView
{
protected:
    FRHIRenderTargetView(FRHIResource* InResource, const FRHIRenderTargetViewDesc& InDesc)
        : FRHIResourceView(ERHIResourceType::RenderTargetView, InResource)
        , Desc(InDesc)
    {
    }

    virtual ~FRHIRenderTargetView() = default;

public:

    /** @return D3D12: D3D12_CPU_DESCRIPTOR_HANDLE::ptr. Vulkan: VkImageView. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeHandle() const = 0;

    /** @brief Returns the descriptor used to create this view. */
    NODISCARD const FRHIRenderTargetViewDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHIRenderTargetViewDesc Desc;
};

class FRHIDepthStencilView : public FRHIResourceView
{
protected:
    FRHIDepthStencilView(FRHIResource* InResource, const FRHIDepthStencilViewDesc& InDesc)
        : FRHIResourceView(ERHIResourceType::DepthStencilView, InResource)
        , Desc(InDesc)
    {
    }

    virtual ~FRHIDepthStencilView() = default;

public:

    /** @return D3D12: D3D12_CPU_DESCRIPTOR_HANDLE::ptr. Vulkan: VkImageView. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeHandle() const = 0;

    /** @brief Returns the descriptor used to create this view. */
    NODISCARD const FRHIDepthStencilViewDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHIDepthStencilViewDesc Desc;
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
        : ViewInstancingState()
        , DepthStencilAttachment()
        , RenderTargets(InRenderTargets)
        , NumRenderTargets(InNumRenderTargets)
        , StaticShadingRate(EShadingRate::VRS_1x1)
        , ShadingRateTexture(nullptr)
    {
    }

    FRHIBeginRenderPassDesc(const FRenderTargetAttachments& InRenderTargets, uint32 InNumRenderTargets, FRHIDepthStencilAttachment InDepthStencilAttachment,
        FRHITexture* InShadingRateTexture = nullptr, EShadingRate InStaticShadingRate = EShadingRate::VRS_1x1) noexcept
        : ViewInstancingState()
        , DepthStencilAttachment(InDepthStencilAttachment)
        , RenderTargets(InRenderTargets)
        , NumRenderTargets(InNumRenderTargets)
        , StaticShadingRate(InStaticShadingRate)
        , ShadingRateTexture(InShadingRateTexture)
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
