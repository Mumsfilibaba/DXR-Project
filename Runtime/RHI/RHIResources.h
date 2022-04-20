#pragma once
#include "IRHIResource.h"
#include "RHITypes.h"
#include "RHIUtilities.h"

#include "Core/Math/Vector3.h"
#include "Core/Math/IntVector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Containers/String.h"
#include "Core/Containers/HashTable.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Optional.h"

class CRHIRayTracingGeometryInstance;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIResource>            CRHIResourceRef;

typedef TSharedRef<class CRHIBuffer>              CRHIBufferRef;
typedef TSharedRef<class CRHIVertexBuffer>        CRHIVertexBufferRef;
typedef TSharedRef<class CRHIIndexBuffer>         CRHIIndexBufferRef;
typedef TSharedRef<class CRHIGenericBuffer>       CRHIGenericBufferRef;
typedef TSharedRef<class CRHIConstantBuffer>      CRHIConstantBufferRef;

typedef TSharedRef<class CRHITexture>             CRHITextureRef;
typedef TSharedRef<class CRHITexture2D>           CRHITexture2DRef;
typedef TSharedRef<class CRHITexture2DArray>      CRHITexture2DArrayRef;
typedef TSharedRef<class CRHITextureCube>         CRHITextureCubeRef;
typedef TSharedRef<class CRHITexture3D>           CRHITexture3DRef;

typedef TSharedRef<class CRHIShaderResourceView>  CRHIShaderResourceViewRef;
typedef TSharedRef<class CRHIUnorderedAccessView> CRHIUnorderedAccessViewRef;
typedef TSharedRef<class CRHIRenderTargetView>    CRHIRenderTargetViewRef;
typedef TSharedRef<class CRHIDepthStencilView>    CRHIDepthStencilViewRef;

typedef TSharedRef<class CRHISamplerState>        CRHISamplerStateRef;

typedef TSharedRef<class CRHIRayTracingGeometry>  CRHIRayTracingGeometryRef;
typedef TSharedRef<class CRHIRayTracingScene>     CRHIRayTracingSceneRef;

typedef TSharedRef<class CRHITimeQuery>           CRHITimeQueryRef;

typedef TSharedRef<class CRHIViewport>            CRHIViewportRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIResource

class CRHIResource : public IRHIResource
{
protected:

    CRHIResource()
        : StrongReferences(1)
    { }

    virtual ~CRHIResource() = default;

public:

    virtual int32 AddRef() override final
    {
        Check(StrongReferences.Load() > 0);
        ++StrongReferences;
    }

    virtual int32 Release() override final
    {
        const int32 RefCount = --StrongReferences;
        Check(RefCount >= 0);

        if (RefCount < 1)
        {
            delete this;
        }

        return RefCount;
    }

protected:
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

    AllowUnorderedAccess = FLAG(7), // Can be used in UnorderedAccessViews
    AllowShaderResource  = FLAG(8), // Can be used in ShaderResourceViews

    RWBuffer             = AllowUnorderedAccess | AllowShaderResource
};

