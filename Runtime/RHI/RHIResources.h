#pragma once
#include "RHITypes.h"

#include "Core/Math/Vector3.h"
#include "Core/Math/IntVector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"

class CRHIRayTracingGeometryInstance;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIResource>            CRHIResourceRef;

typedef TSharedRef<class CRHIBuffer>              CRHIBufferRef;
typedef TSharedRef<class CRHITexture>             CRHITextureRef;

typedef TSharedRef<class CRHIShaderResourceView>  CRHIShaderResourceViewRef;
typedef TSharedRef<class CRHIUnorderedAccessView> CRHIUnorderedAccessViewRef;
typedef TSharedRef<class CRHIRenderTargetView>    CRHIRenderTargetViewRef;
typedef TSharedRef<class CRHIDepthStencilView>    CRHIDepthStencilViewRef;

typedef TSharedRef<class CRHISamplerState>        CRHISamplerStateRef;

typedef TSharedRef<class CRHIRayTracingGeometry>  CRHIRayTracingGeometryRef;
typedef TSharedRef<class CRHIRayTracingScene>     CRHIRayTracingSceneRef;

typedef TSharedRef<class CRHITimestampQuery>      CRHITimestampQueryRef;

typedef TSharedRef<class CRHIViewport>            CRHIViewportRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIResourceType

enum class ERHIResourceType : uint8
{
    Unknown                 = 0,
    Buffer                  = 1,
    Texture                 = 2,

    ShaderResourceView      = 3,
    UnorderedAccessView     = 4,
    RenderTargetView        = 5,
    DepthStencilView        = 6,

    RayTracingGeometry      = 7,
    RayTracingScene         = 8,

    GraphicsPipelineState   = 9,
    ComputePipelineState    = 10,
    RayTracingPipelineState = 11,

    DepthStencilState       = 12,
    RasterizerState         = 13,
    VertexInputLayout       = 14,

    SamplerState            = 15,

    Shader                  = 16,

    Viewport                = 17,

    TimestampQuery          = 18,
};

