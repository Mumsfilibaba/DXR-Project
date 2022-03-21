#pragma once
#include "RHITypes.h"
#include "RHIShader.h"
#include "RHIPipelineState.h"
#include "RHISamplerState.h"
#include "RHIRayTracing.h"
#include "RHITimestampQuery.h"

#include "Core/RefCounted.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"

class CRHIDescriptorHandle;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIObject>              CRHIObjectRef;
typedef TSharedRef<class CRHIResource>            CRHIResourceRef;

typedef TSharedRef<class CRHIBuffer>              CRHIBufferRef;
typedef TSharedRef<class CRHIConstantBuffer>      CRHIConstantBufferRef;
typedef TSharedRef<class CRHITexture>             CRHITextureRef;

typedef TSharedRef<class CRHIShaderResourceView>  CRHIShaderResourceViewRef;
typedef TSharedRef<class CRHIUnorderedAccessView> CRHIUnorderedAccessViewRef;
typedef TSharedRef<class CRHIRenderTargetView>    CRHIRenderTargetViewRef;
typedef TSharedRef<class CRHIDepthStencilView>    CRHIDepthStencilViewRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIResourceDimension

enum class ERHIResourceType : uint8
{
    Unknown                 = 0,
    Buffer                  = 1,
    ConstantBuffer          = 2,
    Texture                 = 3,

    ShaderResourceView      = 4,
    UnorderedAccessView     = 5,
    RenderTargetView        = 6,
    DepthStencilView        = 7,

    RayTracingGeometry      = 8,
    RayTracingScene         = 9,

    GraphicsPipelineState   = 10,
    ComputePipelineState    = 11,
    RayTracingPipelineState = 12,

    DepthStencilState       = 13,
    RasterizerState         = 14,
    VertexInputLayout       = 15
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIResource

class CRHIResource : public CRefCounted
{
public:

    CRHIResource(ERHIResourceType InResourceType)
        : CRefCounted()
        , Type(InResourceType)
    { }

