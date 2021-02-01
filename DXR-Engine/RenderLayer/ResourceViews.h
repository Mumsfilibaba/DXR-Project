#pragma once
#include "Resources.h"

#include "Memory/Memory.h"

struct ShaderResourceViewCreateInfo
{
    enum class EShaderResourceViewType
    {
        Texture2D        = 1,
        Texture2DArray   = 2,
        TextureCube      = 3,
        TextureCubeArray = 4,
        Texture3D        = 5,
        VertexBuffer     = 6,
        IndexBuffer      = 7,
        StructuredBuffer = 8,
    };

    struct Texture2DSRV
    {
        Texture2DSRV() = default;

        Texture2DSRV(Texture2D* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InNumMipLevels, Float InMinMipBias)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , NumMipLevels(InNumMipLevels)
            , MinMipBias(InMinMipBias)
        {
        }

        Texture2D* Texture   = nullptr;
        EFormat Format       = EFormat::Unknown;
        UInt32  MipLevel     = 0;
        UInt32  NumMipLevels = 0;
        Float   MinMipBias   = 0.0f;
    };

    struct Texture2DArraySRV
    {
        Texture2DArraySRV() = default;

        Texture2DArraySRV(Texture2DArray* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InNumMipLevels, UInt32 InArraySlice, UInt32 InNumArraySlices, Float InMinMipBias)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , NumMipLevels(InNumMipLevels)
            , ArraySlice(InArraySlice)
            , NumArraySlices(InNumArraySlices)
            , MinMipBias(InMinMipBias)
        {
        }

        Texture2DArray* Texture = nullptr;
        EFormat Format          = EFormat::Unknown;
        UInt32  MipLevel        = 0;
        UInt32  NumMipLevels    = 0;
        UInt32  ArraySlice      = 0;
        UInt32  NumArraySlices  = 0;
        Float   MinMipBias      = 0.0f;
    };

    struct TextureCubeSRV
    {
        TextureCubeSRV() = default;

        TextureCubeSRV(TextureCube* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InNumMipLevels, Float InMinMipBias)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , NumMipLevels(InNumMipLevels)
            , MinMipBias(InMinMipBias)
        {
        }

        TextureCube* Texture = nullptr;
        EFormat Format       = EFormat::Unknown;
        UInt32  MipLevel     = 0;
        UInt32  NumMipLevels = 0;
        Float   MinMipBias   = 0.0f;
    };

    struct TextureCubeArraySRV
    {
        TextureCubeArraySRV() = default;

        TextureCubeArraySRV(TextureCubeArray* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InNumMipLevels, UInt32 InArraySlice, UInt32 InNumArraySlices, Float InMinMipBias)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , NumMipLevels(InNumMipLevels)
            , ArraySlice(InArraySlice)
            , NumArraySlices(InNumArraySlices)
            , MinMipBias(InMinMipBias)
        {
        }

        TextureCubeArray* Texture = nullptr;
        EFormat Format            = EFormat::Unknown;
        UInt32  MipLevel          = 0;
        UInt32  NumMipLevels      = 0;
        UInt32  ArraySlice        = 0;
        UInt32  NumArraySlices    = 0;
        Float   MinMipBias        = 0.0f;
    };

    struct Texture3DSRV
    {
        Texture3DSRV() = default;

        Texture3DSRV(Texture3D* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InNumMipLevels, UInt32 InDepthSlice, UInt32 InNumDepthSlices, Float InMinMipBias)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , NumMipLevels(InNumMipLevels)
            , DepthSlice(InDepthSlice)
            , NumDepthSlices(InNumDepthSlices)
            , MinMipBias(InMinMipBias)
        {
        }

        Texture3D* Texture     = nullptr;
        EFormat Format         = EFormat::Unknown;
        UInt32  MipLevel       = 0;
        UInt32  NumMipLevels   = 0;
        UInt32  DepthSlice     = 0;
        UInt32  NumDepthSlices = 0;
        Float   MinMipBias     = 0.0f;
    };

    struct VertexBufferSRV
    {
        VertexBufferSRV() = default;

        VertexBufferSRV(VertexBuffer* InBuffer, UInt32 InFirstVertex, UInt32 InNumVertices)
            : Buffer(InBuffer)
            , FirstVertex(InFirstVertex)
            , NumVertices(InNumVertices)
        {
        }

