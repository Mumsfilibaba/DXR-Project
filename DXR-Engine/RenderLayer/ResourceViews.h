#pragma once
#include "Resources.h"

#include "Memory/Memory.h"

#include <Containers/StaticArray.h>

class ShaderResourceView : public Resource
{
};

class UnorderedAccessView : public Resource
{
};

class DepthStencilView : public Resource
{
};

class RenderTargetView : public Resource
{
};

using DepthStencilViewCube = TStaticArray<TSharedRef<DepthStencilView>, 6>;

struct ShaderResourceViewCreateInfo
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
        StructuredBuffer = 8,
    };

    struct Texture2DSRV
    {
        Texture2D* Texture = nullptr;
        EFormat Format     = EFormat::Unknown;
        UInt32  Mip        = 0;
        UInt32  NumMips    = 0;
        Float   MinMipBias = 0.0f;
    };

    struct Texture2DArraySRV
    {
        Texture2DArray* Texture = nullptr;
        EFormat Format          = EFormat::Unknown;
        UInt32  Mip             = 0;
        UInt32  NumMips         = 0;
        UInt32  ArraySlice      = 0;
        UInt32  NumArraySlices  = 0;
        Float   MinMipBias      = 0.0f;
    };

    struct TextureCubeSRV
    {
        TextureCube* Texture = nullptr;
        EFormat Format       = EFormat::Unknown;
        UInt32  Mip          = 0;
        UInt32  NumMips      = 0;
        Float   MinMipBias   = 0.0f;
    };

    struct TextureCubeArraySRV
    {
        TextureCubeArray* Texture = nullptr;
        EFormat Format            = EFormat::Unknown;
        UInt32  Mip               = 0;
        UInt32  NumMips           = 0;
        UInt32  ArraySlice        = 0;
        UInt32  NumArraySlices    = 0;
        Float   MinMipBias        = 0.0f;
    };

    struct Texture3DSRV
    {
        Texture3D* Texture     = nullptr;
        EFormat Format         = EFormat::Unknown;
        UInt32  Mip            = 0;
        UInt32  NumMips        = 0;
        UInt32  DepthSlice     = 0;
        UInt32  NumDepthSlices = 0;
        Float   MinMipBias     = 0.0f;
    };

    struct VertexBufferSRV
    {
        VertexBuffer* Buffer = nullptr;
        UInt32 FirstVertex   = 0;
        UInt32 NumVertices   = 0;
    };

    struct IndexBufferSRV
    {
        IndexBuffer* Buffer = nullptr;
        UInt32 FirstIndex   = 0;
        UInt32 NumIndices   = 0;
    };

    struct StructuredBufferSRV
    {
        StructuredBuffer* Buffer = nullptr;
        UInt32 FirstElement      = 0;
        UInt32 NumElements       = 0;
    };

    ShaderResourceViewCreateInfo(EType InType)
        : Type(InType)
    {
    }

    EType Type;
    union
    {
        Texture2DSRV        Texture2D;
        Texture2DArraySRV   Texture2DArray;
        TextureCubeSRV      TextureCube;
        TextureCubeArraySRV TextureCubeArray;
        Texture3DSRV        Texture3D;
        VertexBufferSRV     VertexBuffer;
        IndexBufferSRV      IndexBuffer;
        StructuredBufferSRV StructuredBuffer;
    };
};