    /**
     * Retrieve the type of the resource
     * 
     * @return: Returns the type of the resource
     */
    inline ERHIResourceType GetType() const 
    { 
        return Type;
    }

private:
    ERHIResourceType Type;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIIndexFormat 

enum class ERHIIndexFormat : uint8
{
    Unknown = 0,
    uint16  = 1,
    uint32  = 2,
};

inline const char* ToString(ERHIIndexFormat IndexFormat)
{
    switch (IndexFormat)
    {
    case ERHIIndexFormat::uint16: return "uint16";
    case ERHIIndexFormat::uint32: return "uint32";
    default:                      return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIIndexFormat helpers

inline ERHIIndexFormat GetIndexFormatFromStride(uint32 StrideInBytes)
{
    if (StrideInBytes == 2)
    {
        return ERHIIndexFormat::uint16;
    }
    else if (StrideInBytes == 4)
    {
        return ERHIIndexFormat::uint32;
    }
    else
    {
        return ERHIIndexFormat::Unknown;
    }
}

inline uint32 GetStrideFromIndexFormat(ERHIIndexFormat IndexFormat)
{
    if (IndexFormat == ERHIIndexFormat::uint16)
    {
        return 2;
    }
    else if (IndexFormat == ERHIIndexFormat::uint32)
    {
        return 4;
    }
    else
    {
        return 0;
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIBufferUsageFlags

enum ERHIBufferUsageFlags : uint16
{
    BufferUsageFlag_None                 = 0,

    BufferUsageFlag_Default              = FLAG(1), // Default Device Memory
    BufferUsageFlag_Readback             = FLAG(2), // CPU readable Memory
    BufferUsageFlag_Dynamic              = FLAG(3), // Dynamic Memory

    BufferUsageFlag_AllowVertexBuffer    = FLAG(5), // Can be used as vertex buffer
    BufferUsageFlag_AllowIndexBuffer     = FLAG(6), // Can be used as index buffer
    BufferUsageFlag_AllowUnorderedAccess = FLAG(7), // Can be used in UnorderedAccessViews
    BufferUsageFlag_AllowShaderResource  = FLAG(8), // Can be used in ShaderResourceViews

    BufferUsageFlags_RWBuffer = BufferUsageFlag_AllowUnorderedAccess | BufferUsageFlag_AllowShaderResource
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIBufferDesc

struct SRHIBufferDesc
{
    static SRHIBufferDesc CreateVertexBuffer(uint32 NumVertices, uint32 VertexStride, uint32 InFlags)
    {
        return SRHIBufferDesc(NumVertices * VertexStride, VertexStride, InFlags | BufferUsageFlag_AllowVertexBuffer);
    }

    static SRHIBufferDesc CreateIndexBuffer(ERHIIndexFormat IndexFormat, uint32 NumIndices, uint32 InFlags)
    {
        return SRHIBufferDesc(
            GetStrideFromIndexFormat(IndexFormat) * NumIndices,
            GetStrideFromIndexFormat(IndexFormat), 
            InFlags | BufferUsageFlag_AllowIndexBuffer);
    }

    static SRHIBufferDesc CreateStructured(uint32 NumElements, uint32 Stride, uint32 InFlags)
    {
        return SRHIBufferDesc(NumElements * Stride, Stride, InFlags);
    }

    static SRHIBufferDesc CreateSRV(uint32 NumElements, uint32 Stride, uint32 InFlags)
    {
        return SRHIBufferDesc(NumElements * Stride, Stride, InFlags | BufferUsageFlag_AllowUnorderedAccess);
    }

    static SRHIBufferDesc CreateUAV(uint32 NumElements, uint32 Stride, uint32 InFlags)
    {
        return SRHIBufferDesc(NumElements * Stride, Stride, InFlags | BufferUsageFlag_AllowShaderResource);
    }

    static SRHIBufferDesc CreateRWBuffer(uint32 NumElements, uint32 Stride, uint32 InFlags)
    {
        return SRHIBufferDesc(NumElements * Stride, Stride, InFlags | BufferUsageFlags_RWBuffer);
    }

    SRHIBufferDesc()
        : Size(0)
        , ElementStride(0)
        , UsageFlags(0)
    { }

    SRHIBufferDesc(uint32 InSizeInBytes, uint32 InStrideInBytes, uint32 InFlags)
        : Size(InSizeInBytes)
        , ElementStride(InStrideInBytes)
        , UsageFlags(InFlags)
    { }

    bool IsUAV() const { return (UsageFlags & BufferUsageFlag_AllowUnorderedAccess); }
    bool IsSRV() const { return (UsageFlags & BufferUsageFlag_AllowShaderResource); }

    bool IsVertexBuffer() const { return (UsageFlags & BufferUsageFlag_AllowVertexBuffer); }
    bool IsIndexBuffer()  const { return (UsageFlags & BufferUsageFlag_AllowIndexBuffer); }

    bool IsRWBuffer() const { return (UsageFlags & BufferUsageFlags_RWBuffer); }
    bool IsDynamic()  const { return (UsageFlags & BufferUsageFlag_Dynamic); }
    bool IsReadBack() const { return (UsageFlags & BufferUsageFlag_Readback); }

    bool operator==(const SRHIBufferDesc& Rhs) const
    {
        return 
            (UsageFlags    == Rhs.UsageFlags) && 
            (Size          == Rhs.Size)       && 
            (ElementStride == Rhs.ElementStride);
    }

    bool operator!=(const SRHIBufferDesc& Rhs) const
    {
        return !(*this == Rhs);
    }

    uint32 Size          = 0;
    uint16 ElementStride = 0;
    uint16 UsageFlags    = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBuffer

class CRHIBuffer : public CRHIResource
{
public:

    CRHIBuffer(const SRHIBufferDesc& InBufferDesc)
        : CRHIResource(ERHIResourceType::Buffer)
        , BufferDesc(InBufferDesc)
    { }

    /**
     * Retrieve the Buffer Description
     * 
     * @return: Returns the Buffer Description
     */
    const SRHIBufferDesc& GetDesc() const 
    { 
        return BufferDesc; 
    }

protected:
    SRHIBufferDesc BufferDesc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIConstantBufferType

enum class ERHIConstantBufferType : uint8
{
    Default = 1,
    Dynamic = 2,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIConstantBuffer

class CRHIConstantBuffer : public CRHIResource
{
public:
    CRHIConstantBuffer(ERHIConstantBufferType InType, uint32 InSize)
        : CRHIResource(ERHIResourceType::ConstantBuffer)
        , Type(InType)
        , Size(InSize)
    { }

    /**
     * Retrieve the type of Constant-Buffer 
     * 
     * @return: Returns the type of Constant-Buffer 
     */
    ERHIConstantBufferType GetType() const 
    { 
        return Type;
    }

    /**
     * Retrieve the type of Constant-Buffer
     *
     * @return: Returns the type of Constant-Buffer
     */
    uint32 GetSize() const 
    { 
        return Size;
    }

private:
    ERHIConstantBufferType Type;
    uint32                 Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHITextureUsageFlags

enum ERHITextureUsageFlags : uint16
{
    TextureUsageFlag_None                 = 0,

    TextureUsageFlag_AllowRenderTarget    = FLAG(1), // RenderTargetView
    TextureUsageFlag_AllowDepthStencil    = FLAG(2), // DepthStencilView
    TextureUsageFlag_AllowUnorderedAccess = FLAG(3), // UnorderedAccessView
    TextureUsageFlag_AllowShaderResource  = FLAG(4), // ShaderResourceView

    TextureUsageFlag_NoDefaultRTV         = FLAG(5), // Do not create default RenderTargetView
    TextureUsageFlag_NoDefaultDSV         = FLAG(6), // Do not create default DepthStencilView
    TextureUsageFlag_NoDefaultUAV         = FLAG(7), // Do not create default UnorderedAccessView
    TextureUsageFlag_NoDefaultSRV         = FLAG(8), // Do not create default ShaderResourceView

    TextureUsageFlags_RWTexture    = TextureUsageFlag_AllowUnorderedAccess | TextureUsageFlag_AllowShaderResource,
    TextureUsageFlags_RenderTarget = TextureUsageFlag_AllowRenderTarget | TextureUsageFlag_AllowShaderResource,
    TextureUsageFlags_ShadowMap    = TextureUsageFlag_AllowDepthStencil | TextureUsageFlag_AllowShaderResource,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHITextureDimension

enum class ERHITextureDimension : uint8
{
    Unknown          = 0,
    Texture2D        = 1,
    Texture2DArray   = 2,
    TextureCube      = 3,
    TextureCubeArray = 4,
    Texture3D        = 5
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHITextureDesc

struct SRHITextureDesc
{
    static SRHITextureDesc CreateTexture2D(ERHIFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return SRHITextureDesc(ERHITextureDimension::Texture2D, InFormat, InWidth, InHeight, 1, InNumMips, InNumSamples, InFlags);
    }

    static SRHITextureDesc CreateTexture2DArray(
        ERHIFormat InFormat,
        uint32 InWidth, 
        uint32 InHeight, 
        uint32 InArraySize, 
        uint32 InNumMips, 
        uint32 InNumSamples, 
        uint32 InFlags)
    {
        return SRHITextureDesc(ERHITextureDimension::Texture2DArray, InFormat, InWidth, InHeight, InArraySize, InNumMips, InNumSamples, InFlags);
    }

    static SRHITextureDesc CreateTextureCube(ERHIFormat InFormat, uint32 InSize, uint32 InNumMips, uint32 InNumSamples, uint32 InFlags)
    {
        return SRHITextureDesc(ERHITextureDimension::TextureCube, InFormat, InSize, InSize, 1, InNumMips, InNumSamples, InFlags);
    }

    static SRHITextureDesc CreateTextureCubeArray(
        ERHIFormat InFormat, 
        uint32 InSize, 
        uint32 InArraySize, 
        uint32 InNumMips, 
        uint32 InNumSamples, 
        uint32 InFlags)
    {
        return SRHITextureDesc(ERHITextureDimension::TextureCubeArray, InFormat, InSize, InSize, InArraySize, InNumMips, InNumSamples, InFlags);
    }

    static SRHITextureDesc CreateTexture3D(
        ERHIFormat InFormat,
        uint32 InWidth, 
        uint32 InHeight, 
        uint32 InDepth, 
        uint32 InNumMips, 
        uint32 InNumSamples, 
        uint32 InFlags)
    {
        return SRHITextureDesc(ERHITextureDimension::Texture3D, InFormat, InWidth, InHeight, InDepth, InNumMips, InNumSamples, InFlags);
    }

    SRHITextureDesc() = default;

    SRHITextureDesc(
        ERHITextureDimension InDimension,
        ERHIFormat InFormat, 
        uint32 InWidth, 
        uint32 InHeight, 
        uint32 InDepthOrArraySize, 
        uint32 InNumMips, 
        uint32 InNumSamples, 
        uint32 InFlags)
        : Dimension(InDimension)
        , Format(InFormat)
        , Width(InWidth)
        , Height(InHeight)
        , DepthOrArraySize(InDepthOrArraySize)
        , NumMips(InNumMips)
        , NumSamples(InNumSamples)
        , UsageFlags(InFlags)
    { }

    bool IsTexture2D()        const { return Dimension == ERHITextureDimension::Texture2D; }
    bool IsTexture2DArray()   const { return Dimension == ERHITextureDimension::Texture2DArray; }
    bool IsTextureCube()      const { return Dimension == ERHITextureDimension::TextureCube; }
    bool IsTextureCubeArray() const { return Dimension == ERHITextureDimension::TextureCubeArray; }
    bool IsTexture3D()        const { return Dimension == ERHITextureDimension::Texture3D; }

    bool IsSRV() const { return (UsageFlags & TextureUsageFlag_AllowShaderResource); }
    bool IsUAV() const { return (UsageFlags & TextureUsageFlag_AllowUnorderedAccess); }
    bool IsRTV() const { return (UsageFlags & TextureUsageFlag_AllowRenderTarget); }
    bool IsDSV() const { return (UsageFlags & TextureUsageFlag_AllowDepthStencil); }
    
    bool IsRWTexture() const { return (UsageFlags & TextureUsageFlags_RWTexture); }

    bool operator==(const SRHITextureDesc& Rhs) const
    {
        return 
            (Dimension        == Rhs.Dimension)        &&
            (Format           == Rhs.Format)           &&
            (Width            == Rhs.Width)            &&
            (Height           == Rhs.Height)           &&
            (DepthOrArraySize == Rhs.DepthOrArraySize) && 
            (NumMips          == Rhs.NumMips)          &&
            (NumSamples       == Rhs.NumSamples)       &&
            (UsageFlags       == Rhs.UsageFlags)       &&
            (ClearValue       == Rhs.ClearValue);
    }

    bool operator!=(const SRHITextureDesc& Rhs) const
    {
        return !(*this == Rhs);
    }

    ERHITextureDimension Dimension        = ERHITextureDimension::Unknown;
    ERHIFormat           Format           = ERHIFormat::Unknown;
    uint16               Width            = 0;
    uint16               Height           = 0;
    uint16               DepthOrArraySize = 0;
    uint8                NumMips          = 0;
    uint8                NumSamples       = 0;
    uint16               UsageFlags       = 0;
    CRHIClearValue       ClearValue       = CRHIClearValue();
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture

class CRHITexture : public CRHIResource
{
public:

    CRHITexture(const SRHITextureDesc& InTextureDesc)
        : CRHIResource(ERHIResourceType::Texture)
        , TextureDesc(InTextureDesc)
    { }

    /**
     * Retrieve the default ShaderResourceView. Pointer is valid if the AllowShaderResouce flag is set.
     * 
     * @return: Returns a pointer to the default ShaderResourceView
     */
    virtual CRHIShaderResourceView* GetDefaultShaderResouceView() const { return nullptr; }

    /**
     * Retrieve the default ShaderResourceView as a bindless descriptor-handle. Pointer is valid if the AllowShaderResouce flag is set.
     *
     * @return: Returns a pointer to the default ShaderResourceView as a bindless descriptor-handle
     */
    virtual CRHIDescriptorHandle* GetDefaultBindlessHandle() const { return nullptr; }

    /**
     * Retrieve the Native handle of the Texture
     * 
     * @return: Returns the native handle of the resource
     */
    virtual void* GetNativeHandle() const { return nullptr; }

    /**
     * Set the name of the Texture
     * 
     * @param InName: New name of of the resource
     */
    virtual void SetName(const String& InName) { }

    /**
     * Retrieve the name of the Texture
     * 
     * @return: Returns the name of the Texture
     */
    virtual String GetName() const { return ""; }

    /**
     * Retrieve the texture description
     * 
     * @return: Returns the texture description
     */
    const SRHITextureDesc& GetDesc() const { return TextureDesc; }

protected:
    SRHITextureDesc TextureDesc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIShaderResourceViewTextureDesc

struct SRHIShaderResourceViewTextureDesc
{
    SRHIShaderResourceViewTextureDesc()
        : Format(ERHIFormat::Unknown)
        , FirstDepthOrArraySlice(0)
        , NumDepthOrArraySlices(0)
        , FirstMipLevel(0)
        , NumMipLevels(0)
    { }

    SRHIShaderResourceViewTextureDesc(ERHIFormat InFormat, uint16 InFirstDepthOrArraySlice, uint16 InNumDepthOrArraySlices, uint8 InFirstMipLevel, 
        uint8 InNumMipLevels)
        : Format(InFormat)
        , FirstDepthOrArraySlice(InFirstDepthOrArraySlice)
        , NumDepthOrArraySlices(InNumDepthOrArraySlices)
        , FirstMipLevel(InFirstMipLevel)
        , NumMipLevels(InNumMipLevels)
    { }

    bool operator==(const SRHIShaderResourceViewTextureDesc& Rhs) const
    {
        return 
            (Format                 == Rhs.Format)                 && 
            (FirstDepthOrArraySlice == Rhs.FirstDepthOrArraySlice) &&
            (NumDepthOrArraySlices  == Rhs.NumDepthOrArraySlices)  && 
            (FirstMipLevel          == Rhs.FirstMipLevel)          && 
            (NumMipLevels           == Rhs.NumMipLevels);
    }

    bool operator!=(const SRHIShaderResourceViewTextureDesc& Rhs) const
    {
        return !(*this == Rhs);
    }

    ERHIFormat Format;
    uint16     FirstDepthOrArraySlice;
    uint16     NumDepthOrArraySlices;
    uint8      FirstMipLevel;
    uint8      NumMipLevels;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIShaderResourceViewBufferDesc

struct SRHIShaderResourceViewBufferDesc
{
    SRHIShaderResourceViewBufferDesc()
        : FirstElement(0)
        , NumElements(0)
        , ElementStride(0)
    { }

    SRHIShaderResourceViewBufferDesc(uint32 InFirstElement, uint32 InNumElements, uint32 InElementStride)
        : FirstElement(InFirstElement)
        , NumElements(InNumElements)
        , ElementStride(InElementStride)
    { }

    bool operator==(const SRHIShaderResourceViewBufferDesc& Rhs) const
    {
        return (FirstElement && Rhs.FirstElement) && (NumElements && Rhs.NumElements) && (ElementStride && Rhs.ElementStride);
    }

    bool operator!=(const SRHIShaderResourceViewBufferDesc& Rhs) const
    {
        return !(*this == Rhs);
    }

    uint32 FirstElement;
    uint32 NumElements;
    uint32 ElementStride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderResourceView

class CRHIShaderResourceView : public CRHIResource
{
public:

    CRHIShaderResourceView()
        : CRHIResource(ERHIResourceType::ShaderResourceView)
    { }

    /**
     * Retrieve the Resource that the View represents
     * 
     * @return: Returns the resource the View represents
     */
    virtual CRHIResource* GetResource() const { return nullptr; }

    /**
     * Retrieve the Bindless descriptor-handle for this view
     * 
     * @param: Returns the bindless descriptor-handle for the view
     */
    virtual CRHIDescriptorHandle* GetBindlessHandle() const { return nullptr; }
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
// CRHIUnorderedAccessView

class CRHIUnorderedAccessView : public CRHIObject
{
public:
    virtual CRHIResource* GetResource() const { return nullptr; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDepthStencilView


class CRHIDepthStencilView : public CRHIObject
{
public:
    virtual CRHIResource* GetResource() const { return nullptr; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRenderTargetView

class CRHIRenderTargetView : public CRHIObject
{
public:
    virtual CRHIResource* GetResource() const { return nullptr; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper

using DepthStencilViewCube = TStaticArray<TSharedRef<CRHIDepthStencilView>, 6>;