        VertexBuffer* Buffer = nullptr;
        UInt32 FirstVertex   = 0;
        UInt32 NumVertices   = 0;
    };

    struct IndexBufferSRV
    {
        IndexBufferSRV() = default;

        IndexBufferSRV(IndexBuffer* InBuffer, UInt32 InFirstIndex, UInt32 InNumIndices)
            : Buffer(InBuffer)
            , FirstIndex(InFirstIndex)
            , NumIndices(InNumIndices)
        {
        }

        IndexBuffer* Buffer = nullptr;
        UInt32 FirstIndex   = 0;
        UInt32 NumIndices   = 0;
    };

    struct StructuredBufferSRV
    {
        StructuredBufferSRV() = default;

        StructuredBufferSRV(StructuredBuffer* InBuffer, UInt32 InFirstElement, UInt32 InNumElements)
            : Buffer(InBuffer)
            , FirstElement(InFirstElement)
            , NumElements(InNumElements)
        {
        }

        StructuredBuffer* Buffer = nullptr;
        UInt32 FirstElement = 0;
        UInt32 NumElements  = 0;
    };

    ShaderResourceViewCreateInfo(Texture2D* Texture, EFormat Format, UInt32 MipLevel, UInt32 NumMipLevels, Float MinMipBias)
        : Type(EShaderResourceViewType::Texture2D)
        , Texure2D(Texture, Format, MipLevel, NumMipLevels, MinMipBias)
    {
    }

    ShaderResourceViewCreateInfo(Texture2DArray* Texture, EFormat Format, UInt32 MipLevel, UInt32 NumMipLevels, UInt32 ArraySlice, UInt32 NumArraySlices, Float MinMipBias)
        : Type(EShaderResourceViewType::Texture2DArray)
        , Texure2DArray(Texture, Format, MipLevel, NumMipLevels, ArraySlice, NumArraySlices, MinMipBias)
    {
    }

    ShaderResourceViewCreateInfo(TextureCube* Texture, EFormat Format, UInt32 MipLevel, UInt32 NumMipLevels, Float MinMipBias)
        : Type(EShaderResourceViewType::TextureCube)
        , TexureCube(Texture, Format, MipLevel, NumMipLevels, MinMipBias)
    {
    }

    ShaderResourceViewCreateInfo(TextureCubeArray* Texture, EFormat Format, UInt32 MipLevel, UInt32 NumMipLevels, UInt32 ArraySlice, UInt32 NumArraySlices, Float MinMipBias)
        : Type(EShaderResourceViewType::TextureCubeArray)
        , TexureCubeArray(Texture, Format, MipLevel, NumMipLevels, ArraySlice, NumArraySlices, MinMipBias)
    {
    }

    ShaderResourceViewCreateInfo(Texture3D* Texture, EFormat Format, UInt32 MipLevel, UInt32 NumMipLevels, UInt32 DepthSlice, UInt32 NumDepthSlices, Float MinMipBias)
        : Type(EShaderResourceViewType::Texture3D)
        , Texure3D(Texture, Format, MipLevel, NumMipLevels, DepthSlice, NumDepthSlices, MinMipBias)
    {
    }

    ShaderResourceViewCreateInfo(VertexBuffer* InVertexBuffer, UInt32 FirstVertex, UInt32 NumVertices)
        : Type(EShaderResourceViewType::VertexBuffer)
        , VertexBuffer(InVertexBuffer, FirstVertex, NumVertices)
    {
    }

    ShaderResourceViewCreateInfo(IndexBuffer* InIndexBuffer, UInt32 FirstIndex, UInt32 NumIndices)
        : Type(EShaderResourceViewType::IndexBuffer)
        , IndexBuffer(InIndexBuffer, FirstIndex, NumIndices)
    {
    }

    ShaderResourceViewCreateInfo(StructuredBuffer* InStructuredBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EShaderResourceViewType::StructuredBuffer)
        , StructuredBuffer(InStructuredBuffer, FirstElement, NumElements)
    {
    }

    ShaderResourceViewCreateInfo(const ShaderResourceViewCreateInfo& Other)
        : Type(Other.Type)
        , Texure2D()
    {
        *this = Other;
    }