inline const char* ToString(ERHIResourceType ResourceType)
{
    switch (ResourceType)
    {
    case ERHIResourceType::Buffer:                  return "Buffer";
    case ERHIResourceType::Texture:                 return "Texture";
    
    case ERHIResourceType::ShaderResourceView:      return "ShaderResourceView";
    case ERHIResourceType::UnorderedAccessView:     return "UnorderedAccessView";
    case ERHIResourceType::RenderTargetView:        return "RenderTargetView";
    case ERHIResourceType::DepthStencilView:        return "DepthStencilView";
    
    case ERHIResourceType::RayTracingGeometry:      return "RayTracingGeometry";
    case ERHIResourceType::RayTracingScene:         return "RayTracingScene";
    
    case ERHIResourceType::GraphicsPipelineState:   return "GraphicsPipelineState";
    case ERHIResourceType::ComputePipelineState:    return "ComputePipelineState";
    case ERHIResourceType::RayTracingPipelineState: return "RayTracingPipelineState";

    case ERHIResourceType::DepthStencilState:       return "DepthStencilState";
    case ERHIResourceType::RasterizerState:         return "RasterizerState";
    case ERHIResourceType::VertexInputLayout:       return "VertexInputLayout";
    
    case ERHIResourceType::SamplerState:            return "SamplerState";

    case ERHIResourceType::Shader:                  return "Shader";

    case ERHIResourceType::Viewport:                return "Viewport";
    default:                                        return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIResource

class RHI_API CRHIResource
{
public:

    /**
     * @brief: Constructor taking the type of resource as argument
     * 
     * @param InResourceType: The type of resource being created
     */
    CRHIResource(ERHIResourceType InResourceType)
        : Type(InResourceType)
        , StrongReferences(1)
    { }

    /**
     * @brief: Add a reference to the reference-count, the new count is returned
     * 
     * @return: Returns the the Reference-Count
     */
    int32 AddRef()
    {
        Check(StrongReferences.Load() > 0);
        ++StrongReferences;
    }

    /**
     * @brief: Release a reference by decreasing the reference-count
     * 
     * @return: Returns the reference count
     */
    int32 Release()
    {
        const int32 RefCount = --StrongReferences;
        Check(RefCount >= 0);

        if (RefCount < 1)
        {
            Destroy_Internal();
        }

        return RefCount;
    }

    /**
     * @brief: Destroy the reference directly, bypassing the reference-count
     * 
     * @return: Returns the reference count
     */
    int32 Destroy()
    {
        const int32 RefCount = StrongReferences.Load();
        Check(RefCount > 0);

        Destroy_Internal();

        return RefCount;
    }

    /**
     * @brief: Retrieve the type of the resource
     * 
     * @return: Returns the type of the resource
     */
    ERHIResourceType GetType() const { return Type; }

protected:
    virtual ~CRHIResource() = default;

private:
    void Destroy_Internal()
    {
        delete this;
    }
    
    ERHIResourceType    Type;
    mutable AtomicInt32 StrongReferences;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EIndexFormat 

enum class EIndexFormat : uint8
{
    Unknown = 0,
    uint16  = 1,
    uint32  = 2,
};

inline const char* ToString(EIndexFormat IndexFormat)
{
    switch (IndexFormat)
    {
    case EIndexFormat::uint16: return "uint16";
    case EIndexFormat::uint32: return "uint32";
    default:                   return "Unknown";
    }
}

inline EIndexFormat GetIndexFormatFromStride(uint32 StrideInBytes)
{
    switch (StrideInBytes)
    {
    case 2:  return EIndexFormat::uint16;
    case 4:  return EIndexFormat::uint32;
    default: return EIndexFormat::Unknown;
    }
}

inline uint32 GetStrideFromIndexFormat(EIndexFormat IndexFormat)
{
    switch (IndexFormat)
    {
    case EIndexFormat::uint16: 2;
    case EIndexFormat::uint32: 4;
    default:                   0;
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EBufferUsageFlags

enum class EBufferUsageFlags : uint8
{
    None                 = 0,

    Default              = FLAG(1), // Default Device Memory
    Readback             = FLAG(2), // CPU readable Memory
    Dynamic              = FLAG(3), // Dynamic Memory

    AllowVertexBuffer    = FLAG(5), // Can be used as VertexBuffer
    AllowIndexBuffer     = FLAG(6), // Can be used as IndexBuffer
    AllowUnorderedAccess = FLAG(7), // Can be used in UnorderedAccessViews
    AllowShaderResource  = FLAG(8), // Can be used in ShaderResourceViews
    AllowConstantBuffer  = FLAG(9), // Can be used as a ConstantBuffer (Must be exclusive)

    RWBuffer = AllowUnorderedAccess | AllowShaderResource
};

ENUM_OPERATORS(EBufferUsageFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBufferCreateDesc

class CRHIBufferCreateDesc
{
public:

    /**
     * @brief: Creates a BufferDesc that describes a VertexBuffer
     * 
     * @param NumVertices: Number of vertices in the VertexBuffer
     * @param VertexStride: Stride of one vertex
     * @param InFlags: Flags that describe the usage of the buffer
     */
    static CRHIBufferCreateDesc CreateVertexDesc(uint32 NumVertices, uint16 VertexStride, EBufferUsageFlags InFlags = EBufferUsageFlags::None)
    {
        return CRHIBufferCreateDesc(NumVertices * VertexStride, VertexStride, InFlags | EBufferUsageFlags::AllowVertexBuffer);
    }

    /**
     * @brief: Creates a BufferDesc that describes a IndexBuffer
     *
     * @param IndexFormat: IndexFormat of the IndexBuffer
     * @param NumIndices: Number of indices in the buffer
     * @param InFlags: Flags that describe the usage of the buffer
     */
    static CRHIBufferCreateDesc CreateIndexDesc(EIndexFormat IndexFormat, uint32 NumIndices, EBufferUsageFlags InFlags = EBufferUsageFlags::None)
    {
        return CRHIBufferCreateDesc( GetStrideFromIndexFormat(IndexFormat) * NumIndices
                                   , GetStrideFromIndexFormat(IndexFormat)
                                   , InFlags | EBufferUsageFlags::AllowIndexBuffer);
    }

    /**
     * @brief: Creates a BufferDesc that describes a StructuredBuffer
     *
     * @param NumElements: Number of elements in the Buffer
     * @param Stride: Stride of each element in the Buffer
     * @param InFlags: Flags that describe the usage of the Buffer
     */
    static CRHIBufferCreateDesc CreateStructuredDesc(uint32 NumElements, uint16 Stride, EBufferUsageFlags InFlags = EBufferUsageFlags::None)
    {
        return CRHIBufferCreateDesc(NumElements * Stride, Stride, InFlags);
    }

    /**
     * @brief: Creates a BufferDesc that describes a ConstantBuffer
     *
     * @param NumElements: Number of elements in the Buffer
     * @param Stride: Stride of each element in the Buffer
     * @param InFlags: Flags that describe the usage of the Buffer
     */
    static CRHIBufferCreateDesc CreateConstantDesc(uint32 NumElements, uint16 Stride, EBufferUsageFlags InFlags = EBufferUsageFlags::None)
    {
        return CRHIBufferCreateDesc(NumElements * Stride, Stride, InFlags | EBufferUsageFlags::AllowConstantBuffer);
    }

    /**
     * @brief: Default Constructor
     */
    CRHIBufferCreateDesc()
        : Size(0)
        , ElementStride(0)
        , UsageFlags(EBufferUsageFlags::None)
    { }

    /**
     * @brief: Constructor
     * 
     * @param InSizeInBytes: Size of the buffer in bytes
     * @param InStrideInBytes: Stride of each element in the buffer
     * @param InFlags: Flags that describes the usage of the buffer
     */
    CRHIBufferCreateDesc(uint32 InSizeInBytes, uint16 InStrideInBytes, EBufferUsageFlags InFlags)
        : Size(InSizeInBytes)
        , ElementStride(InStrideInBytes)
        , UsageFlags(InFlags)
    { }

    /**
     * @brief: Check if the Buffer can be used as a UnorderedAccessView
     * 
     * @return: Returns true if the buffer can be used as a UnorderedAccessView
     */
    bool IsUAV() const { return bool(UsageFlags & EBufferUsageFlags::AllowUnorderedAccess); }
    
    /**
     * @brief: Check if the Buffer can be used as a ShaderResourceView
     *
     * @return: Returns true if the buffer can be used as a ShaderResourceView
     */
    bool IsSRV() const { return bool(UsageFlags & EBufferUsageFlags::AllowShaderResource); }

    /**
     * @brief: Check if the Buffer can be a used as a VertexBuffer
     *
     * @return: Returns true if the buffer can be used as a VertexBuffer
     */
    bool IsVertexBuffer() const { return bool(UsageFlags & EBufferUsageFlags::AllowVertexBuffer); }

    /**
     * @brief: Check if the Buffer can be a used as a IndexBuffer
     *
     * @return: Returns true if the buffer can be used as a IndexBuffer
     */
    bool IsIndexBuffer()  const { return bool(UsageFlags & EBufferUsageFlags::AllowIndexBuffer); }

    /**
     * @brief: Check if the Buffer can be a used as a RWBuffer (UnorderedAccessView and ShaderResourceView)
     *
     * @return: Returns true if the buffer can be used as a RWBuffer
     */
    bool IsRWBuffer() const { return bool(UsageFlags & EBufferUsageFlags::RWBuffer); }

    /**
     * @brief: Check if the Buffer is dynamic (Dynamic Buffers are stored CPU accessible)
     * 
     * @return: Returns true if the Buffer is dynamic
     */
    bool IsDynamic()  const { return bool(UsageFlags & EBufferUsageFlags::Dynamic); }
    
    /**
     * @brief: Check if the data in the Buffer can be read back to the CPU
     *
     * @return: Returns true if the data in the Buffer can be read back to the CPU
     */
    bool IsReadBack() const { return bool(UsageFlags & EBufferUsageFlags::Readback); }

    /**
     * @brief: Compare this Buffer Description to another instance
     * 
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIBufferCreateDesc& RHS) const
    {
        return (UsageFlags    == RHS.UsageFlags) 
            && (Size          == RHS.Size) 
            && (ElementStride == RHS.ElementStride);
    }

    /**
     * @brief: Compare this Buffer Description to another instance
     *
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const CRHIBufferCreateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Size of the buffer */
    uint32 Size;
    
    /** @brief: Stride of each element in the buffer */
    uint16 ElementStride;

    /** @brief: Flags of describing the usage of the buffer */
    EBufferUsageFlags UsageFlags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBuffer

class RHI_API CRHIBuffer : public CRHIResource
{
public:

    /**
     * @brief: Constructor for creating a new buffer
     * 
     * @param BufferDesc: Buffer Description
     */
    CRHIBuffer(const CRHIBufferCreateDesc& BufferDesc)
        : CRHIResource(ERHIResourceType::Buffer)
        , Size(BufferDesc.Size)
        , Stride(BufferDesc.ElementStride)
        , UsageFlags(BufferDesc.UsageFlags)
    { }

    /**
     * @brief: Retrieve the Native handle of the Buffer
     *
     * @return: Returns the native handle of the Buffer
     */
    virtual void* GetRHIHandle() const { return nullptr; }

    /**
     * @brief: Set the name of the Buffer
     *
     * @param InName: New name of of the Buffer
     */
    virtual void SetName(const String& InName) { }

    /**
     * @brief: Retrieve the name of the Buffer
     *
     * @return: Returns the name of the Buffer
     */
    virtual String GetName() const { return ""; }

    /**
     * @brief: Retrieve the bindless handle if the RHI-backend supports it, 
     * and if the buffer is created with the ConstantBuffer- flag
     *
     * @return: Returns the bindless handle
     */
    CRHIDescriptorHandle GetBindlessHandle() const { return BindlessHandle; }

    /**
     * @brief: Retrieve the Buffer Size
     * 
     * @return: Returns the Buffer Size
     */
    uint32 GetSize() const { return Size; }

    /**
     * @brief: Retrieve the Stride of each element in the buffer
     *
     * @return: Returns the Buffer Stride
     */
    uint16 GetStride() const { return Stride; }

    /**
     * @brief: Retrieve the Buffer UsageFlags
     *
     * @return: Returns the Buffer UsageFlags
     */
    EBufferUsageFlags GetUsageFlags() const { return UsageFlags; }

protected:
    CRHIDescriptorHandle BindlessHandle;

    EBufferUsageFlags    UsageFlags;

    uint16               Stride;
    uint32               Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ETextureUsageFlags

enum class ETextureUsageFlags : uint8
{
    None                 = 0,

    AllowRenderTarget    = FLAG(1), // RenderTargetView
    AllowDepthStencil    = FLAG(2), // DepthStencilView
    AllowUnorderedAccess = FLAG(3), // UnorderedAccessView
    AllowShaderResource  = FLAG(4), // ShaderResourceView

    NoDefaultRTV         = FLAG(5), // Do not create default RenderTargetView
    NoDefaultDSV         = FLAG(6), // Do not create default DepthStencilView
    NoDefaultUAV         = FLAG(7), // Do not create default UnorderedAccessView
    NoDefaultSRV         = FLAG(8), // Do not create default ShaderResourceView

    RWTexture    = AllowUnorderedAccess | AllowShaderResource,
    RenderTarget = AllowRenderTarget    | AllowShaderResource,
    ShadowMap    = AllowDepthStencil    | AllowShaderResource,
};

ENUM_OPERATORS(ETextureUsageFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ETextureType

enum class ETextureType : uint8
{
    Unknown          = 0,
    Texture1D        = 1,
    Texture1DArray   = 2,
    Texture2D        = 3,
    Texture2DArray   = 4,
    TextureCube      = 5,
    TextureCubeArray = 6,
    Texture3D        = 7
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCreateDesc

class CRHITextureCreateDesc
{
public:
    /**
     * @brief: Create a Texture Description that describes a Texture1D
     *
     * @param InFormat: Format of the Texture
     * @param InWidth: Width of the texture
     * @param InNumMips: Number of MipLevels of the Texture
     * @param InFlags: UsageFlags of the Texture
     */
    static CRHITextureCreateDesc Create1D( ERHIFormat InFormat
                                         , uint16 InWidth
                                         , uint8 InNumMips
                                         , ETextureUsageFlags InFlags)
    {
        return CRHITextureCreateDesc(ETextureType::Texture1D, InFormat, InWidth, 1, 1, 1, InNumMips, 1, InFlags);
    }

    /**
     * @brief: Create a Texture Description that describes a Texture1DArray
     *
     * @param InFormat: Format of the Texture
     * @param InWidth: Width of the texture
     * @param InArraySize: ArraySize of the Texture
     * @param InNumMips: Number of MipLevels of the Texture
     * @param InFlags: UsageFlags of the Texture
     */
    static CRHITextureCreateDesc Create1DArray( ERHIFormat InFormat
                                              , uint16 InWidth
                                              , uint16 InArraySize
                                              , uint8 InNumMips
                                              , ETextureUsageFlags InFlags)
    {
        return CRHITextureCreateDesc(ETextureType::Texture1DArray, InFormat, InWidth, 1, 1, InArraySize, InNumMips, 1, InFlags);
    }

    /**
     * @brief: Create a Texture Description that describes a Texture2D
     * 
     * @param InFormat: Format of the Texture
     * @param InWidth: Width of the texture
     * @param InHeight: Height of the texture
     * @param InNumMips: Number of MipLevels of the Texture
     * @param InNumSamples: Number of Samples of the Texture
     * @param InFlags: UsageFlags of the Texture
     */
    static CRHITextureCreateDesc Create2D( ERHIFormat InFormat
                                         , uint16 InWidth
                                         , uint16 InHeight
                                         , uint8 InNumMips
                                         , uint8 InNumSamples
                                         , ETextureUsageFlags InFlags)
    {
        return CRHITextureCreateDesc(ETextureType::Texture2D, InFormat, InWidth, InHeight, 1, 1, InNumMips, InNumSamples, InFlags);
    }

    /**
     * @brief: Create a Texture Description that describes a Texture2DArray
     *
     * @param InFormat: Format of the Texture
     * @param InWidth: Width of the Texture
     * @param InHeight: Height of the Texture
     * @param InArraySize: ArraySize of the Texture
     * @param InNumMips: Number of MipLevels of the Texture
     * @param InNumSamples: Number of Samples of the Texture
     * @param InFlags: UsageFlags of the Texture
     */
    static CRHITextureCreateDesc Create2DArray( ERHIFormat InFormat
                                              , uint16 InWidth
                                              , uint16 InHeight
                                              , uint16 InArraySize
                                              , uint8 InNumMips
                                              , uint8 InNumSamples
                                              , ETextureUsageFlags InFlags)
    {
        return CRHITextureCreateDesc(ETextureType::Texture2DArray, InFormat, InWidth, InHeight, 1, InArraySize, InNumMips, InNumSamples, InFlags);
    }

    /**
     * @brief: Create a Texture Description that describes a TextureCube
     *
     * @param InFormat: Format of the Texture
     * @param InSize: Width of the Texture
     * @param InNumMips: Number of MipLevels of the Texture
     * @param InNumSamples: Number of Samples of the Texture
     * @param InFlags: UsageFlags of the Texture
     */
    static CRHITextureCreateDesc CreateCube( ERHIFormat InFormat
                                           , uint16 InSize
                                           , uint8 InNumMips
                                           , uint8 InNumSamples
                                           , ETextureUsageFlags InFlags)
    {
        return CRHITextureCreateDesc(ETextureType::TextureCube, InFormat, InSize, InSize, 1, 1, InNumMips, InNumSamples, InFlags);
    }

    /**
     * @brief: Create a Texture Description that describes a TextureCubeArray
     *
     * @param InFormat: Format of the Texture
     * @param InSize: Width of the Texture
     * @param InArraySize: ArraySize of the Texture
     * @param InNumMips: Number of MipLevels of the Texture
     * @param InNumSamples: Number of Samples of the Texture
     * @param InFlags: UsageFlags of the Texture
     */
    static CRHITextureCreateDesc CreateCubeArray( ERHIFormat InFormat
                                                , uint16 InSize
                                                , uint16 InArraySize
                                                , uint8 InNumMips
                                                , uint8 InNumSamples
                                                , ETextureUsageFlags InFlags)
    {
        return CRHITextureCreateDesc(ETextureType::TextureCubeArray, InFormat, InSize, InSize, 1, InArraySize, InNumMips, InNumSamples, InFlags);
    }

    /**
     * @brief: Create a Texture Description that describes a Texture3D
     *
     * @param InFormat: Format of the Texture
     * @param InWidth: Width of the Texture
     * @param InHeight: Height of the Texture
     * @param InDepth: Depth of the Texture
     * @param InNumMips: Number of MipLevels of the Texture
     * @param InNumSamples: Number of Samples of the Texture
     * @param InFlags: UsageFlags of the Texture
     */
    static CRHITextureCreateDesc Create3D( ERHIFormat InFormat
                                         , uint16 InWidth
                                         , uint16 InHeight
                                         , uint16 InDepth
                                         , uint8 InNumMips
                                         , uint8 InNumSamples
                                         , ETextureUsageFlags InFlags)
    {
        return CRHITextureCreateDesc(ETextureType::Texture3D, InFormat, InWidth, InHeight, InDepth, 1, InNumMips, InNumSamples, InFlags);
    }

    /**
     * @brief: Default Constructor
     */
    CRHITextureCreateDesc()
        : Type(ETextureType::Unknown)
        , Format(ERHIFormat::Unknown)
        , Width(0)
        , Height(0)
        , Depth(0)
        , ArraySize(0)
        , NumMips(0)
        , NumSamples(0)
        , UsageFlags(ETextureUsageFlags::None)
    { }

    /**
     * @brief: Constructor
     *
     * @param InType: Type of the texture
     * @param InFormat: Format of the Texture
     * @param InWidth: Width of the Texture
     * @param InHeight: Height of the Texture
     * @param InDepth: Depth of the Texture
     * @param InNumMips: Number of MipLevels of the Texture
     * @param InNumSamples: Number of Samples of the Texture
     * @param InFlags: UsageFlags of the Texture
     */
    CRHITextureCreateDesc( ETextureType InType
                         , ERHIFormat InFormat
                         , uint16 InWidth
                         , uint16 InHeight
                         , uint16 InDepth
                         , uint16 InArraySize
                         , uint8 InNumMips
                         , uint8 InNumSamples
                         , ETextureUsageFlags InFlags)
        : Type(InType)
        , Format(InFormat)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
        , ArraySize(InArraySize)
        , NumMips(InNumMips)
        , NumSamples(InNumSamples)
        , UsageFlags(InFlags)
    { }

    /**
     * @brief: Check if the texture type is Texture1D
     * 
     * @return: Returns true if Texture1D
     */
    bool IsTexture1D() const { return (Type == ETextureType::Texture1D); }

    /**
     * @brief: Check if the texture type is Texture1DArray
     *
     * @return: Returns true if Texture1DArray
     */
    bool IsTexture1DArray() const { return (Type == ETextureType::Texture1DArray); }

    /**
     * @brief: Check if the texture type is Texture2D
     *
     * @return: Returns true if Texture2D
     */
    bool IsTexture2D() const { return (Type == ETextureType::Texture2D); }

    /**
     * @brief: Check if the texture type is Texture2DArray
     *
     * @return: Returns true if Texture2DArray
     */
    bool IsTexture2DArray() const { return (Type == ETextureType::Texture2DArray); }

    /**
     * @brief: Check if the texture type is TextureCube
     *
     * @return: Returns true if TextureCube
     */
    bool IsTextureCube() const { return (Type == ETextureType::TextureCube); }
    
    /**
     * @brief: Check if the texture type is TextureCubeArray
     *
     * @return: Returns true if TextureCubeArray
     */
    bool IsTextureCubeArray() const { return (Type == ETextureType::TextureCubeArray); }
    
    /**
     * @brief: Check if the texture type is Texture3D
     *
     * @return: Returns true if Texture3D
     */
    bool IsTexture3D() const { return (Type == ETextureType::Texture3D); }

    /**
     * @brief: Check if the Texture can be used as a ShaderResourceView
     * 
     * @return: Returns true if the Texture can be used as a ShaderResourceView 
     */
    bool IsSRV() const { return bool(UsageFlags & ETextureUsageFlags::AllowShaderResource); }

    /**
     * @brief: Check if the Texture can be used as a UnorderedAccessView
     *
     * @return: Returns true if the Texture can be used as a UnorderedAccessView
     */
    bool IsUAV() const { return bool(UsageFlags & ETextureUsageFlags::AllowUnorderedAccess); }

    /**
     * @brief: Check if the Texture can be used as a RenderTargetView
     *
     * @return: Returns true if the Texture can be used as a RenderTargetView
     */
    bool IsRTV() const { return bool(UsageFlags & ETextureUsageFlags::AllowRenderTarget); }

    /**
     * @brief: Check if the Texture can be used as a DepthStencilView
     *
     * @return: Returns true if the Texture can be used as a DepthStencilView
     */
    bool IsDSV() const { return bool(UsageFlags & ETextureUsageFlags::AllowDepthStencil); }
    
    /**
     * @brief: Check if the Texture can be a used as a RWTexture (UnorderedAccessView and ShaderResourceView)
     *
     * @return: Returns true if the buffer can be used as a RWTexture
     */
    bool IsRWTexture() const { return bool(UsageFlags &  ETextureUsageFlags::RWTexture); }

    /**
     * @brief: Compare two instances with each other
     * 
     * @return: Returns true if the instances are equal to each other
     */
    bool operator==(const CRHITextureCreateDesc& RHS) const
    {
        return (Type       == RHS.Type) 
            && (Format     == RHS.Format)
            && (Width      == RHS.Width)
            && (Height     == RHS.Height)
            && (Depth      == RHS.Depth)
            && (ArraySize  == RHS.ArraySize)
            && (NumMips    == RHS.NumMips)
            && (NumSamples == RHS.NumSamples)
            && (UsageFlags == RHS.UsageFlags)
            && (ClearValue == RHS.ClearValue);
    }

    /**
     * @brief: Compare two instances with each other
     *
     * @return: Returns false if the instances are equal to each other
     */
    bool operator!=(const CRHITextureCreateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Type of texture and the dimension */
    ETextureType Type;
    
    /** @brief: Format of the texture */
    ERHIFormat Format;
    
    /** @brief: UsageFlags of the texture */
    ETextureUsageFlags UsageFlags;
    
    /** @brief: Width of the texture */
    uint16 Width;
    
    /** @brief: Height of the texture */
    uint16 Height;
    
    /** @brief: Depth of the texture */
    uint16 Depth;
    
    /** @brief: ArraySize of the texture */
    uint16 ArraySize;
    
    /** @brief: Number of MipLevels of the texture */
    uint8 NumMips;
    
    /** @brief: Number of Samples of the texture */
    uint8 NumSamples;
    
    /** @brief: ClearValue of the texture */
    CRHIClearValue ClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture

class RHI_API CRHITexture : public CRHIResource
{
public:

    /**
     * @brief: Constructor
     *
     * @param TextureDesc: Texture Description
     */
    CRHITexture(const CRHITextureCreateDesc& TextureDesc)
        : CRHIResource(ERHIResourceType::Texture)
        , Type(TextureDesc.Type)
        , Format(TextureDesc.Format)
        , Width(TextureDesc.Width)
        , Height(TextureDesc.Height)
        , Depth(TextureDesc.Depth)
        , ArraySize(TextureDesc.ArraySize)
        , NumMips(TextureDesc.NumMips)
        , NumSamples(TextureDesc.NumSamples)
        , UsageFlags(TextureDesc.UsageFlags)
    { }

    /**
     * @brief: Retrieve the default ShaderResourceView. Pointer is valid if the AllowShaderResouce flag is set.
     * 
     * @return: Returns a pointer to the default ShaderResourceView
     */
    virtual CRHIShaderResourceView* GetDefaultShaderResouceView() const { return nullptr; }

    /**
     * @brief: Retrieve the Native handle of the Texture
     * 
     * @return: Returns the native handle of the resource
     */
    virtual void* GetRHIHandle() const { return nullptr; }

    /**
     * @brief: Set the name of the Texture
     * 
     * @param InName: New name of of the resource
     */
    virtual void SetName(const String& InName) { }

    /**
     * @brief: Retrieve the name of the Texture
     * 
     * @return: Returns the name of the Texture
     */
    virtual String GetName() const { return ""; }

    /**
     * @brief: Retrieve the default ShaderResourceView as a bindless descriptor-handle.
     * Pointer is valid if the AllowShaderResouce flag is set.
     *
     * @return: Returns a pointer to the default ShaderResourceView as a bindless descriptor-handle
     */
    CRHIDescriptorHandle GetDefaultBindlessHandle() const { return BindlessHandle; }

    /**
     * @brief: Retrieve the extent of the texture
     * 
     * @return: Returns a IntVector3 with Width, Height, and Depth
     */
    CIntVector3 GetExtent() const { return CIntVector3(Width, Height, Depth); }

    /**
     * @brief: Retrieve the texture Type
     *
     * @return: Returns the texture Type
     */
    ETextureType GetType() const { return Type; }

    /**
     * @brief: Retrieve the Usage-Flags of the texture
     *
     * @return: Returns the Usage-Flags of the texture
     */
    ETextureUsageFlags GetFlags() const { return UsageFlags; }

    /**
     * @brief: Retrieve the texture Format
     * 
     * @return: Returns the texture Format
     */
    ERHIFormat GetFormat() const { return Format; }

    /**
     * @brief: Retrieve the texture Width
     *
     * @return: Returns the texture Width
     */
    uint16 GetWidth() const { return Width; }

    /**
     * @brief: Retrieve the texture Height
     *
     * @return: Returns the texture Height
     */
    uint16 GetHeight() const { return Height; }

    /**
     * @brief: Retrieve the texture Depth
     *
     * @return: Returns the texture Depth
     */
    uint16 GetDepth() const { return Depth; }

    /**
     * @brief: Retrieve the texture Depth
     *
     * @return: Returns the texture Depth
     */
    uint16 GetArraySize() const { return ArraySize; }

    /**
     * @brief: Retrieve the number of MipLevels of the texture
     *
     * @return: Returns the number of MipLevels of the texture
     */
    uint8 GetNumMips() const { return NumMips; }

    /**
     * @brief: Retrieve the number of Samples of the texture
     *
     * @return: Returns the number of Samples of the texture
     */
    uint8 GetNumSamples() const { return NumSamples; }

private:
    ETextureType         Type;

    ETextureUsageFlags   UsageFlags;
    ERHIFormat           Format;

    uint16               Width;
    uint16               Height;
    uint16               Depth;
    uint16               ArraySize;
    uint8                NumMips;
    uint8                NumSamples;

    CRHIDescriptorHandle BindlessHandle;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderResourceViewCreateDesc

class CRHIShaderResourceViewCreateDesc
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIShaderResourceViewCreateDesc()
        : Format(ERHIFormat::Unknown)
        , FirstSlice(0)
        , NumSlices(0)
        , FirstMipLevel(0)
        , NumMipLevels(0)
    { }

    /**
     * @brief: Constructor
     * 
     * @param InFormat: Format for the ShaderResourceView
     * @param InFirstSlice: First slice of the view in terms of depth or array-index
     * @param InNumSlices: Number of slices in the view in terms of depth or array-index
     * @param InFirstMipLevel: First MipLevel of the texture in the view
     * @param InNumMipLevels: Number of MipLevels of the texture in the view
     */
    CRHIShaderResourceViewCreateDesc(ERHIFormat InFormat, uint16 InFirstSlice, uint16 InNumSlices, uint8 InFirstMipLevel, uint8 InNumMipLevels)
        : Format(InFormat)
        , FirstSlice(InFirstSlice)
        , NumSlices(InNumSlices)
        , FirstMipLevel(InFirstMipLevel)
        , NumMipLevels(InNumMipLevels)
    { }

    /**
     * @brief: Compare this instance with another
     * 
     * @param RHS: Other instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIShaderResourceViewCreateDesc& RHS) const
    {
        return (Format        == RHS.Format)
            && (FirstSlice    == RHS.FirstSlice)
            && (NumSlices     == RHS.NumSlices)
            && (FirstMipLevel == RHS.FirstMipLevel) 
            && (NumMipLevels  == RHS.NumMipLevels);
    }

    /**
     * @brief: Compare this instance with another
     *
     * @param RHS: Other instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const CRHIShaderResourceViewCreateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Format of the resource-view */
    ERHIFormat Format;
    
    /** @brief: First slice of depth or array-slice of the view */
    uint16 FirstSlice;
    
    /** @brief: Number of slices of the view */
    uint16 NumSlices;
    
    /** @brief: First MipLevel of the view */
    uint8 FirstMipLevel;
    
    /** @brief: Number of MipLevels of the view */
    uint8 NumMipLevels;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderResourceView

class RHI_API CRHIShaderResourceView : public CRHIResource
{
public:

    /**
     * @brief: Constructor
     */
    CRHIShaderResourceView(CRHIResource* InResource)
        : CRHIResource(ERHIResourceType::ShaderResourceView)
        , Resource(InResource)
    { }

    /**
     * @brief: Retrieve the Resource that the View represents
     * 
     * @return: Returns the resource the View represents
     */
    CRHIResource* GetResource() const { return Resource; }

    /**
     * @brief: Retrieve the Bindless descriptor-handle for this view
     *
     * @param: Returns the bindless descriptor-handle for the view
     */
    CRHIDescriptorHandle GetBindlessHandle() const { return BindlessHandle; }

protected:
    CRHIResource*        Resource;
    CRHIDescriptorHandle BindlessHandle;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIUnorderedAccessViewDesc

class CRHIUnorderedAccessViewCreateDesc
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIUnorderedAccessViewCreateDesc()
        : Format(ERHIFormat::Unknown)
        , FirstSlice(0)
        , NumSlices(0)
        , MipLevel(0)
    { }

    /**
     * @brief: Constructor
     *
     * @param InFormat: Format for the ShaderResourceView
     * @param InFirstSlice: First slice of the view in terms of depth or array-index
     * @param InNumSlices: Number of slices in the view in terms of depth or array-index
     * @param InMipLevel: MipLevel of the texture in the view
     */
    CRHIUnorderedAccessViewCreateDesc(ERHIFormat InFormat, uint16 InFirstSlice, uint16 InNumSlices, uint8 InMipLevel)
        : Format(InFormat)
        , FirstSlice(InFirstSlice)
        , NumSlices(InNumSlices)
        , MipLevel(InMipLevel)
    { }

    /**
     * @brief: Compare this instance with another
     *
     * @param RHS: Other instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIUnorderedAccessViewCreateDesc& RHS) const
    {
        return (Format == RHS.Format) && (FirstSlice == RHS.FirstSlice) && (NumSlices == RHS.NumSlices) && (MipLevel == RHS.MipLevel);
    }

    /**
     * @brief: Compare this instance with another
     *
     * @param RHS: Other instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const CRHIUnorderedAccessViewCreateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Format of the resource-view */
    ERHIFormat Format;
    
    /** @brief: First slice of depth or array-slice of the view */
    uint16 FirstSlice;
    
    /** @brief: Number of slices of the view */
    uint16 NumSlices;

    /** @brief: MipLevel of the view */
    uint8 MipLevel;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIUnorderedAccessView

class RHI_API CRHIUnorderedAccessView : public CRHIResource
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIUnorderedAccessView(CRHIResource* InResource)
        : CRHIResource(ERHIResourceType::UnorderedAccessView)
        , Resource(InResource)
        , BindlessHandle()
    { }

    /**
     * @brief: Retrieve the Resource that the View represents
     *
     * @return: Returns the resource the View represents
     */
    CRHIResource* GetResource() const { return Resource; }

    /**
     * @brief: Retrieve the Bindless descriptor-handle for this view
     *
     * @param: Returns the bindless descriptor-handle for the view
     */
    CRHIDescriptorHandle GetBindlessHandle() const { return BindlessHandle; }

protected:
    CRHIResource*        Resource;
    CRHIDescriptorHandle BindlessHandle;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRenderTargetViewDesc

class CRHIRenderTargetViewCreateDesc
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIRenderTargetViewCreateDesc()
        : Format(ERHIFormat::Unknown)
        , FirstSlice(0)
        , NumSlices(0)
        , MipLevel(0)
    { }

    /**
     * @brief: Constructor
     *
     * @param InFormat: Format for the ShaderResourceView
     * @param InFirstSlice: First slice of the view in terms of depth or array-index
     * @param InNumSlices: Number of slices in the view in terms of depth or array-index
     * @param InMipLevel: MipLevel of the texture in the view
     */
    CRHIRenderTargetViewCreateDesc(ERHIFormat InFormat, uint16 InFirstSlice, uint16 InNumSlices, uint8 InMipLevel)
        : Format(InFormat)
        , FirstSlice(InFirstSlice)
        , NumSlices(InNumSlices)
        , MipLevel(InMipLevel)
    { }

    /**
     * @brief: Compare this instance with another
     *
     * @param RHS: Other instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIRenderTargetViewCreateDesc& RHS) const
    {
        return (Format == RHS.Format) && (FirstSlice == RHS.FirstSlice) && (NumSlices == RHS.NumSlices) && (MipLevel == RHS.MipLevel);
    }

    /**
     * @brief: Compare this instance with another
     *
     * @param RHS: Other instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const CRHIRenderTargetViewCreateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Format of the resource-view */
    ERHIFormat Format;

    /** @brief: First slice of depth or array-slice of the view */
    uint16 FirstSlice;
    
    /** @brief: Number of slices of the view */
    uint16 NumSlices;
    
    /** @brief: MipLevel of the view */
    uint8 MipLevel;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRenderTargetView

class RHI_API CRHIRenderTargetView : public CRHIResource
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIRenderTargetView(CRHITexture* InResource)
        : CRHIResource(ERHIResourceType::RenderTargetView)
        , Resource(InResource)
    { }

    /**
     * @brief: Retrieve the Resource that the View represents
     *
     * @return: Returns the resource the View represents
     */
    CRHITexture* GetResource() const { return Resource; }

protected:
    CRHITexture* Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDepthStencilViewDesc

class CRHIDepthStencilViewCreateDesc
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIDepthStencilViewCreateDesc()
        : Format(ERHIFormat::Unknown)
        , FirstSlice(0)
        , NumSlices(0)
        , MipLevel(0)
    { }

    /**
     * @brief: Constructor
     *
     * @param InFormat: Format for the ShaderResourceView
     * @param InFirstSlice: First slice of the view in terms of depth or array-index
     * @param InNumSlices: Number of slices in the view in terms of depth or array-index
     * @param InMipLevel: MipLevel of the texture in the view
     */
    CRHIDepthStencilViewCreateDesc(ERHIFormat InFormat, uint16 InFirstSlice, uint16 InNumSlices, uint8 InMipLevel)
        : Format(InFormat)
        , FirstSlice(InFirstSlice)
        , NumSlices(InNumSlices)
        , MipLevel(InMipLevel)
    { }

    /**
     * @brief: Compare this instance with another
     *
     * @param RHS: Other instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIRenderTargetViewCreateDesc& RHS) const
    {
        return (Format == RHS.Format) && (FirstSlice == RHS.FirstSlice) && (NumSlices  == RHS.NumSlices) && (MipLevel == RHS.MipLevel);
    }

    /**
     * @brief: Compare this instance with another
     *
     * @param RHS: Other instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const CRHIRenderTargetViewCreateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Format of the resource-view */
    ERHIFormat Format;

    /** @brief: First slice of depth or array-slice of the view */
    uint16 FirstSlice;

    /** @brief: Number of slices of the view */
    uint16 NumSlices;
    
    /** @brief: MipLevel of the view */
    uint8 MipLevel;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDepthStencilView

class RHI_API CRHIDepthStencilView : public CRHIResource
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIDepthStencilView(CRHITexture* InResource)
        : CRHIResource(ERHIResourceType::DepthStencilView)
        , Resource(InResource)
    { }

    /**
     * @brief: Retrieve the Resource that the View represents
     *
     * @return: Returns the resource the View represents
     */
    CRHITexture* GetResource() const { return Resource; }

protected:
    CRHITexture* Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ESamplerMode

enum class ESamplerMode : uint8
{
    Unknown    = 0,
    Wrap       = 1,
    Mirror     = 2,
    Clamp      = 3,
    Border     = 4,
    MirrorOnce = 5,
};

inline const char* ToString(ESamplerMode SamplerMode)
{
    switch (SamplerMode)
    {
    case ESamplerMode::Wrap:       return "Wrap";
    case ESamplerMode::Mirror:     return "Mirror";
    case ESamplerMode::Clamp:      return "Clamp";
    case ESamplerMode::Border:     return "Border";
    case ESamplerMode::MirrorOnce: return "MirrorOnce";
    default:                       return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ESamplerFilter

enum class ESamplerFilter : uint8
{
    Unknown                                 = 0,
    MinMagMipPoint                          = 1,
    MinMagPoint_MipLinear                   = 2,
    MinPoint_MagLinear_MipPoint             = 3,
    MinPoint_MagMipLinear                   = 4,
    MinLinear_MagMipPoint                   = 5,
    MinLinear_MagPoint_MipLinear            = 6,
    MinMagLinear_MipPoint                   = 7,
    MinMagMipLinear                         = 8,
    Anistrotopic                            = 9,
    Comparison_MinMagMipPoint               = 10,
    Comparison_MinMagPoint_MipLinear        = 11,
    Comparison_MinPoint_MagLinear_MipPoint  = 12,
    Comparison_MinPoint_MagMipLinear        = 13,
    Comparison_MinLinear_MagMipPoint        = 14,
    Comparison_MinLinear_MagPoint_MipLinear = 15,
    Comparison_MinMagLinear_MipPoint        = 16,
    Comparison_MinMagMipLinear              = 17,
    Comparison_Anistrotopic                 = 18,
};

inline const char* ToString(ESamplerFilter SamplerFilter)
{
    switch (SamplerFilter)
    {
    case ESamplerFilter::MinMagMipPoint:                          return "MinMagMipPoint";
    case ESamplerFilter::MinMagPoint_MipLinear:                   return "MinMagPoint_MipLinear";
    case ESamplerFilter::MinPoint_MagLinear_MipPoint:             return "MinPoint_MagLinear_MipPoint";
    case ESamplerFilter::MinPoint_MagMipLinear:                   return "MinPoint_MagMipLinear";
    case ESamplerFilter::MinLinear_MagMipPoint:                   return "MinLinear_MagMipPoint";
    case ESamplerFilter::MinLinear_MagPoint_MipLinear:            return "MinLinear_MagPoint_MipLinear";
    case ESamplerFilter::MinMagLinear_MipPoint:                   return "MinMagLinear_MipPoint";
    case ESamplerFilter::MinMagMipLinear:                         return "MinMagMipLinear";
    case ESamplerFilter::Anistrotopic:                            return "Anistrotopic";
    case ESamplerFilter::Comparison_MinMagMipPoint:               return "Comparison_MinMagMipPoint";
    case ESamplerFilter::Comparison_MinMagPoint_MipLinear:        return "Comparison_MinMagPoint_MipLinear";
    case ESamplerFilter::Comparison_MinPoint_MagLinear_MipPoint:  return "Comparison_MinPoint_MagLinear_MipPoint";
    case ESamplerFilter::Comparison_MinPoint_MagMipLinear:        return "Comparison_MinPoint_MagMipLinear";
    case ESamplerFilter::Comparison_MinLinear_MagMipPoint:        return "Comparison_MinLinear_MagMipPoint";
    case ESamplerFilter::Comparison_MinLinear_MagPoint_MipLinear: return "Comparison_MinLinear_MagPoint_MipLinear";
    case ESamplerFilter::Comparison_MinMagLinear_MipPoint:        return "Comparison_MinMagLinear_MipPoint";
    case ESamplerFilter::Comparison_MinMagMipLinear:              return "Comparison_MinMagMipLinear";
    case ESamplerFilter::Comparison_Anistrotopic:                 return "Comparison_Anistrotopic";
    default:                                                      return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHISamplerStateCreateDesc

class CRHISamplerStateCreateDesc
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHISamplerStateCreateDesc()
        : AddressU(ESamplerMode::Clamp)
        , AddressV(ESamplerMode::Clamp)
        , AddressW(ESamplerMode::Clamp)
        , Filter(ESamplerFilter::MinMagMipLinear)
        , ComparisonFunc(EComparisonFunc::Never)
        , MipLODBias(0.0f)
        , MaxAnisotropy(1)
        , MinLOD(-FLT_MAX)
        , MaxLOD(FLT_MAX)
        , BorderColor()
    { }

    /**
     * @brief: Constructor that fills in a new sampler
     * 
     * @param InAddressU: Sampler mode in the U-direction
     * @param InAddressV: Sampler mode in the V-direction
     * @param InAddressW: Sampler mode in the W-direction
     * @param InFilter: Type of sampler
     * @param InComparisonFunc: ComparisonFunction if the sampler is a comparison sampler otherwise this is a no-op
     * @param InMipLODBias: Bias added to the selected MipLevel when sampling
     * @param InMaxAnisotropy: Maximum anisotropy for the sampler when the sampler is a Anistrotopic sampler
     * @param InMinLOD: Minimum MipLevel
     * @param InMaxLOD: Maximum MipLevel
     * @param InBorderColor: Color to return when the sampler should use a color when sampling out of range
     */
    CRHISamplerStateCreateDesc( ESamplerMode InAddressU
                              , ESamplerMode InAddressV
                              , ESamplerMode InAddressW
                              , ESamplerFilter InFilter
                              , EComparisonFunc InComparisonFunc
                              , float InMipLODBias
                              , uint8 InMaxAnisotropy
                              , float InMinLOD
                              , float InMaxLOD
                              , const CFloatColor& InBorderColor)
        : AddressU(InAddressU)
        , AddressV(InAddressV)
        , AddressW(InAddressW)
        , Filter(InFilter)
        , ComparisonFunc(InComparisonFunc)
        , MipLODBias(InMipLODBias)
        , MaxAnisotropy(InMaxAnisotropy)
        , MinLOD(InMinLOD)
        , MaxLOD(InMaxLOD)
        , BorderColor()
    { }

    /**
     * @brief: Compare this description with another instance
     * 
     * @param RHS: Other instance to compare with
     * @return: Returns true when the instances are equal
     */
    bool operator==(const CRHISamplerStateCreateDesc& RHS) const
    {
        return (AddressU       == RHS.AddressU)
            && (AddressV       == RHS.AddressV)
            && (AddressW       == RHS.AddressW)
            && (Filter         == RHS.Filter)
            && (ComparisonFunc == RHS.ComparisonFunc)
            && (MipLODBias     == RHS.MipLODBias)
            && (MaxAnisotropy  == RHS.MaxAnisotropy)
            && (MinLOD         == RHS.MinLOD)
            && (MaxLOD         == RHS.MaxLOD)
            && (BorderColor    == RHS.BorderColor);
    }

    /**
     * @brief: Compare this description with another instance
     *
     * @param RHS: Other instance to compare with
     * @return: Returns false when the instances are equal
     */
    bool operator!=(const CRHISamplerStateCreateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Sampler mode in the U-direction */
    ESamplerMode AddressU;
    
    /** @brief: Sampler mode in the V-direction */
    ESamplerMode AddressV;
    
    /** @brief: Sampler mode in the W-direction */
    ESamplerMode AddressW;
    
    /** @brief: Type of sampler */
    ESamplerFilter Filter;
    
    /** @brief: ComparisonFunction if the sampler is a comparison sampler otherwise this is a no-op */
    EComparisonFunc ComparisonFunc;
    
    /** @brief: Maximum anisotropy for the sampler when the sampler is a Anistrotopic sampler */
    uint8 MaxAnisotropy;
    
    /** @brief: Bias added to the selected MipLevel when sampling */
    float MipLODBias;
    
    /** @brief: Minimum MipLevel */
    float MinLOD;
    
    /** @brief: Maximum MipLevel */
    float MaxLOD;

    /** @brief: Color to return when the sampler should use a color when sampling out of range */
    CFloatColor BorderColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHISamplerState

class RHI_API CRHISamplerState : public CRHIResource
{
public:

    /**
     * @brief: Constructor
     *
     * @param InDesc: Description for the SamplerState
     */
    CRHISamplerState(const CRHISamplerStateCreateDesc& InDesc)
        : CRHIResource(ERHIResourceType::SamplerState)
        , Desc(InDesc)
    { }

    /**
     * @brief: Retrieve the bindless descriptor-handle if the RHI-supports descriptor-handles
     *
     * @return: Returns the bindless descriptor-handle if the RHI-supports descriptor-handles
     */
    CRHIDescriptorHandle GetBindlessHandle() const { return BindlessHandle; }

    /**
     * @brief: Retrieve the SamplerState description
     *
     * @return: Returns the SamplerState description
     */
    const CRHISamplerStateCreateDesc& GetDesc() const { return Desc; }

protected:
    CRHIDescriptorHandle       BindlessHandle;
    CRHISamplerStateCreateDesc Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRayPayload

struct SRayPayload
{
    CVector3 Color;
    uint32   CurrentDepth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRayIntersectionAttributes

struct SRayIntersectionAttributes
{
    float Attrib0;
    float Attrib1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERayTracingStructureBuildFlag

enum class ERayTracingStructureFlag : uint8
{
    None            = 0,
    AllowUpdate     = FLAG(1),
    PreferFastTrace = FLAG(2),
    PreferFastBuild = FLAG(3),
};

ENUM_OPERATORS(ERayTracingStructureFlag);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERayTracingInstanceFlags

enum class ERayTracingInstanceFlag : uint32
{
    None                  = 0,
    CullDisable           = FLAG(1),
    FrontCounterClockwise = FLAG(2),
    ForceOpaque           = FLAG(3),
    ForceNonOpaque        = FLAG(4),
};

ENUM_OPERATORS(ERayTracingInstanceFlag);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingGeometryCreateDesc

class CRHIRayTracingGeometryCreateDesc
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIRayTracingGeometryCreateDesc()
        : VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
        , NumVertices(0)
        , NumIndicies(0)
        , Flags(ERayTracingStructureFlag::None)
    { }

    /**
     * @brief: Constructor that fills in all the members
     * 
     * @param InVertexBuffer: VertexBuffer for the geometry
     * @param InNumVerteices: Number of vertices in the VertexBuffer
     * @param InIndexBuffer: IndexBuffer for the geometry
     */
    CRHIRayTracingGeometryCreateDesc( CRHIBuffer* InVertexBuffer
                                    , uint32 InNumVertices
                                    , CRHIBuffer* InIndexbuffer
                                    , EIndexFormat InIndexFormat
                                    , uint32 InNumIndicies
                                    , ERayTracingStructureFlag InFlags)
        : VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexbuffer)
        , IndexFormat(InIndexFormat)
        , NumVertices(InNumVertices)
        , NumIndicies(InNumIndicies)
        , Flags(InFlags)
    { }

    /**
     * @brief: Compare this instance to another one
     * 
     * @param RHS: Instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIRayTracingGeometryCreateDesc& RHS) const
    {
        return (VertexBuffer == RHS.VertexBuffer)
            && (IndexBuffer  == RHS.IndexBuffer)
            && (IndexFormat  == RHS.IndexFormat)
            && (NumVertices  == RHS.NumVertices)
            && (NumIndicies  == RHS.NumIndicies)
            && (Flags        == RHS.Flags);
    }

    /**
     * @brief: Compare this instance to another one
     *
     * @param RHS: Instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const CRHIRayTracingGeometryCreateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIBuffer*              VertexBuffer;
    CRHIBuffer*              IndexBuffer;
    EIndexFormat             IndexFormat;
    uint32                   NumVertices;
    uint32                   NumIndicies;
    ERayTracingStructureFlag Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingGeometry - (Bottom-Level Acceleration-Structure)

class CRHIRayTracingGeometry : public CRHIResource
{
public:

    /**
     * @brief: Constructor that fills in the members
     * 
     * @param InFlags: Flags for the RayTracingGeometry
     */
    CRHIRayTracingGeometry(ERayTracingStructureFlag InFlags)
        : CRHIResource(ERHIResourceType::RayTracingGeometry)
        , Flags(InFlags)
    { }

    /**
     * @brief: Retrieve the Flags of the RayTracingGeometry
     * 
     * @return: Returns the Flags of the RayTracingGeometry
     */
    ERayTracingStructureFlag GetFlags() const { return Flags; }

protected:
    ERayTracingStructureFlag Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIRayTracingGeometryInstance

class CRHIRayTracingGeometryInstance
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIRayTracingGeometryInstance()
        : Geometry(nullptr)
        , InstanceIndex(0)
        , HitGroupIndex(0)
        , Flags(ERayTracingInstanceFlag::None)
        , Mask(0xff)
        , Transform()
    { }

    /**
     * @brief: Constructor that fills in all members
     * 
     * @param InGeometry: Geometry used for the instance
     * @param InInstanceIndex: Custom instance-index
     * @param InHitGroupIndex: Hit-Group index
     * @param InFlags: Flags for the instance
     * @param InMask: Instance mask
     * @param InTransform: Instance-transform
     */
    CRHIRayTracingGeometryInstance( const CRHIRayTracingGeometryRef InGeometry
                                  , uint32 InInstanceIndex
                                  , uint32 InHitGroupIndex
                                  , ERayTracingInstanceFlag InFlags
                                  , uint32 InMask
                                  , const CMatrix3x4& InTransform)
        : Geometry(InGeometry)
        , InstanceIndex(InInstanceIndex)
        , HitGroupIndex(InHitGroupIndex)
        , Flags(InFlags)
        , Mask(InMask)
        , Transform(InTransform)
    { }

    /**
     * @brief: Check if two instances are equal
     * 
     * @param RHS: Other instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIRayTracingGeometryInstance& RHS) const
    {
        return (Geometry      == RHS.Geometry)
            && (InstanceIndex == RHS.InstanceIndex)
            && (HitGroupIndex == RHS.HitGroupIndex)
            && (Flags         == RHS.Flags)
            && (Mask          == RHS.Mask)
            && (Transform     == RHS.Transform);
    }

    /**
     * @brief: Check if two instances are equal
     *
     * @param RHS: Other instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator==(const CRHIRayTracingGeometryInstance& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief:Geometry to use for the instance */
    CRHIRayTracingGeometryRef Geometry;

    /** @brief: Custom InstanceIndex */
    uint32 InstanceIndex;

    /** @brief: Set the HitGroup index */
    uint32 HitGroupIndex;

    /** @brief: Flags for the instances */
    ERayTracingInstanceFlag Flags;

    /** @brief: Instance mask used to mask hits */
    uint32 Mask;

    /** @brief: Instance transform */
    CMatrix3x4 Transform;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingSceneCreateDesc

class CRHIRayTracingSceneCreateDesc
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIRayTracingSceneCreateDesc()
        : Instances(nullptr)
        , NumInstances(0)
        , Flags(ERayTracingInstanceFlag::None)
    { }

    /**
     * @brief: Constructor that fills in the members
     * 
     * @param InInstances: Instances for the Scene
     * @param InNumInstances: Number of instances
     * @param InFlags: Flags for the Scene
     */
    CRHIRayTracingSceneCreateDesc(CRHIRayTracingGeometryInstance* InInstances, uint32 InNumInstances, ERayTracingInstanceFlag InFlags)
        : Instances(InInstances)
        , NumInstances(InNumInstances)
        , Flags(InFlags)
    { }

    /**
     * @brief: Compare two instances with each other
     * 
     * @param RHS: Other instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIRayTracingSceneCreateDesc& RHS) const
    {
        return (Instances == RHS.Instances) && (NumInstances == RHS.NumInstances) && (Flags == RHS.Flags);
    }

    /**
     * @brief: Compare two instances with each other
     *
     * @param RHS: Other instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator==(const CRHIRayTracingSceneCreateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Array of Geometry instances */
    CRHIRayTracingGeometryInstance* Instances;

    /** @brief: Number of instances */
    uint32 NumInstances;

    /** @brief: Flags of the instance */
    ERayTracingInstanceFlag Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingScene  - (Top-Level Acceleration-Structure)

class CRHIRayTracingScene : public CRHIResource
{
public:

    /**
     * @brief: Constructor that fills in the members
     *
     * @param InFlags: Flags for the RayTracingGeometry
     */
    CRHIRayTracingScene(ERayTracingStructureFlag InFlags)
        : CRHIResource(ERHIResourceType::RayTracingScene)
        , Flags(InFlags)
    { }

    /**
     * @brief: Retrieve the default ShaderResourceView. Pointer is valid if the AllowShaderResouce flag is set.
     *
     * @return: Returns a pointer to the default ShaderResourceView
     */
    virtual CRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }
    
    /**
     * @brief: Retrieve the bindless descriptor-handle if the RHI-supports descriptor-handles
     *
     * @return: Returns the bindless descriptor-handle if the RHI-supports descriptor-handles
     */
    CRHIDescriptorHandle GetBindlessHandle() const { return BindlessHandle; }

    /**
     * @brief: Retrieve the Flags of the RayTracingScene
     *
     * @return: Returns the Flags of the RayTracingScene
     */
    ERayTracingStructureFlag GetFlags() const { return Flags; }

protected:
    ERayTracingStructureFlag Flags;
    CRHIDescriptorHandle     BindlessHandle;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHITimestamp

struct SRHITimestamp
{
    uint64 Begin;
    uint64 End;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITimestampQuery

class CRHITimestampQuery : public CRHIResource
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHITimestampQuery()
        : CRHIResource(ERHIResourceType::TimestampQuery)
    { }

    /**
     * @brief: Retrieve the number of timestamps in the query (Highest index)
     * 
     * @return: Returns the number of timestamps recorded
     */
    virtual uint32 GetNumTimestamps() const = 0;

    /**
     * @brief: Retrieve a timestamp from the query
     * 
     * @param OutQuery: Query to fill out
     * @param Index: Index of the query to fill 
     */
    virtual void GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const = 0;

    /**
     * @brief: Retrieve the frequency used to determine the timing in the query
     * 
     * @return: Returns the frequency which is used to convert the timestamps into a time-value
     */
    virtual uint64 GetFrequency() const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIViewport

class CRHIViewport : public CRHIResource
{
public:

    CRHIViewport(ERHIFormat InColorFormat, uint16 InWidth, uint16 InHeight)
        : CRHIResource(ERHIResourceType::Viewport)
        , Width(InWidth)
        , Height(InHeight)
        , ColorFormat(InColorFormat)
    { }

    virtual bool Resize(uint32 Width, uint32 Height) = 0;

    virtual CRHITexture* GetBackBuffer() const = 0;

    ERHIFormat GetColorFormat() const { return ColorFormat; }

    uint16 GetWidth()  const { return Width; }
    uint16 GetHeight() const { return Height; }

protected:
    ERHIFormat ColorFormat;
    uint16     Width;
    uint16     Height;
};