ENUM_CLASS_OPERATORS(EBufferUsageFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBufferInitializer

class CRHIBufferInitializer
{
public:

    CRHIBufferInitializer()
        : UsageFlags(EBufferUsageFlags::None)
        , InitialState(EResourceAccess::Common)
    { }

    CRHIBufferInitializer(EBufferUsageFlags InUsageFlags, EResourceAccess InInitialState)
        : UsageFlags(InUsageFlags)
        , InitialState(InInitialState)
    { }

    bool operator==(const CRHIBufferInitializer& RHS) const
    {
        return (UsageFlags == RHS.UsageFlags) && (InitialState == RHS.InitialState);
    }

    bool operator!=(const CRHIBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    EBufferUsageFlags UsageFlags;
    EResourceAccess   InitialState;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBuffer

class CRHIBuffer : public CRHIResource
{
protected:

    CRHIBuffer(const CRHIBufferInitializer& Initializer)
        : CRHIResource()
        , UsageFlags(Initializer.UsageFlags)
    { }

public:

    /** @return: Returns the CRHIVertexBuffer interface if implemented */
    virtual class CRHIVertexBuffer* GetVertexBuffer() { return nullptr; }

    /** @return: Returns the CRHIIndexBuffer interface if implemented */
    virtual class CRHIIndexBuffer* GetIndexBuffer() { return nullptr; }

    /** @return: Returns the CRHIGenericBuffer interface if implemented */
    virtual class CRHIGenericBuffer* GetGenericBuffer() { return nullptr; }

    /** @return: Returns the CRHIConstantBuffer interface if implemented */
    virtual class CRHIConstantBuffer* GetConstantBuffer() { return nullptr; }

    /** @return: Returns the native handle of the Buffer */
    virtual void* GetRHIHandle() const { return nullptr; }

    /** @return: Returns the RHI-backend buffer interface */
    virtual void* GetRHIBaseBuffer() const { return nullptr; }

    /**
     * @brief: Set the name of the Buffer
     *
     * @param InName: New name of of the Buffer
     */
    virtual void SetName(const String& InName) { }

    /** @return: Returns true if the buffer is structured or not (If stride is more than one) */
    virtual bool IsStructured() const { return (GetStride() > 1); }

    /** @return: Returns the name of the Buffer */
    virtual String GetName() const { return ""; }

    /** @return: Returns the Buffer Stride */
    virtual uint16 GetStride() const { return 1; }

    /** @return: Returns the Buffer Size */
    virtual uint32 GetSize() const { return 1; }

    /** @return: Returns the Buffer UsageFlags */
    EBufferUsageFlags GetUsageFlags() const { return UsageFlags; }

protected:
    EBufferUsageFlags UsageFlags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexBufferInitializer

class CRHIVertexBufferInitializer : public CRHIBufferInitializer
{
public:

    CRHIVertexBufferInitializer()
        : CRHIBufferInitializer()
        , NumVertices(0)
        , Stride(0)
    { }

    CRHIVertexBufferInitializer(EBufferUsageFlags InUsageFlags, uint32 InNumVertices, uint16 InStride, EResourceAccess InInitialState)
        : CRHIBufferInitializer(InUsageFlags, InInitialState)
        , NumVertices(InNumVertices)
        , Stride(InStride)
    { }

    bool operator==(const CRHIVertexBufferInitializer& RHS) const
    {
        return CRHIBufferInitializer::operator==(RHS)
            && (NumVertices == RHS.NumVertices)
            && (Stride      == RHS.Stride);
    }

    bool operator!=(const CRHIVertexBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint32 NumVertices;
    uint16 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexBuffer

class CRHIVertexBuffer : public CRHIBuffer
{
protected:

    CRHIVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
        : CRHIBuffer(Initializer)
        , NumVertices(Initializer.NumVertices)
        , Stride(Initializer.Stride)
    { }

public:

    /** @return: Returns the CRHIVertexBuffer */
    virtual CRHIVertexBuffer* GetVertexBuffer() override final { return this; }

    /** @return: Returns the Buffer Stride */
    virtual uint16 GetStride() const override final { return Stride; }

    /** @return: Returns the Buffer Size */
    virtual uint32 GetSize() const override final { return NumVertices * Stride; }

    /** @return: Returns the number of vertices */
    uint32 GetNumVertices() const { return NumVertices; }

protected:
    uint32 NumVertices;
    uint16 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIIndexBufferInitializer

class CRHIIndexBufferInitializer : public CRHIBufferInitializer
{
public:

    CRHIIndexBufferInitializer()
        : CRHIBufferInitializer()
        , IndexFormat(EIndexFormat::Unknown)
        , NumIndicies(0)
    { }

    CRHIIndexBufferInitializer(EBufferUsageFlags InUsageFlags, EIndexFormat InIndexFormat, uint32 InNumIndicies, EResourceAccess InInitialState)
        : CRHIBufferInitializer(InUsageFlags, InInitialState)
        , IndexFormat(InIndexFormat)
        , NumIndicies(InNumIndicies)
    { }

    bool operator==(const CRHIIndexBufferInitializer& RHS) const
    {
        return CRHIBufferInitializer::operator==(RHS)
            && (IndexFormat == RHS.IndexFormat)
            && (NumIndicies == RHS.NumIndicies);
    }

    bool operator!=(const CRHIIndexBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    EIndexFormat IndexFormat;
    uint32       NumIndicies;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIIndexBuffer

class CRHIIndexBuffer : public CRHIBuffer
{
protected:

    CRHIIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
        : CRHIBuffer(Initializer)
        , Format(Initializer.IndexFormat)
        , NumIndicies(Initializer.NumIndicies)
    { }

public:

    /** @return: Returns the CRHIIndexBuffer */
    virtual CRHIIndexBuffer* GetIndexBuffer() override final { return this; }

    /** @return: Returns the Buffer Stride */
    virtual uint16 GetStride() const override final { return GetStrideFromIndexFormat(Format); }

    /** @return: Returns the Buffer Size */
    virtual uint32 GetSize() const override final { return NumIndicies * GetStride(); }

    /** @return: Returns the number of indices */
    uint32 GetNumIndicies() const { return NumIndicies; }

    /** @return: Returns the format on the indices */
    EIndexFormat GetIndexFormat() const { return Format; }

protected:
    uint32       NumIndicies;
    EIndexFormat Format;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGenericBufferInitializer

class CRHIGenericBufferInitializer : public CRHIBufferInitializer
{
public:

    CRHIGenericBufferInitializer()
        : CRHIBufferInitializer()
        , Size(0)
        , Stride(0)
    { }

    CRHIGenericBufferInitializer(EBufferUsageFlags InUsageFlags, uint32 InSize, uint32 InStride, EResourceAccess InInitialState)
        : CRHIBufferInitializer(InUsageFlags, InInitialState)
        , Size(InSize)
        , Stride(InStride)
    { }

    bool operator==(const CRHIGenericBufferInitializer& RHS) const
    {
        return CRHIBufferInitializer::operator==(RHS)
            && (Size   == RHS.Size)
            && (Stride == RHS.Stride);
    }

    bool operator!=(const CRHIGenericBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint32 Size;
    uint32 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGenericBuffer

class CRHIGenericBuffer : public CRHIBuffer
{
protected:

    CRHIGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
        : CRHIBuffer(Initializer)
        , Size(Initializer.Size)
        , Stride(Initializer.Stride)
    { }

public:

    /** @return: Returns the CRHIGenericBuffer */
    virtual CRHIGenericBuffer* GetGenericBuffer() override final { return this; }

    /** @return: Returns the Buffer Stride */
    virtual uint16 GetStride() const override final { return Stride; }

    /** @return: Returns the Buffer Size */
    virtual uint32 GetSize() const override final { return Size; }

protected:
    uint32 Size;
    uint32 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIConstantBufferInitializer

class CRHIConstantBufferInitializer : public CRHIBufferInitializer
{
public:

    CRHIConstantBufferInitializer()
        : CRHIBufferInitializer()
        , Size(0)
        , Stride(0)
    { }

    CRHIConstantBufferInitializer(EBufferUsageFlags InUsageFlags, uint32 InSize, uint32 InStride, EResourceAccess InInitialState)
        : CRHIBufferInitializer(InUsageFlags, InInitialState)
        , Size(InSize)
        , Stride(InStride)
    { }

    bool operator==(const CRHIConstantBufferInitializer& RHS) const
    {
        return CRHIBufferInitializer::operator==(RHS)
            && (Size   == RHS.Size)
            && (Stride == RHS.Stride);
    }

    bool operator!=(const CRHIConstantBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint32 Size;
    uint32 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIConstantBuffer

class CRHIConstantBuffer : public CRHIBuffer
{
protected:

    CRHIConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
        : CRHIBuffer(Initializer)
        , Size(Initializer.Size)
        , Stride(Initializer.Stride)
    {
        Check(!(InUsageFlags & (EBufferUsageFlags::AllowShaderResource | EBufferUsageFlags::AllowUnorderedAccess)));
    }

public:

    /** @return: Returns the Bindless handle if the RHI-backend supports it */
    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }

    /** @return: Returns the CRHIConstantBuffer */
    virtual CRHIConstantBuffer* GetConstantBuffer() override final { return this; }

    /** @return: Returns the Buffer Stride */
    virtual uint16 GetStride() const override final { return Stride; }

    /** @return: Returns the Buffer Size */
    virtual uint32 GetSize() const override final { return Size; }

protected:
    uint32 Size;
    uint32 Stride;
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
};

ENUM_CLASS_OPERATORS(ETextureUsageFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureInitializer

class CRHITextureInitializer
{
public:

    CRHITextureInitializer()
        : ClearValue()
        , Format(ERHIFormat::Unknown)
        , UsageFlags(ETextureUsageFlags::None)
        , InitialAccess(EResourceAccess::Common)
        , NumMips(1)
    { }

    CRHITextureInitializer( ERHIFormat InFormat
                          , ETextureUsageFlags InUsageFlags
                          , EResourceAccess InInitialAccess
                          , uint8 InNumMips
                          , const CTextureClearValue& InClearValue)
        : ClearValue(InClearValue)
        , Format(InFormat)
        , UsageFlags(InUsageFlags)
        , InitialAccess(InInitialAccess)
        , NumMips(InNumMips)
    { }

    bool operator==(const CRHITextureInitializer& RHS) const
    {
        return (ClearValue    == RHS.ClearValue)
            && (Format        == RHS.Format)
            && (UsageFlags    == RHS.UsageFlags)
            && (InitialAccess == RHS.InitialAccess)
            && (NumMips       == RHS.NumMips);
    }

    bool operator!=(const CRHITextureInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CTextureClearValue ClearValue;
    
    ERHIFormat         Format;

    ETextureUsageFlags UsageFlags;
    EResourceAccess    InitialAccess;

    uint8              NumMips;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture

class CRHITexture : public CRHIResource
{
protected:

    CRHITexture(const CRHITextureInitializer& Initializer)
        : CRHIResource()
        , Format(Initializer.Format)
        , UsageFlags(Initializer.UsageFlags)
        , NumMips(Initializer.NumMips)
    { }

public:

    /** @return: Returns the CRHITexture2D interface if implemented otherwise nullptr */
    virtual class CRHITexture2D* GetTexture2D() { return nullptr; }

    /** @return: Returns the CRHITexture2DArray interface if implemented otherwise nullptr */
    virtual class CRHITexture2DArray* GetTexture2DArray() { return nullptr; }

    /** @return: Returns the CRHITextureCube interface if implemented otherwise nullptr */
    virtual class CRHITextureCube* GetTextureCube() { return nullptr; }

    /** @return: Returns the CRHITexture3D interface if implemented otherwise nullptr */
    virtual class CRHITexture3D* GetTexture3D() { return nullptr; }

    /** @return: Returns a valid pointer to the default ShaderResourceView if the AllowShaderResouce flag is set */
    virtual CRHIShaderResourceView* GetDefaultShaderResouceView() const { return nullptr; }

    /**
     * @return: Returns a valid Bindless descriptor-handle to the default ShaderResourceView
     * if the AllowShaderResouce flag is set and the RHI supports
     */
    virtual CRHIDescriptorHandle GetDefaultBindlessHandle() const { return CRHIDescriptorHandle(); }

    /** @return: Returns the native handle of the resource */
    virtual void* GetRHIHandle() const { return nullptr; }

    /** @return: Returns the RHI-backend texture interface */
    virtual void* GetRHIBaseTexture() const { return nullptr; }

    /**
     * @brief: Set the name of the Texture
     *
     * @param InName: New name of of the resource
     */
    virtual void SetName(const String& InName) { }

    /** @return: Returns the name of the Texture */
    virtual String GetName() const { return ""; }

    /** @return: Returns a IntVector3 with Width, Height, and Depth */
    virtual CIntVector3 GetExtent() const { return CIntVector3(1, 1, 1); }

    /** @return: Returns the texture Width */
    virtual uint16 GetWidth() const { return 1; }

    /** @return: Returns the texture Height */
    virtual uint16 GetHeight() const { return 1; }

    /** @return: Returns the texture Depth */
    virtual uint16 GetDepth() const { return 1; }

    /** @return: Returns the texture ArraySize */
    virtual uint16 GetArraySize() const { return 1; }

    /** @return: Returns the number of Samples of the texture */
    virtual uint8 GetNumSamples() const { return 1; }

    /** @return: Returns the Usage-Flags of the texture */
    ETextureUsageFlags GetFlags() const { return UsageFlags; }

    /** @return: Returns the texture Format */
    ERHIFormat GetFormat() const { return Format; }

    /** @return: Returns the number of MipLevels of the texture */
    uint8 GetNumMips() const { return NumMips; }

    /** @return: Returns the ClearValue of the texture */
    const CTextureClearValue& GetClearValue() const { return ClearValue; }

private:
    ERHIFormat         Format;
    ETextureUsageFlags UsageFlags;

    uint8              NumMips;

    CTextureClearValue ClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2DInitializer

class CRHITexture2DInitializer : public CRHITextureInitializer
{
public:

    CRHITexture2DInitializer()
        : CRHITextureInitializer()
        , Width(1)
        , Height(1)
        , NumMips(1)
        , NumSamples(1)
    { }

    CRHITexture2DInitializer( ERHIFormat InFormat
                            , uint16 InWidth
                            , uint16 InHeight
                            , uint8 InNumMips
                            , uint8 InNumSamples
                            , ETextureUsageFlags InUsageFlags
                            , EResourceAccess InInitialAccess
                            , const CTextureClearValue& InClearValue)
        : CRHITextureInitializer(InFormat, InUsageFlags, InInitialAccess, InNumMips, InClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , NumMips(InNumMips)
        , NumSamples(InNumSamples)
    { }

    bool operator==(const CRHITexture2DInitializer& RHS) const
    {
        return CRHITextureInitializer::operator==(RHS)
            && (Width      == RHS.Width)
            && (Height     == RHS.Height)
            && (NumMips    == RHS.NumMips)
            && (NumSamples == RHS.NumSamples);
    }

    bool operator!=(const CRHITexture2DInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 Width;
    uint16 Height;

    uint8  NumMips;
    uint8  NumSamples;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2D

class CRHITexture2D : public CRHITexture
{
protected:

    CRHITexture2D(const CRHITexture2DInitializer& Initializer)
        : CRHITexture(Initializer)
        , Width(Initializer.Width)
        , Height(Initializer.Height)
        , NumSamples(Initializer.NumSamples)
    { }

public:

    /** @return: Returns the CRHITexture2D interface */
    virtual CRHITexture2D* GetTexture2D() { return this; }

    /** @return: Returns a IntVector3 with Width, Height, and Depth */
    virtual CIntVector3 GetExtent() const override { return CIntVector3(Width, Height, 1); }

    /** @return: Returns the texture Width */
    virtual uint16 GetWidth() const override final { return Width; }

    /** @return: Returns the texture Height */
    virtual uint16 GetHeight() const override final { return Height; }

    /** @return: Returns the number of samples in the texture */
    virtual uint8 GetNumSamples() const override final { return NumSamples; }

protected:
    uint16 Width;
    uint16 Height;
    uint16 NumSamples;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2DArrayInitializer

class CRHITexture2DArrayInitializer : public CRHITexture2DInitializer
{
public:

    CRHITexture2DArrayInitializer()
        : CRHITexture2DInitializer()
        , ArraySize(1)
    { }

    CRHITexture2DArrayInitializer( ERHIFormat InFormat
                                 , uint16 InWidth
                                 , uint16 InHeight
                                 , uint16 InArraySize
                                 , uint8 InNumMips
                                 , uint8 InNumSamples
                                 , ETextureUsageFlags InUsageFlags
                                 , EResourceAccess InInitialAccess
                                 , const CTextureClearValue& InClearValue)
        : CRHITexture2DInitializer(InFormat, InWidth, InHeight,InNumMips, InNumSamples, InUsageFlags, InInitialAccess, InClearValue)
        , ArraySize(InArraySize)
    { }

    bool operator==(const CRHITexture2DArrayInitializer& RHS) const
    {
        return CRHITexture2DInitializer::operator==(RHS)
            && (ArraySize == RHS.ArraySize);
    }

    bool operator!=(const CRHITexture2DArrayInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture2DArray

class CRHITexture2DArray : public CRHITexture2D
{
protected:

    CRHITexture2DArray(const CRHITexture2DArrayInitializer& Initializer)
        : CRHITexture2D(Initializer)
        , ArraySize(Initializer.ArraySize)
    { }

public:

    /** @return: Returns the CRHITexture2D interface */
    virtual CRHITexture2D* GetTexture2D() override final { return nullptr; }

    /** @return: Returns the CRHITexture2DArray interface */
    virtual CRHITexture2DArray* GetTexture2DArray() override final { return this; }

    /** @return: Returns a IntVector3 with Width, Height, and Depth */
    virtual CIntVector3 GetExtent() const override final { return CIntVector3(GetWidth(), GetDepth(), ArraySize); }

    /** @return: Returns the ArraySize of the texture */
    virtual uint16 GetArraySize() const override final { return ArraySize; }

protected:
    uint16 ArraySize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCubeInitializer

class CRHITextureCubeInitializer : public CRHITextureInitializer
{
public:

    CRHITextureCubeInitializer()
        : CRHITextureInitializer()
        , Extent(1)
        , ArraySize(1)
        , NumSamples(1)
    { }

    CRHITextureCubeInitializer(ERHIFormat InFormat
                             , uint16 InExtent
                             , uint16 InArraySize
                             , uint8 InNumMips
                             , uint8 InNumSamples
                             , ETextureUsageFlags InUsageFlags
                             , EResourceAccess InInitialAccess
                             , const CTextureClearValue& InClearValue)
        : CRHITextureInitializer(InFormat, InUsageFlags, InInitialAccess, InNumMips, InClearValue)
        , Extent(InExtent)
        , ArraySize(InArraySize)
        , NumSamples(InNumSamples)
    { }

    bool operator==(const CRHITextureCubeInitializer& RHS) const
    {
        return CRHITextureInitializer::operator==(RHS)
            && (Extent     == RHS.Extent)
            && (ArraySize  == RHS.ArraySize)
            && (NumSamples == RHS.NumSamples);
    }

    bool operator!=(const CRHITextureCubeInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 Extent;
    uint16 ArraySize;

    uint8  NumSamples;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureCube

class CRHITextureCube : public CRHITexture
{
protected:

    CRHITextureCube(const CRHITextureCubeInitializer& Initializer)
        : CRHITexture(Initializer)
        , Extent(Initializer.Extent)
        , ArraySize(Initializer.ArraySize)
        , NumSamples(Initializer.NumSamples)
    { }

public:

    /** @return: Returns the CRHITextureCube interface if implemented otherwise nullptr */
    virtual CRHITextureCube* GetTextureCube() override final { return this; }

    /** @return: Returns a IntVector3 with Width, Height, and Depth */
    virtual CIntVector3 GetExtent() const override final { return CIntVector3(Extent, Extent, ArraySize); }

    /** @return: Returns the texture Width */
    virtual uint16 GetWidth()  const override final { return Extent; }

    /** @return: Returns the texture Height */
    virtual uint16 GetHeight() const override final { return Extent; }

    /** @return: Returns the ArraySize of the texture */
    virtual uint16 GetArraySize() const override final { return ArraySize; }

    /** @return: Returns the number of samples in the texture */
    virtual uint8 GetNumSamples() const override final { return NumSamples; }

protected:
    uint16 Extent;
    uint16 ArraySize;
    uint16 NumSamples;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture3DInitializer

class CRHITexture3DInitializer : public CRHITextureInitializer
{
public:

    CRHITexture3DInitializer()
        : CRHITextureInitializer()
        , Width(1)
        , Height(1)
        , Depth(1)
    { }

    CRHITexture3DInitializer( ERHIFormat InFormat
                            , uint16 InWidth
                            , uint16 InHeight
                            , uint16 InDepth
                            , uint8 InNumMips
                            , ETextureUsageFlags InUsageFlags
                            , EResourceAccess InInitialAccess
                            , const CTextureClearValue& InClearValue)
        : CRHITextureInitializer(InFormat, InUsageFlags, InInitialAccess, InNumMips, InClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    { }

    bool operator==(const CRHITexture3DInitializer& RHS) const
    {
        return CRHITextureInitializer::operator==(RHS)
            && (Width  == RHS.Width)
            && (Height == RHS.Height)
            && (Depth  == RHS.Depth);
    }

    bool operator!=(const CRHITexture3DInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint16 Width;
    uint16 Height;
    uint16 Depth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITexture3D

class CRHITexture3D : public CRHITexture
{
protected:

    CRHITexture3D(const CRHITexture3DInitializer& Initializer)
        : CRHITexture(Initializer)
        , Width(Initializer.Width)
        , Height(Initializer.Height)
        , Depth(Initializer.Depth)
    { }

public:

    /** @return: Returns the CRHITexture3D interface if implemented otherwise nullptr */
    virtual CRHITexture3D* GetTexture3D() override final { return this; }

    /** @return: Returns a IntVector3 with Width, Height, and Depth */
    virtual CIntVector3 GetExtent() const override final { return CIntVector3(Width, Height, Depth); }

    /** @return: Returns the texture Width */
    virtual uint16 GetWidth() const override final { return Width; }

    /** @return: Returns the texture Height */
    virtual uint16 GetHeight() const override final { return Height; }

    /** @return: Returns the texture Depth */
    virtual uint16 GetDepth()  const override final { return Depth; }

protected:
    uint16 Width;
    uint16 Height;
    uint16 Depth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureSRVInitializer

class CRHITextureSRVInitializer
{
public:

    CRHITextureSRVInitializer()
        : Texture(nullptr)
        , Format(ERHIFormat::Unknown)
        , FirstMipLevel(0)
        , NumMips(0)
        , FirstArraySlice(0)
        , NumSlices(0)
    { }

    CRHITextureSRVInitializer( CRHITexture* InTexture
                            , ERHIFormat InFormat
                            , uint8 InFirstMipLevel
                            , uint8 InNumMips
                            , uint16 InFirstArraySlice
                            , uint16 InNumSlices)
        : Texture(InTexture)
        , Format(InFormat)
        , FirstMipLevel(InFirstMipLevel)
        , NumMips(InNumMips)
        , FirstArraySlice(InFirstArraySlice)
        , NumSlices(InNumSlices)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
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

    ERHIFormat   Format;

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
    { }

    CRHIBufferSRVInitializer(CRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements)
        : Buffer(InBuffer)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Buffer);
        HashCombine(Hash, FirstElement);
        HashCombine(Hash, NumElements);
        return Hash;
    }

    bool operator==(const CRHIBufferSRVInitializer& RHS) const
    {
        return (Buffer == RHS.Buffer) && (FirstElement == RHS.FirstElement) && (NumElements == RHS.NumElements);
    }

    bool operator!=(const CRHIBufferSRVInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIBuffer* Buffer;

    uint32      FirstElement;
    uint32      NumElements;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderResourceView

class CRHIShaderResourceView : public CRHIResource
{
protected:

    explicit CRHIShaderResourceView(CRHIResource* InResource)
        : CRHIResource()
        , Resource(InResource)
    { }

public:

    /** @return: Returns the Bindless handle if the RHI-backend supports it */
    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }

    /** @return: Returns the resource the View represents */
    CRHIResource* GetResource() const { return Resource; }

protected:
    CRHIResource* Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITextureUAVInitializer

class CRHITextureUAVInitializer
{
public:

    CRHITextureUAVInitializer()
        : Texture(nullptr)
        , Format(ERHIFormat::Unknown)
        , MipLevel(0)
        , FirstArraySlice(0)
        , NumSlices(0)
    { }

    CRHITextureUAVInitializer( CRHITexture* InTexture
                            , ERHIFormat InFormat
                            , uint8 InMipLevel
                            , uint16 InFirstArraySlice
                            , uint16 InNumSlices)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(InMipLevel)
        , FirstArraySlice(InFirstArraySlice)
        , NumSlices(InNumSlices)
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

    ERHIFormat   Format;

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
    { }

    CRHIBufferUAVInitializer(CRHIBuffer* InBuffer, uint32 InFirstElement, uint32 InNumElements)
        : Buffer(InBuffer)
        , FirstElement(InFirstElement)
        , NumElements(InNumElements)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Buffer);
        HashCombine(Hash, FirstElement);
        HashCombine(Hash, NumElements);
        return Hash;
    }

    bool operator==(const CRHIBufferUAVInitializer& RHS) const
    {
        return (Buffer == RHS.Buffer) && (FirstElement == RHS.FirstElement) && (NumElements == RHS.NumElements);
    }

    bool operator!=(const CRHIBufferUAVInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIBuffer* Buffer;

    uint32      FirstElement;
    uint32      NumElements;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIUnorderedAccessView

class CRHIUnorderedAccessView : public CRHIResource
{
protected:

    explicit CRHIUnorderedAccessView(CRHIResource* InResource)
        : CRHIResource()
        , Resource(InResource)
    { }

public:

    /** @return: Returns the Bindless handle if the RHI-backend supports it */
    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }

    /** @return: Returns the resource the View represents */
    CRHIResource* GetResource() const { return Resource; }

protected:
    CRHIResource* Resource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRenderTargetView

class CRHIRenderTargetView
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIRenderTargetView()
        : Texture(nullptr)
        , Format(ERHIFormat::Unknown)
        , FirstArraySlice(0)
        , NumArraySlices(0)
        , MipLevel(0)
        , LoadAction(EAttachmentLoadAction::None)
        , StoreAction(EAttachmentStoreAction::None)
        , ClearValue()
    { }

    /**
     * @brief: Constructor
     *
     * @param InTexture: Texture that the view represent
     * @param InFormat: Format of the texture
     * @param InFirstArraySlice: FirstArraySlice of the texture
     * @param InNumArraySlices: Number of ArraySlices of the texture
     * @param InMipLevel: MipLevel of the texture
     * @param InLoadAction: Action to take before rendering to the Rendertarget
     * @param InStoreAction: Action to take after rendering to the Rendertarget
     */
    CRHIRenderTargetView( CRHITexture* InTexture
                        , ERHIFormat InFormat
                        , uint16 InFirstArraySlice
                        , uint16 InNumArraySlices
                        , uint8 InMipLevel
                        , EAttachmentLoadAction InLoadAction
                        , EAttachmentStoreAction InStoreAction
                        , CFloatColor InClearValue)
        : Texture(InTexture)
        , Format(InFormat)
        , FirstArraySlice(InFirstArraySlice)
        , NumArraySlices(InNumArraySlices)
        , MipLevel(InMipLevel)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    /** @return: Returns and calculates the hash for this type */
    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, FirstArraySlice);
        HashCombine(Hash, NumArraySlices);
        HashCombine(Hash, MipLevel);
        HashCombine(Hash, ToUnderlying(LoadAction));
        HashCombine(Hash, ToUnderlying(StoreAction));
        HashCombine(Hash, ClearValue.GetHash());
        return Hash;
    }

    /**
     * @brief: Compare the view with another view
     *
     * @param RHS: Instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIRenderTargetView& RHS) const
    {
        return (Texture         == RHS.Texture)
            && (Format          == RHS.Format)
            && (FirstArraySlice == RHS.FirstArraySlice)
            && (NumArraySlices  == RHS.NumArraySlices)
            && (MipLevel        == RHS.MipLevel)
            && (LoadAction      == RHS.LoadAction)
            && (StoreAction     == RHS.StoreAction)
            && (ClearValue      == RHS.ClearValue);
    }

    /**
     * @brief: Compare the view with another view
     *
     * @param RHS: Instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const CRHIRenderTargetView& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Texture that the view represent */
    CRHITexture* Texture;

    /** @brief: Format of the texture */
    ERHIFormat Format;

    /** @brief: FirstArraySlice of the texture */
    uint16 FirstArraySlice;

    /** @brief: Number of ArraySlices of the texture */
    uint16 NumArraySlices;

    /** @brief: MipLevel of the texture */
    uint8 MipLevel;

    /** @brief: Action to take when loading the RenderTarget before rendering */
    EAttachmentLoadAction LoadAction;

    /** @breif: Action to take when storing the RenderTarget after rendering */
    EAttachmentStoreAction StoreAction;

    /** @breif: Clear-value used if LoadAction is set to clear */
    CFloatColor ClearValue;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDepthStencilView

class CRHIDepthStencilView
{
public:

    /**
     * @brief: Default Constructor
     */
    CRHIDepthStencilView()
        : Texture(nullptr)
        , Format(ERHIFormat::Unknown)
        , FirstArraySlice(0)
        , NumArraySlices(0)
        , MipLevel(0)
        , LoadAction(EAttachmentLoadAction::None)
        , StoreAction(EAttachmentStoreAction::None)
        , ClearValue()
    { }

    /**
     * @brief: Constructor
     *
     * @param InTexture: Texture that the view represent
     * @param InFormat: Format of the texture
     * @param InFirstArraySlice: FirstArraySlice of the texture
     * @param InNumArraySlices: Number of ArraySlices of the texture
     * @param InMipLevel: MipLevel of the texture
     * @param InLoadAction: Action to take before rendering to the Rendertarget
     * @param InStoreAction: Action to take after rendering to the Rendertarget
     */
    CRHIDepthStencilView( CRHITexture* InTexture
                        , ERHIFormat InFormat
                        , uint16 InFirstArraySlice
                        , uint16 InNumArraySlices
                        , uint8 InMipLevel
                        , EAttachmentLoadAction InLoadAction
                        , EAttachmentStoreAction InStoreAction
                        , CTextureDepthStencilValue InClearValue)
        : Texture(InTexture)
        , Format(InFormat)
        , FirstArraySlice(InFirstArraySlice)
        , NumArraySlices(InNumArraySlices)
        , MipLevel(InMipLevel)
        , LoadAction(InLoadAction)
        , StoreAction(InStoreAction)
        , ClearValue(InClearValue)
    { }

    /** @return: Returns and calculates the hash for this type */
    uint64 GetHash() const
    {
        uint64 Hash = ToInteger(Texture);
        HashCombine(Hash, ToUnderlying(Format));
        HashCombine(Hash, FirstArraySlice);
        HashCombine(Hash, NumArraySlices);
        HashCombine(Hash, MipLevel);
        HashCombine(Hash, ToUnderlying(LoadAction));
        HashCombine(Hash, ToUnderlying(StoreAction));
        HashCombine(Hash, ClearValue.GetHash());
        return Hash;
    }

    /**
     * @brief: Compare the view with another view
     *
     * @param RHS: Instance to compare with
     * @return: Returns true if the instances are equal
     */
    bool operator==(const CRHIDepthStencilView& RHS) const
    {
        return (Texture         == RHS.Texture)
            && (Format          == RHS.Format)
            && (FirstArraySlice == RHS.FirstArraySlice)
            && (NumArraySlices  == RHS.NumArraySlices)
            && (MipLevel        == RHS.MipLevel)
            && (LoadAction      == RHS.LoadAction)
            && (StoreAction     == RHS.StoreAction)
            && (ClearValue      == RHS.ClearValue);
    }

    /**
     * @brief: Compare the view with another view
     *
     * @param RHS: Instance to compare with
     * @return: Returns false if the instances are equal
     */
    bool operator!=(const CRHIDepthStencilView& RHS) const
    {
        return !(*this == RHS);
    }

    /** @brief: Texture that the view represent */
    CRHITexture* Texture;

    /** @brief: Format of the texture */
    ERHIFormat Format;

    /** @brief: FirstArraySlice of the texture */
    uint16 FirstArraySlice;

    /** @brief: Number of ArraySlices of the texture */
    uint16 NumArraySlices;

    /** @brief: MipLevel of the texture */
    uint8 MipLevel;

    /** @brief: Action to take when loading the RenderTarget before rendering */
    EAttachmentLoadAction LoadAction;

    /** @breif: Action to take when storing the RenderTarget after rendering */
    EAttachmentStoreAction StoreAction;

    /** @breif: Clear-value used if LoadAction is set to clear */
    CTextureDepthStencilValue ClearValue;
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
// CRHISamplerStateDesc

class CRHISamplerStateInitializer
{
public:

    /**
     * @brief: Create a simple SamplerState
     *
     * @param InAddressMode: Address-mode for all axises
     * @param InFilter: Filtering mode
     * @param InMaxAnisotropy: Max count of anisotropic filtering if that is the selected filter
     */
    static CRHISamplerStateInitializer Create(ESamplerMode InAddressMode, ESamplerFilter InFilter, uint8 InMaxAnisotropy)
    {
        return CRHISamplerStateInitializer(InAddressMode
                                    , InAddressMode
                                    , InAddressMode
                                    , InFilter
                                    , EComparisonFunc::Unknown
                                    , 0.0f
                                    , InMaxAnisotropy
                                    , 0.0f
                                    , FLT_MAX
                                    , CFloatColor(0.0f, 0.0f, 0.0f, 1.0f));
    }

    /**
     * @brief: Default Constructor
     */
    CRHISamplerStateInitializer()
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
    CRHISamplerStateInitializer( ESamplerMode InAddressU
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

    /** @return: Returns and calculates the hash for this type */
    uint64 GetHash() const
    {
        uint64 Hash = ToUnderlying(AddressU);
        HashCombine(Hash, ToUnderlying(AddressV));
        HashCombine(Hash, ToUnderlying(AddressW));
        HashCombine(Hash, ToUnderlying(Filter));
        HashCombine(Hash, ToUnderlying(ComparisonFunc));
        HashCombine(Hash, MaxAnisotropy);
        HashCombine(Hash, MinLOD);
        HashCombine(Hash, MinLOD);
        HashCombine(Hash, MaxLOD);
        HashCombine(Hash, BorderColor.GetHash());
        return Hash;
    }

    /**
     * @brief: Compare this description with another instance
     *
     * @param RHS: Other instance to compare with
     * @return: Returns true when the instances are equal
     */
    bool operator==(const CRHISamplerStateInitializer& RHS) const
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
    bool operator!=(const CRHISamplerStateInitializer& RHS) const
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

class CRHISamplerState : public CRHIResource
{
protected:

    explicit CRHISamplerState(const CRHISamplerStateInitializer& InInitializer)
        : CRHIResource()
        , Initializer(InInitializer)
    { }

public:

    /** @return: Returns the Bindless descriptor-handle if the RHI-supports descriptor-handles */
    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }

    /** @return: Returns the SamplerState description */
    const CRHISamplerStateInitializer& GetInitializer() const { return Initializer; }

protected:
    CRHISamplerStateInitializer Initializer;
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

ENUM_CLASS_OPERATORS(ERayTracingStructureFlag);

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

ENUM_CLASS_OPERATORS(ERayTracingInstanceFlag);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingGeometry

class CRHIRayTracingGeometry : public CRHIResource
{
protected:

    explicit CRHIRayTracingGeometry(ERayTracingStructureFlag InFlags)
        : CRHIResource()
        , Flags(InFlags)
    { }

public:

    /** @return: Returns the Flags of the RayTracingGeometry */
    ERayTracingStructureFlag GetFlags() const { return Flags; }

protected:
    ERayTracingStructureFlag Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingGeometryInstance

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
    CRHIRayTracingGeometryInstance( CRHIRayTracingGeometry* InGeometry
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
    CRHIRayTracingGeometry* Geometry;

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
// CRHIRayTracingScene

class CRHIRayTracingScene : public CRHIResource
{
protected:

    CRHIRayTracingScene(ERayTracingStructureFlag InFlags)
        : CRHIResource()
        , Flags(InFlags)
    { }

public:

    /** @return: Returns a pointer to the ShaderResourceView */
    virtual CRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }

    /** @return: Returns the Bindless descriptor-handle if the RHI supports descriptor-handles */
    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }

    /** @return: Returns the Flags of the RayTracingScene */
    ERayTracingStructureFlag GetFlags() const { return Flags; }

protected:
    ERayTracingStructureFlag Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHITimestamp

struct SRHITimestamp
{
    SRHITimestamp()
        : Begin(0)
        , End(0)
    { }

    SRHITimestamp(uint64 InBegin, uint64 InEnd)
        : Begin(InBegin)
        , End(InEnd)
    { }

    bool operator==(const SRHITimestamp& RHS) const
    {
        return (Begin == RHS.Begin) && (End == RHS.End);
    }

    bool operator!=(const SRHITimestamp& RHS) const
    {
        return !(*this == RHS);
    }

    uint64 Begin;
    uint64 End;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHITimeQuery

class CRHITimeQuery : public CRHIResource
{
protected:

    CRHITimeQuery()
        : CRHIResource()
    { }

public:

    /** @return: Returns the number of timestamps recorded */
    virtual uint32 GetNumTimestamps() const = 0;

    /**
     * @brief: Retrieve a timestamp from the query
     *
     * @param OutQuery: Query to fill out
     * @param Index: Index of the query to fill
     */
    virtual void GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const = 0;

    /** @return: Returns the frequency which is used to convert the timestamps into a time-value */
    virtual uint64 GetFrequency() const = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIViewport

class CRHIViewport : public CRHIResource
{
protected:

    CRHIViewport(ERHIFormat InColorFormat, uint16 InWidth, uint16 InHeight)
        : CRHIResource()
        , Width(InWidth)
        , Height(InHeight)
        , ColorFormat(InColorFormat)
    { }

public:

    /**
     * @brief: Resize the viewport
     *
     * @param InWidth: The new width
     * @param InHeight: The new height
     * @return: Returns true if the resize resulted in the specified width and height
     */
    virtual bool Resize(uint32 Width, uint32 Height) = 0;

    /** @return: Returns the BackBuffer */
    virtual CRHITexture2D* GetBackBuffer() const = 0;

    /** @return: Returns the ColorFormat */
    ERHIFormat GetColorFormat() const { return ColorFormat; }

    /** @return: Returns the Width */
    uint16 GetWidth() const { return Width; }

    /** @return: Returns the Height */
    uint16 GetHeight() const { return Height; }

protected:
    ERHIFormat ColorFormat;

    uint16     Width;
    uint16     Height;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderResourceViewCache

class RHI_API CRHIShaderResourceViewCache
{
public:

    static CRHIShaderResourceViewCache& Get();

    static void Destroy();

    CRHIShaderResourceViewRef GetOrCreateView(const CRHITextureSRVInitializer& Initializer);
    CRHIShaderResourceViewRef GetOrCreateView(const CRHIBufferSRVInitializer& Initializer);

private:

    static TOptional<CRHIShaderResourceViewCache>& GetCacheInstance();

    CRHIShaderResourceViewCache()  = default;
    ~CRHIShaderResourceViewCache() = default;

    TStateHashTable<CRHIShaderResourceViewRef, CRHIBufferSRVInitializer>  Buffers;
    TStateHashTable<CRHIShaderResourceViewRef, CRHITextureSRVInitializer> Textures;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIUnorderedAccessViewCache

class RHI_API CRHIUnorderedAccessViewCache
{
public:

    static CRHIUnorderedAccessViewCache& Get();

    static void Destroy();

    CRHIUnorderedAccessViewRef GetOrCreateView(const CRHITextureUAVInitializer& Initializer);
    CRHIUnorderedAccessViewRef GetOrCreateView(const CRHIBufferUAVInitializer& Initializer);

private:

    static TOptional<CRHIUnorderedAccessViewCache>& GetCacheInstance();

    CRHIUnorderedAccessViewCache() = default;
    ~CRHIUnorderedAccessViewCache() = default;

    TStateHashTable<CRHIUnorderedAccessViewRef, CRHIBufferUAVInitializer>  Buffers;
    TStateHashTable<CRHIUnorderedAccessViewRef, CRHITextureUAVInitializer> Textures;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHISamplerStateCache

class RHI_API CRHISamplerStateCache
{
public:
    static CRHISamplerStateCache& Get();

    static void Destroy();

    CRHISamplerStateRef GetOrCreateSampler(const CRHISamplerStateInitializer& Initializer);

private:
    
    static TOptional<CRHISamplerStateCache>& GetCacheInstance();
    
    CRHISamplerStateCache()  = default;
    ~CRHISamplerStateCache() = default;

    TStateHashTable<CRHISamplerStateRef, CRHISamplerStateInitializer> Samplers;
};