    Texture2DSRV& GetTexture2DSRV()
    {
        VALIDATE(Type == EShaderResourceViewType::Texture2D);
        return Texure2D;
    }

    Texture2DArraySRV& GetTexture2DArraySRV()
    {
        VALIDATE(Type == EShaderResourceViewType::Texture2DArray);
        return Texure2DArray;
    }

    TextureCubeSRV& GetTextureCubeSRV()
    {
        VALIDATE(Type == EShaderResourceViewType::TextureCube);
        return TexureCube;
    }

    TextureCubeArraySRV& GetTextureCubeArraySRV()
    {
        VALIDATE(Type == EShaderResourceViewType::TextureCubeArray);
        return TexureCubeArray;
    }

    Texture3DSRV& GetTexture3DSRV()
    {
        VALIDATE(Type == EShaderResourceViewType::Texture3D);
        return Texure3D;
    }

    VertexBufferSRV& GetVertexBufferSRV()
    {
        VALIDATE(Type == EShaderResourceViewType::VertexBuffer);
        return VertexBuffer;
    }

    IndexBufferSRV& GetIndexBufferSRV()
    {
        VALIDATE(Type == EShaderResourceViewType::IndexBuffer);
        return IndexBuffer;
    }

    StructuredBufferSRV& GetStructuredBufferSRV()
    {
        VALIDATE(Type == EShaderResourceViewType::StructuredBuffer);
        return StructuredBuffer;
    }

    EShaderResourceViewType GetType() const
    {
        return Type;
    }

    ShaderResourceViewCreateInfo& operator=(const ShaderResourceViewCreateInfo& Other)
    {
        Type = Other.Type;
        if (Type == EShaderResourceViewType::Texture2D)
        {
            Texure2D = Other.Texure2D;
        }
        else if (Type == EShaderResourceViewType::Texture2DArray)
        {
            Texure2DArray = Other.Texure2DArray;
        }
        else if (Type == EShaderResourceViewType::TextureCube)
        {
            TexureCube = Other.TexureCube;
        }
        else if (Type == EShaderResourceViewType::TextureCubeArray)
        {
            TexureCubeArray = Other.TexureCubeArray;
        }
        else if (Type == EShaderResourceViewType::Texture3D)
        {
            Texure3D = Other.Texure3D;
        }
        else if (Type == EShaderResourceViewType::VertexBuffer)
        {
            VertexBuffer = Other.VertexBuffer;
        }
        else if (Type == EShaderResourceViewType::IndexBuffer)
        {
            IndexBuffer = Other.IndexBuffer;
        }
        else if (Type == EShaderResourceViewType::StructuredBuffer)
        {
            StructuredBuffer = Other.StructuredBuffer;
        }

        return *this;
    }

private:
    EShaderResourceViewType Type;
    union
    {
        Texture2DSRV        Texure2D;
        Texture2DArraySRV   Texure2DArray;
        TextureCubeSRV      TexureCube;
        TextureCubeArraySRV TexureCubeArray;
        Texture3DSRV        Texure3D;
        VertexBufferSRV     VertexBuffer;
        IndexBufferSRV      IndexBuffer;
        StructuredBufferSRV StructuredBuffer;
    };
};

class ShaderResourceView : public Resource
{
};

struct UnorderedAccessViewCreateInfo
{
    enum class EUnorderedAccessViewType
    {
        Texture2D        = 1,
        Texture2DArray   = 2,
        TextureCube      = 3,
        TextureCubeArray = 4,
        Texture3D        = 5,
        VertexBuffer     = 6,
        IndexBuffer      = 7,
        StructuredBuffer = 8,
    };

    struct Texture2DUAV
    {
        Texture2DUAV() = default;

        Texture2DUAV(Texture2D* InTexture, EFormat InFormat, UInt32 InMipLevel)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
        {
        }

        Texture2D* Texture = nullptr;
        EFormat Format     = EFormat::Unknown;
        UInt32  MipLevel   = 0;
    };

    struct Texture2DArrayUAV
    {
        Texture2DArrayUAV() = default;

        Texture2DArrayUAV(Texture2DArray* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InArraySlice, UInt32 InNumArraySlices)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , ArraySlice(InArraySlice)
            , NumArraySlices(InNumArraySlices)
        {
        }

