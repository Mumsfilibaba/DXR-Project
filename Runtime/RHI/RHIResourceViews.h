#pragma once
#include "RHIResources.h"

#include "Core/Memory/Memory.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

using DepthStencilViewCube = TStaticArray<TSharedRef<CRHIDepthStencilView>, 6>;

///////////////////////////////////////////////////////////////////////////////////////////////////

struct SShaderResourceViewCreateInfo
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

    FORCEINLINE SShaderResourceViewCreateInfo( EType InType )
        : Type( InType )
    {
    }

    EType Type;
    union
    {
        struct
        {
            CRHITexture2D* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            float   MinMipBias = 0.0f;
        } Texture2D;
        struct
        {
            CRHITexture2DArray* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
            float   MinMipBias = 0.0f;
        } Texture2DArray;
        struct
        {
            CRHITextureCube* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            float   MinMipBias = 0.0f;
        } TextureCube;
        struct
        {
            CRHITextureCubeArray* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
            float   MinMipBias = 0.0f;
        } TextureCubeArray;
        struct
        {
            CRHITexture3D* Texture = nullptr;
            EFormat Format = EFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            uint32  DepthSlice = 0;
            uint32  NumDepthSlices = 0;
            float   MinMipBias = 0.0f;
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
            CRHIStructuredBuffer* Buffer = nullptr;
            uint32 FirstElement = 0;
            uint32 NumElements = 0;
        } StructuredBuffer;
    };
};

///////////////////////////////////////////////////////////////////////////////////////////////////

struct SUnorderedAccessViewCreateInfo
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

    FORCEINLINE SUnorderedAccessViewCreateInfo( EType InType )
        : Type( InType )
    {
    }

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
            CRHIStructuredBuffer* Buffer = nullptr;
            uint32 FirstElement = 0;
            uint32 NumElements = 0;
        } StructuredBuffer;
    };
};

///////////////////////////////////////////////////////////////////////////////////////////////////

struct SRenderTargetViewCreateInfo
{
    // TODO: Add support for texel buffers?
    enum class EType
    {
        Texture2D = 1,
        Texture2DArray = 2,
        TextureCube = 3,
        TextureCubeArray = 4,
        Texture3D = 5,
    };

    FORCEINLINE SRenderTargetViewCreateInfo( EType InType )
        : Type( InType )
    {
    }

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

///////////////////////////////////////////////////////////////////////////////////////////////////

struct SDepthStencilViewCreateInfo
{
    enum class EType
    {
        Texture2D = 1,
        Texture2DArray = 2,
        TextureCube = 3,
        TextureCubeArray = 4,
    };

    FORCEINLINE SDepthStencilViewCreateInfo( EType InType )
        : Type( InType )
    {
    }

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

///////////////////////////////////////////////////////////////////////////////////////////////////

class CRHIShaderResourceView : public CRHIResource
{
};

class CRHIUnorderedAccessView : public CRHIResource
{
};

class CRHIDepthStencilView : public CRHIResource
{
};

class CRHIRenderTargetView : public CRHIResource
{
};