#pragma once
#include "RHIResources.h"

#include "Core/Memory/Memory.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/StaticArray.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIShaderResourceViewDesc

struct SRHIShaderResourceViewDesc
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

    FORCEINLINE SRHIShaderResourceViewDesc(EType InType)
        : Type(InType)
    {
    }

    EType Type;
    union
    {
        struct
        {
            CRHITexture2D* Texture = nullptr;
            ERHIFormat Format = ERHIFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            float   MinMipBias = 0.0f;
        } Texture2D;
        struct
        {
            CRHITexture2DArray* Texture = nullptr;
            ERHIFormat Format = ERHIFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
            float   MinMipBias = 0.0f;
        } Texture2DArray;
        struct
        {
            CRHITextureCube* Texture = nullptr;
            ERHIFormat Format = ERHIFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            float   MinMipBias = 0.0f;
        } TextureCube;
        struct
        {
            CRHITextureCubeArray* Texture = nullptr;
            ERHIFormat Format = ERHIFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
            float   MinMipBias = 0.0f;
        } TextureCubeArray;
        struct
        {
            CRHITexture3D* Texture = nullptr;
            ERHIFormat Format = ERHIFormat::Unknown;
            uint32  Mip = 0;
            uint32  NumMips = 0;
            uint32  DepthSlice = 0;
            uint32  NumDepthSlices = 0;
            float   MinMipBias = 0.0f;
        } Texture3D;
        struct
        {
            CRHIBuffer* Buffer = nullptr;
            uint32 FirstVertex = 0;
            uint32 NumVertices = 0;
        } VertexBuffer;
        struct
        {
            CRHIBuffer* Buffer = nullptr;
            uint32 FirstIndex = 0;
            uint32 NumIndices = 0;
        } IndexBuffer;
        struct
        {
            CRHIBuffer* Buffer = nullptr;
            uint32 FirstElement = 0;
            uint32 NumElements = 0;
        } StructuredBuffer;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIUnorderedAccessViewDesc

struct SRHIUnorderedAccessViewDesc
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

    FORCEINLINE SRHIUnorderedAccessViewDesc(EType InType)
        : Type(InType)
    {
    }

    EType Type;
    union
    {
        struct
        {
            CRHITexture2D* Texture = nullptr;
            ERHIFormat Format = ERHIFormat::Unknown;
            uint32  Mip = 0;
        } Texture2D;
        struct
        {
            CRHITexture2DArray* Texture = nullptr;
            ERHIFormat Format = ERHIFormat::Unknown;
            uint32  Mip = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
        } Texture2DArray;
        struct
        {
            CRHITextureCube* Texture = nullptr;
            ERHIFormat Format = ERHIFormat::Unknown;
            uint32  Mip = 0;
        } TextureCube;
        struct
        {
            CRHITextureCubeArray* Texture = nullptr;
            ERHIFormat Format = ERHIFormat::Unknown;
            uint32  Mip = 0;
            uint32  ArraySlice = 0;
            uint32  NumArraySlices = 0;
        } TextureCubeArray;
        struct
        {
            CRHITexture3D* Texture = nullptr;
            ERHIFormat Format = ERHIFormat::Unknown;
            uint32  Mip = 0;
            uint32  DepthSlice = 0;
            uint32  NumDepthSlices = 0;
        } Texture3D;
        struct
        {
            CRHIBuffer* Buffer = nullptr;
            uint32 FirstVertex = 0;
            uint32 NumVertices = 0;
        } VertexBuffer;
        struct
        {
            CRHIBuffer* Buffer = nullptr;
            uint32 FirstIndex = 0;
            uint32 NumIndices = 0;
        } IndexBuffer;
        struct
        {
            CRHIBuffer* Buffer = nullptr;
            uint32 FirstElement = 0;
            uint32 NumElements = 0;
        } StructuredBuffer;
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIRenderTargetViewDesc

struct SRHIRenderTargetViewDesc
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

    FORCEINLINE SRHIRenderTargetViewDesc(EType InType)
        : Type(InType)
    {
    }

    EType   Type;
    ERHIFormat Format = ERHIFormat::Unknown;
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
            ERHICubeFace CubeFace = ERHICubeFace::PosX;
            uint32    Mip = 0;
        } TextureCube;
        struct
        {
            CRHITextureCubeArray* Texture = nullptr;
            ERHICubeFace CubeFace = ERHICubeFace::PosX;
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
// SRHIDepthStencilViewDesc

struct SRHIDepthStencilViewDesc
{
    enum class EType
    {
        Texture2D = 1,
        Texture2DArray = 2,
        TextureCube = 3,
        TextureCubeArray = 4,
    };

    FORCEINLINE SRHIDepthStencilViewDesc(EType InType)
        : Type(InType)
    {
    }

    EType   Type;
    ERHIFormat Format = ERHIFormat::Unknown;
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
            ERHICubeFace CubeFace = ERHICubeFace::PosX;
            uint32    Mip = 0;
        } TextureCube;

        struct
        {
            CRHITextureCubeArray* Texture = nullptr;
            ERHICubeFace CubeFace = ERHICubeFace::PosX;
            uint32    Mip = 0;
            uint32    ArraySlice = 0;
        } TextureCubeArray;
    };
};


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderResourceView

class CRHIShaderResourceView : public CRHIObject
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIUnorderedAccessView

class CRHIUnorderedAccessView : public CRHIObject
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDepthStencilView

using DepthStencilViewCube = TStaticArray<TSharedRef<CRHIDepthStencilView>, 6>;

class CRHIDepthStencilView : public CRHIObject
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRenderTargetView

class CRHIRenderTargetView : public CRHIObject
{
};