        Texture2DArray* Texture = nullptr;
        EFormat Format          = EFormat::Unknown;
        UInt32  MipLevel        = 0;
        UInt32  ArraySlice      = 0;
        UInt32  NumArraySlices  = 0;
    };

    struct TextureCubeUAV
    {
        TextureCubeUAV() = default;

        TextureCubeUAV(TextureCube* InTexture, EFormat InFormat, UInt32 InMipLevel)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
        {
        }

        TextureCube* Texture = nullptr;
        EFormat Format       = EFormat::Unknown;
        UInt32  MipLevel     = 0;
    };

    struct TextureCubeArrayUAV
    {
        TextureCubeArrayUAV() = default;

        TextureCubeArrayUAV(TextureCubeArray* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InArraySlice, UInt32 InNumArraySlices)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , ArraySlice(InArraySlice)
            , NumArraySlices(InNumArraySlices)
        {
        }

        TextureCubeArray* Texture = nullptr;
        EFormat Format            = EFormat::Unknown;
        UInt32  MipLevel          = 0;
        UInt32  ArraySlice        = 0;
        UInt32  NumArraySlices    = 0;
    };

    struct Texture3DUAV
    {
        Texture3DUAV() = default;

        Texture3DUAV(Texture3D* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InDepthSlice, UInt32 InNumDepthSlices)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , DepthSlice(InDepthSlice)
            , NumDepthSlices(InNumDepthSlices)
        {
        }

        Texture3D* Texture     = nullptr;
        EFormat Format         = EFormat::Unknown;
        UInt32  MipLevel       = 0;
        UInt32  DepthSlice     = 0;
        UInt32  NumDepthSlices = 0;
    };

    struct VertexBufferUAV
    {
        VertexBufferUAV() = default;

        VertexBufferUAV(VertexBuffer* InBuffer, UInt32 InFirstVertex, UInt32 InNumVertices)
            : Buffer(InBuffer)
            , FirstVertex(InFirstVertex)
            , NumVertices(InNumVertices)
        {
        }

        VertexBuffer* Buffer = nullptr;
        UInt32 FirstVertex = 0;
        UInt32 NumVertices = 0;
    };

    struct IndexBufferUAV
    {
        IndexBufferUAV() = default;

        IndexBufferUAV(IndexBuffer* InBuffer, UInt32 InFirstIndex, UInt32 InNumIndices)
            : Buffer(InBuffer)
            , FirstIndex(InFirstIndex)
            , NumIndices(InNumIndices)
        {
        }

        IndexBuffer* Buffer = nullptr;
        UInt32 FirstIndex = 0;
        UInt32 NumIndices = 0;
    };

    struct StructuredBufferUAV
    {
        StructuredBufferUAV() = default;

        StructuredBufferUAV(StructuredBuffer* InBuffer, UInt32 InFirstElement, UInt32 InNumElements)
            : Buffer(InBuffer)
            , FirstElement(InFirstElement)
            , NumElements(InNumElements)
        {
        }

        StructuredBuffer* Buffer = nullptr;
        UInt32 FirstElement = 0;
        UInt32 NumElements = 0;
    };

    UnorderedAccessViewCreateInfo(Texture2D* Texture, EFormat Format, UInt32 MipLevel)
        : Type(EUnorderedAccessViewType::Texture2D)
        , Texture2D(Texture, Format, MipLevel)
    {
    }

    UnorderedAccessViewCreateInfo(Texture2DArray* Texture, EFormat Format, UInt32 MipLevel, UInt32 ArraySlice, UInt32 NumArraySlices)
        : Type(EUnorderedAccessViewType::Texture2DArray)
        , Texture2DArray(Texture, Format, MipLevel, ArraySlice, NumArraySlices)
    {
    }

    UnorderedAccessViewCreateInfo(TextureCube* Texture, EFormat Format, UInt32 MipLevel)
        : Type(EUnorderedAccessViewType::TextureCube)
        , TextureCube(Texture, Format, MipLevel)
    {
    }

    UnorderedAccessViewCreateInfo(TextureCubeArray* Texture, EFormat Format, UInt32 MipLevel, UInt32 ArraySlice, UInt32 NumArraySlices)
        : Type(EUnorderedAccessViewType::TextureCubeArray)
        , TextureCubeArray(Texture, Format, MipLevel, ArraySlice, NumArraySlices)
    {
    }

