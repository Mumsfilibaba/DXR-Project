#pragma once
#include "Resources.h"

#include "Core/Memory/Memory.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"

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
        Texture2D = 1,
        Texture2DArray = 2,
        TextureCube = 3,
        TextureCubeArray = 4,
        Texture3D = 5,
        VertexBuffer = 6,
        IndexBuffer = 7,
        StructuredBuffer = 8,
    };

    ShaderResourceViewCreateInfo( EType InType )
        : Type( InType )
    {
    }

    EType Type;
    union
    {
        struct
        {
            Texture2D* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            float   MinMipBias = 0.0f;
        } Texture2D;
        struct
        {
            Texture2DArray* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
            float   MinMipBias = 0.0f;
        } Texture2DArray;
        struct
        {
            TextureCube* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            float   MinMipBias = 0.0f;
        } TextureCube;
        struct
        {
            TextureCubeArray* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
            float   MinMipBias = 0.0f;
        } TextureCubeArray;
        struct
        {
            Texture3D* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            uint32  DepthSlice = 0;
            uint32  NumDepthSlices = 0;
            float   MinMipBias = 0.0f;
        } Texture3D;
        struct
        {
            VertexBuffer* Buffer = nullptr;
            uint32 FirstVertex = 0;
            uint32 NumVertices = 0;
        } VertexBuffer;
        struct
        {
            IndexBuffer* Buffer = nullptr;
            uint32 FirstIndex = 0;
            uint32 NumIndices = 0;
        } IndexBuffer;
        struct
        {
            StructuredBuffer* Buffer = nullptr;
            uint32 FirstElement = 0;
            uint32 NumElements = 0;
        } StructuredBuffer;
    };
};

struct UnorderedAccessViewCreateInfo
{
    enum class EType
    {
        Texture2D = 1,
        Texture2DArray = 2,
        TextureCube = 3,
        TextureCubeArray = 4,
        Texture3D = 5,
        VertexBuffer = 6,
        IndexBuffer = 7,
        StructuredBuffer = 8,
    };

    UnorderedAccessViewCreateInfo( EType InType )
        : Type( InType )
    {
    }

    EType Type;
    union
    {
        struct
        {
            Texture2D* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
        } Texture2D;
        struct
        {
            Texture2DArray* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
        } Texture2DArray;
        struct
        {
            TextureCube* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
        } TextureCube;
        struct
        {
            TextureCubeArray* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
        } TextureCubeArray;
        struct
        {
            Texture3D* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  DepthSlice = 0;
            uint32  NumDepthSlices = 0;
        } Texture3D;
        struct
        {
            VertexBuffer* Buffer = nullptr;
            uint32 FirstVertex = 0;
            uint32 NumVertices = 0;
        } VertexBuffer;
        struct
        {
            IndexBuffer* Buffer = nullptr;
            uint32 FirstIndex = 0;
            uint32 NumIndices = 0;
        } IndexBuffer;
        struct
        {
            StructuredBuffer* Buffer = nullptr;
            uint32 FirstElement = 0;
            uint32 NumElements = 0;
        } StructuredBuffer;
    };
};

struct RenderTargetViewCreateInfo
{
    // TODO: Add support for texelbuffers?
    enum class EType
    {
        Texture2D = 1,
        Texture2DArray = 2,
        TextureCube = 3,
        TextureCubeArray = 4,
        Texture3D = 5,
    };

    RenderTargetViewCreateInfo( EType InType )
        : Type( InType )
    {
    }

    EType   Type;
    EFormat Format = EFormat::Unknown;
    union
    {
        struct
        {
            Texture2D* Texture = nullptr;
            uint32 Mip = 0;
        } Texture2D;
        struct
        {
            Texture2DArray* Texture = nullptr;
            uint32 Mip = 0;
            uint32 ArraySlice = 0;
            uint32 NumArraySlices = 0;
        } Texture2DArray;
        struct
        {
            TextureCube* Texture = nullptr;
            ECubeFace CubeFace = ECubeFace::PosX;
            uint32    Mip = 0;
        } TextureCube;
        struct
        {
            TextureCubeArray* Texture = nullptr;
            ECubeFace CubeFace = ECubeFace::PosX;
            uint32    Mip = 0;
            uint32    ArraySlice = 0;
        } TextureCubeArray;
        struct
        {
            Texture3D* Texture = nullptr;
            uint32 Mip = 0;
            uint32 DepthSlice = 0;
            uint32 NumDepthSlices = 0;
        } Texture3D;
    };
};

struct DepthStencilViewCreateInfo
{
    enum class EType
    {
        Texture2D = 1,
        Texture2DArray = 2,
        TextureCube = 3,
        TextureCubeArray = 4,
    };

    DepthStencilViewCreateInfo( EType InType )
        : Type( InType )
    {
    }

    EType   Type;
    EFormat Format = EFormat::Unknown;
    union
    {
        struct
        {
            Texture2D* Texture = nullptr;
            uint32 Mip = 0;
        } Texture2D;
        struct
        {
            Texture2DArray* Texture = nullptr;
            uint32 Mip = 0;
            uint32 ArraySlice = 0;
            uint32 NumArraySlices = 0;
        } Texture2DArray;
        struct
        {
            TextureCube* Texture = nullptr;
            ECubeFace CubeFace = ECubeFace::PosX;
            uint32    Mip = 0;
        } TextureCube;

        struct
        {
            TextureCubeArray* Texture = nullptr;
            ECubeFace CubeFace = ECubeFace::PosX;
            uint32    Mip = 0;
            uint32    ArraySlice = 0;
        } TextureCubeArray;
    };
};