struct UnorderedAccessViewCreateInfo
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
        StructuredBuffer = 8,
    };

    struct Texture2DUAV
    {
        Texture2D* Texture = nullptr;
        EFormat Format     = EFormat::Unknown;
        UInt32  Mip        = 0;
    };

    struct Texture2DArrayUAV
    {
        Texture2DArray* Texture = nullptr;
        EFormat Format          = EFormat::Unknown;
        UInt32  Mip             = 0;
        UInt32  ArraySlice      = 0;
        UInt32  NumArraySlices  = 0;
    };

    struct TextureCubeUAV
    {
        TextureCube* Texture = nullptr;
        EFormat Format       = EFormat::Unknown;
        UInt32  Mip          = 0;
    };

    struct TextureCubeArrayUAV
    {
        TextureCubeArray* Texture = nullptr;
        EFormat Format            = EFormat::Unknown;
        UInt32  Mip               = 0;
        UInt32  ArraySlice        = 0;
        UInt32  NumArraySlices    = 0;
    };

    struct Texture3DUAV
    {
        Texture3D* Texture     = nullptr;
        EFormat Format         = EFormat::Unknown;
        UInt32  Mip            = 0;
        UInt32  DepthSlice     = 0;
        UInt32  NumDepthSlices = 0;
    };

    struct VertexBufferUAV
    {
        VertexBuffer* Buffer = nullptr;
        UInt32 FirstVertex   = 0;
        UInt32 NumVertices   = 0;
    };

    struct IndexBufferUAV
    {
        IndexBuffer* Buffer = nullptr;
        UInt32 FirstIndex   = 0;
        UInt32 NumIndices   = 0;
    };

    struct StructuredBufferUAV
    {
        StructuredBuffer* Buffer = nullptr;
        UInt32 FirstElement      = 0;
        UInt32 NumElements       = 0;
    };

    UnorderedAccessViewCreateInfo(EType InType)
        : Type(InType)
    {
    }

    EType Type;
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

struct RenderTargetViewCreateInfo
{
    // TODO: Add support for texelbuffers?
    enum class EType
    {
        Texture2D        = 1,
        Texture2DArray   = 2,
        TextureCube      = 3,
        TextureCubeArray = 4,
        Texture3D        = 5,
    };

    struct Texture2DRTV
    {
        Texture2D* Texture = nullptr;
        UInt32 Mip         = 0;
    };

    struct Texture2DArrayRTV
    {
        Texture2DArray* Texture = nullptr;
        UInt32 Mip              = 0;
        UInt32 ArraySlice       = 0;
        UInt32 NumArraySlices   = 0;
    };

    struct TextureCubeRTV
    {
        TextureCube* Texture = nullptr;
        ECubeFace CubeFace   = ECubeFace::PosX;
        UInt32    Mip        = 0;
    };

    struct TextureCubeArrayRTV
    {
        TextureCubeArray* Texture = nullptr;
        ECubeFace CubeFace        = ECubeFace::PosX;
        UInt32    Mip             = 0;
        UInt32    ArraySlice      = 0;
    };

    struct Texture3DRTV
    {
        Texture3D* Texture     = nullptr;
        UInt32 Mip             = 0;
        UInt32 DepthSlice      = 0;
        UInt32 NumDepthSlices  = 0;
    };

    RenderTargetViewCreateInfo(EType InType)
        : Type(InType)
    {
    }

    EType   Type;
    EFormat Format = EFormat::Unknown;
    union
    {
        Texture2DRTV        Texture2D;
        Texture2DArrayRTV   Texture2DArray;
        TextureCubeRTV      TextureCube;
        TextureCubeArrayRTV TextureCubeArray;
        Texture3DRTV        Texture3D;
    };
};

struct DepthStencilViewCreateInfo
{
    enum class EType
    {
        Texture2D        = 1,
        Texture2DArray   = 2,
        TextureCube      = 3,
        TextureCubeArray = 4,
    };

    struct Texture2DDSV
    {
        Texture2D* Texture = nullptr;
        UInt32 Mip         = 0;
    };

    struct Texture2DArrayDSV
    {
        Texture2DArray* Texture = nullptr;
        UInt32 Mip              = 0;
        UInt32 ArraySlice       = 0;
        UInt32 NumArraySlices   = 0;
    };

    struct TextureCubeDSV
    {
        TextureCube* Texture = nullptr;
        ECubeFace CubeFace   = ECubeFace::PosX;
        UInt32    Mip        = 0;
    };

    struct TextureCubeArrayDSV
    {
        TextureCubeArray* Texture = nullptr;
        ECubeFace CubeFace        = ECubeFace::PosX;
        UInt32    Mip             = 0;
        UInt32    ArraySlice      = 0;
    };

    DepthStencilViewCreateInfo(EType InType)
        : Type(InType)
    {
    }

    EType   Type;
    EFormat Format = EFormat::Unknown;
    union
    {
        Texture2DDSV        Texture2D;
        Texture2DArrayDSV   Texture2DArray;
        TextureCubeDSV      TextureCube;
        TextureCubeArrayDSV TextureCubeArray;
    };
};