    UnorderedAccessViewCreateInfo(Texture3D* Texture, EFormat Format, UInt32 MipLevel, UInt32 DepthSlice, UInt32 NumDepthSlices)
        : Type(EUnorderedAccessViewType::Texture3D)
        , Texture3D(Texture, Format, MipLevel, DepthSlice, NumDepthSlices)
    {
    }

    UnorderedAccessViewCreateInfo(VertexBuffer* InVertexBuffer, UInt32 FirstVertex, UInt32 NumVertices)
        : Type(EUnorderedAccessViewType::VertexBuffer)
        , VertexBuffer(InVertexBuffer, FirstVertex, NumVertices)
    {
    }

    UnorderedAccessViewCreateInfo(IndexBuffer* InIndexBuffer, UInt32 FirstIndex, UInt32 NumIndices)
        : Type(EUnorderedAccessViewType::IndexBuffer)
        , IndexBuffer(InIndexBuffer, FirstIndex, NumIndices)
    {
    }

    UnorderedAccessViewCreateInfo(StructuredBuffer* InStructuredBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EUnorderedAccessViewType::StructuredBuffer)
        , StructuredBuffer(InStructuredBuffer, FirstElement, NumElements)
    {
    }

    UnorderedAccessViewCreateInfo(const UnorderedAccessViewCreateInfo& Other)
        : Type(Other.Type)
        , Texture2D()
    {
        *this = Other;
    }

    Texture2DUAV& GetTexture2DUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::Texture2D);
        return Texture2D;
    }

    Texture2DArrayUAV& GetTexture2DArrayUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::Texture2DArray);
        return Texture2DArray;
    }

    TextureCubeUAV GetTextureCubeUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::TextureCube);
        return TextureCube;
    }

    TextureCubeArrayUAV& GetTextureCubeArrayUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::TextureCubeArray);
        return TextureCubeArray;
    }

    Texture3DUAV& GetTexture3DUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::Texture3D);
        return Texture3D;
    }

    VertexBufferUAV& GetVertexBufferUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::VertexBuffer);
        return VertexBuffer;
    }

    IndexBufferUAV& GetIndexBufferUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::IndexBuffer);
        return IndexBuffer;
    }

    StructuredBufferUAV& GetStructuredBufferUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::StructuredBuffer);
        return StructuredBuffer;
    }

    EUnorderedAccessViewType GetType() const
    {
        return Type;
    }

    UnorderedAccessViewCreateInfo& operator=(const UnorderedAccessViewCreateInfo& Other)
    {
        Type = Other.Type;
        if (Type == EUnorderedAccessViewType::Texture2D)
        {
            Texture2D = Other.Texture2D;
        }
        else if (Type == EUnorderedAccessViewType::Texture2DArray)
        {
            Texture2DArray = Other.Texture2DArray;
        }
        else if (Type == EUnorderedAccessViewType::TextureCube)
        {
            TextureCube = Other.TextureCube;
        }
        else if (Type == EUnorderedAccessViewType::TextureCubeArray)
        {
            TextureCubeArray = Other.TextureCubeArray;
        }
        else if (Type == EUnorderedAccessViewType::Texture3D)
        {
            Texture3D = Other.Texture3D;
        }
        else if (Type == EUnorderedAccessViewType::VertexBuffer)
        {
            VertexBuffer = Other.VertexBuffer;
        }
        else if (Type == EUnorderedAccessViewType::IndexBuffer)
        {
            IndexBuffer = Other.IndexBuffer;
        }
        else if (Type == EUnorderedAccessViewType::StructuredBuffer)
        {
            StructuredBuffer = Other.StructuredBuffer;
        }

        return *this;
    }

private:
    EUnorderedAccessViewType Type;
    union
    {
        Texture2DUAV        Texture2D;
        Texture2DArrayUAV   Texture2DArray;
        TextureCubeUAV      TextureCube;
        TextureCubeArrayUAV TextureCubeArray;
        Texture3DUAV        Texture3D;
        VertexBufferUAV     VertexBuffer;
        IndexBufferUAV      IndexBuffer;
        StructuredBufferUAV StructuredBuffer;
    };
};

class UnorderedAccessView : public Resource
{
};

struct DepthStencilViewCreateInfo
{
    enum class EDepthStencilViewType
    {
        Texture2D        = 1,
        Texture2DArray   = 2,
        TextureCube      = 3,
        TextureCubeArray = 4,
    };

    struct Texture2DDSV
    {
        Texture2DDSV() = default;

        Texture2DDSV(Texture2D* InTexture, EFormat InFormat, UInt32 InMipLevel)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
        {
        }

        Texture2D* Texture = nullptr;
        EFormat Format     = EFormat::Unknown;
        UInt32  MipLevel   = 0;
    };

    struct Texture2DArrayDSV
    {
        Texture2DArrayDSV() = default;

        Texture2DArrayDSV(Texture2DArray* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InArraySlice, UInt32 InNumArraySlices)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , ArraySlice(InArraySlice)
            , NumArraySlices(InNumArraySlices)
        {
        }

        Texture2DArray* Texture = nullptr;
        EFormat Format          = EFormat::Unknown;
        UInt32  MipLevel        = 0;
        UInt32  ArraySlice      = 0;
        UInt32  NumArraySlices  = 0;
    };

    struct TextureCubeDSV
    {
        TextureCubeDSV() = default;

        TextureCubeDSV(TextureCube* InTexture, EFormat InFormat, ECubeFace InCubeFace, UInt32 InMipLevel)
            : Texture(InTexture)
            , Format(InFormat)
            , CubeFace(InCubeFace)
            , MipLevel(InMipLevel)
        {
        }

        TextureCube* Texture = nullptr;
        EFormat   Format     = EFormat::Unknown;
        ECubeFace CubeFace   = ECubeFace::PosX;
        UInt32    MipLevel   = 0;
    };

    struct TextureCubeArrayDSV
    {
        TextureCubeArrayDSV() = default;

        TextureCubeArrayDSV(TextureCubeArray* InTexture, EFormat InFormat, ECubeFace InCubeFace, UInt32 InMipLevel, UInt32 InArraySlice, UInt32 InNumArraySlices)
            : Texture(InTexture)
            , Format(InFormat)
            , CubeFace(InCubeFace)
            , MipLevel(InMipLevel)
            , ArraySlice(InArraySlice)
            , NumArraySlices(InNumArraySlices)
        {
        }

        TextureCubeArray* Texture = nullptr;
        EFormat   Format          = EFormat::Unknown;
        ECubeFace CubeFace        = ECubeFace::PosX;
        UInt32    MipLevel        = 0;
        UInt32    ArraySlice      = 0;
        UInt32    NumArraySlices  = 0;
    };

    DepthStencilViewCreateInfo(Texture2D* Texture, EFormat Format, UInt32 MipLevel)
        : Type(EDepthStencilViewType::Texture2D)
        , Texture2D(Texture, Format, MipLevel)
    {
    }

    DepthStencilViewCreateInfo(Texture2DArray* Texture, EFormat Format, UInt32 MipLevel, UInt32 ArraySlice, UInt32 NumArraySlices)
        : Type(EDepthStencilViewType::Texture2DArray)
        , Texture2DArray(Texture, Format, MipLevel, ArraySlice, NumArraySlices)
    {
    }

    DepthStencilViewCreateInfo(TextureCube* Texture, EFormat Format, ECubeFace CubeFace, UInt32 MipLevel)
        : Type(EDepthStencilViewType::TextureCube)
        , TextureCube(Texture, Format, CubeFace, MipLevel)
    {
    }

    DepthStencilViewCreateInfo(TextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, UInt32 MipLevel, UInt32 ArraySlice, UInt32 NumArraySlices)
        : Type(EDepthStencilViewType::TextureCubeArray)
        , TextureCubeArray(Texture, Format, CubeFace, MipLevel, ArraySlice, NumArraySlices)
    {
    }

    DepthStencilViewCreateInfo(const DepthStencilViewCreateInfo& Other)
        : Type(Other.Type)
        , Texture2D()
    {
        *this = Other;
    }

    Texture2DDSV& GetTexture2DDSV()
    {
        VALIDATE(Type == EDepthStencilViewType::Texture2D);
        return Texture2D;
    }

    Texture2DArrayDSV& GetTexture2DArrayDSV()
    {
        VALIDATE(Type == EDepthStencilViewType::Texture2DArray);
        return Texture2DArray;
    }

    TextureCubeDSV GetTextureCubeDSV()
    {
        VALIDATE(Type == EDepthStencilViewType::TextureCube);
        return TextureCube;
    }

    TextureCubeArrayDSV& GetTextureCubeArrayDSV()
    {
        VALIDATE(Type == EDepthStencilViewType::TextureCubeArray);
        return TextureCubeArray;
    }

    EDepthStencilViewType GetType() const
    {
        return Type;
    }

    DepthStencilViewCreateInfo& operator=(const DepthStencilViewCreateInfo& Other)
    {
        Type = Other.Type;
        if (Type == EDepthStencilViewType::Texture2D)
        {
            Texture2D = Other.Texture2D;
        }
        else if (Type == EDepthStencilViewType::Texture2DArray)
        {
            Texture2DArray = Other.Texture2DArray;
        }
        else if (Type == EDepthStencilViewType::TextureCube)
        {
            TextureCube = Other.TextureCube;
        }
        else if (Type == EDepthStencilViewType::TextureCubeArray)
        {
            TextureCubeArray = Other.TextureCubeArray;
        }

        return *this;
    }

private:
    EDepthStencilViewType Type;
    union
    {
        Texture2DDSV        Texture2D;
        Texture2DArrayDSV   Texture2DArray;
        TextureCubeDSV      TextureCube;
        TextureCubeArrayDSV TextureCubeArray;
    };
};

class DepthStencilView : public Resource
{
};

// TODO: Add support for texelbuffers
struct RenderTargetViewCreateInfo
{
    enum class ERenderTargetViewType
    {
        Texture2D        = 1,
        Texture2DArray   = 2,
        TextureCube      = 3,
        TextureCubeArray = 4,
        Texture3D        = 5,
    };

    struct Texture2DRTV
    {
        Texture2DRTV() = default;

        Texture2DRTV(Texture2D* InTexture, EFormat InFormat, UInt32 InMipLevel)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
        {
        }

        Texture2D* Texture = nullptr;
        EFormat Format     = EFormat::Unknown;
        UInt32  MipLevel   = 0;
    };

    struct Texture2DArrayRTV
    {
        Texture2DArrayRTV() = default;

        Texture2DArrayRTV(Texture2DArray* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InArraySlice, UInt32 InNumArraySlices)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , ArraySlice(InArraySlice)
            , NumArraySlices(InNumArraySlices)
        {
        }

        Texture2DArray* Texture = nullptr;
        EFormat Format          = EFormat::Unknown;
        UInt32  MipLevel        = 0;
        UInt32  ArraySlice      = 0;
        UInt32  NumArraySlices  = 0;
    };

    struct TextureCubeRTV
    {
        TextureCubeRTV() = default;

        TextureCubeRTV(TextureCube* InTexture, EFormat InFormat, ECubeFace InCubeFace, UInt32 InMipLevel)
            : Texture(InTexture)
            , Format(InFormat)
            , CubeFace(InCubeFace)
            , MipLevel(InMipLevel)
        {
        }

        TextureCube* Texture = nullptr;
        EFormat   Format     = EFormat::Unknown;
        ECubeFace CubeFace   = ECubeFace::PosX;
        UInt32    MipLevel   = 0;
    };

    struct TextureCubeArrayRTV
    {
        TextureCubeArrayRTV() = default;

        TextureCubeArrayRTV(TextureCubeArray* InTexture, EFormat InFormat, ECubeFace InCubeFace, UInt32 InMipLevel, UInt32 InArraySlice)
            : Texture(InTexture)
            , Format(InFormat)
            , CubeFace(InCubeFace)
            , MipLevel(InMipLevel)
            , ArraySlice(InArraySlice)
        {
        }

        TextureCubeArray* Texture = nullptr;
        EFormat   Format          = EFormat::Unknown;
        ECubeFace CubeFace        = ECubeFace::PosX;
        UInt32    MipLevel        = 0;
        UInt32    ArraySlice      = 0;
    };

    struct Texture3DRTV
    {
        Texture3DRTV() = default;

        Texture3DRTV(Texture3D* InTexture, EFormat InFormat, UInt32 InMipLevel, UInt32 InDepthSlice, UInt32 InNumDepthSlices)
            : Texture(InTexture)
            , Format(InFormat)
            , MipLevel(InMipLevel)
            , DepthSlice(InDepthSlice)
            , NumDepthSlices(InNumDepthSlices)
        {
        }

        Texture3D* Texture     = nullptr;
        EFormat Format         = EFormat::Unknown;
        UInt32  MipLevel       = 0;
        UInt32  DepthSlice     = 0;
        UInt32  NumDepthSlices = 0;
    };

    RenderTargetViewCreateInfo(Texture2D* Texture, EFormat Format, UInt32 MipLevel)
        : Type(ERenderTargetViewType::Texture2D)
        , Texture2D(Texture, Format, MipLevel)
    {
    }

    RenderTargetViewCreateInfo(Texture2DArray* Texture, EFormat Format, UInt32 MipLevel, UInt32 ArraySlice, UInt32 NumArraySlices)
        : Type(ERenderTargetViewType::Texture2DArray)
        , Texture2DArray(Texture, Format, MipLevel, ArraySlice, NumArraySlices)
    {
    }

    RenderTargetViewCreateInfo(TextureCube* Texture, EFormat Format, ECubeFace CubeFace, UInt32 MipLevel)
        : Type(ERenderTargetViewType::TextureCube)
        , TextureCube(Texture, Format, CubeFace, MipLevel)
    {
    }

    RenderTargetViewCreateInfo(TextureCubeArray* Texture, EFormat Format, ECubeFace CubeFace, UInt32 MipLevel, UInt32 ArraySlice)
        : Type(ERenderTargetViewType::TextureCubeArray)
        , TextureCubeArray(Texture, Format, CubeFace, MipLevel, ArraySlice)
    {
    }

    RenderTargetViewCreateInfo(Texture3D* Texture, EFormat Format, UInt32 MipLevel, UInt32 DepthSlice, UInt32 NumDepthSlices)
        : Type(ERenderTargetViewType::Texture3D)
        , Texture3D(Texture, Format, MipLevel, DepthSlice, NumDepthSlices)
    {
    }

    RenderTargetViewCreateInfo(const RenderTargetViewCreateInfo& Other)
        : Type(Other.Type)
        , Texture2D()
    {
        *this = Other;
    }

    Texture2DRTV& GetTexture2DRTV()
    {
        VALIDATE(Type == ERenderTargetViewType::Texture2D);
        return Texture2D;
    }

    Texture2DArrayRTV& GetTexture2DArrayRTV()
    {
        VALIDATE(Type == ERenderTargetViewType::Texture2DArray);
        return Texture2DArray;
    }

    TextureCubeRTV GetTextureCubeRTV()
    {
        VALIDATE(Type == ERenderTargetViewType::TextureCube);
        return TextureCube;
    }

    TextureCubeArrayRTV& GetTextureCubeArrayRTV()
    {
        VALIDATE(Type == ERenderTargetViewType::TextureCubeArray);
        return TextureCubeArray;
    }

    Texture3DRTV& GetTexture3DRTV()
    {
        VALIDATE(Type == ERenderTargetViewType::Texture3D);
        return Texture3D;
    }

    ERenderTargetViewType GetType() const
    {
        return Type;
    }

    RenderTargetViewCreateInfo& operator=(const RenderTargetViewCreateInfo& Other)
    {
        Type = Other.Type;
        if (Type == ERenderTargetViewType::Texture2D)
        {
            Texture2D = Other.Texture2D;
        }
        else if (Type == ERenderTargetViewType::Texture2DArray)
        {
            Texture2DArray = Other.Texture2DArray;
        }
        else if (Type == ERenderTargetViewType::TextureCube)
        {
            TextureCube = Other.TextureCube;
        }
        else if (Type == ERenderTargetViewType::TextureCubeArray)
        {
            TextureCubeArray = Other.TextureCubeArray;
        }
        else if (Type == ERenderTargetViewType::Texture3D)
        {
            Texture3D = Other.Texture3D;
        }

        return *this;
    }

private:
    ERenderTargetViewType Type;
    union
    {
        Texture2DRTV        Texture2D;
        Texture2DArrayRTV   Texture2DArray;
        TextureCubeRTV      TextureCube;
        TextureCubeArrayRTV TextureCubeArray;
        Texture3DRTV        Texture3D;
    };
};

class RenderTargetView : public Resource
{
};