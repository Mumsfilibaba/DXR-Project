#pragma once
#include "RHIResourceBase.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class CRHIBuffer>         CRHIBufferRef;
typedef TSharedRef<class CRHIConstantBuffer> CRHIConstantBufferRef;

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
// ERHIBufferFlags

enum ERHIBufferFlags : uint16
{
    BufferFlag_None           = 0,
    
    BufferFlag_Default        = FLAG(1), // Default Device Memory
    BufferFlag_Readback       = FLAG(2), // CPU readable Memory
    BufferFlag_Dynamic        = FLAG(3), // Dynamic Memory

    BufferFlag_ConstantBuffer = FLAG(4), // Can be used as constant buffer
    BufferFlag_VertexBuffer   = FLAG(5), // Can be used as vertex buffer
    BufferFlag_IndexBuffer    = FLAG(6), // Can be used as index buffer
    BufferFlag_UAV            = FLAG(7), // Can be used in UnorderedAccessViews
    BufferFlag_SRV            = FLAG(8), // Can be used in ShaderResourceViews

    BufferFlags_RWBuffer = BufferFlag_UAV | BufferFlag_SRV
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBufferDesc

class CRHIBufferDesc
{
public:
    
    static CRHIBufferDesc CreateConstantBuffer(uint32 InSizeInBytes, uint32 InFlags)
    {
        return CRHIBufferDesc(InSizeInBytes, 1, InFlags | BufferFlag_ConstantBuffer);
    }
    
    static CRHIBufferDesc CreateVertexBuffer(uint32 NumVertices, uint32 VertexStride, uint32 InFlags)
    {
        return CRHIBufferDesc(NumVertices * VertexStride, VertexStride, InFlags | BufferFlag_VertexBuffer);
    }
    
    static CRHIBufferDesc CreateIndexBuffer(uint32 NumIndices, ERHIIndexFormat IndexFormat, uint32 InFlags)
    {
        return CRHIBufferDesc(NumIndices * GetStrideFromIndexFormat(IndexFormat), GetStrideFromIndexFormat(IndexFormat), InFlags | BufferFlag_IndexBuffer);
    }
    
    static CRHIBufferDesc CreateStructuredBuffer(uint32 NumElements, uint32 Stride, uint32 InFlags)
    {
        return CRHIBufferDesc(NumElements * Stride, Stride, InFlags);
    }
    
    static CRHIBufferDesc CreateBufferSRV(uint32 NumElements, uint32 Stride, uint32 InFlags)
    {
        return CRHIBufferDesc(NumElements * Stride, Stride, InFlags | BufferFlag_UAV);
    }
    
    static CRHIBufferDesc CreateBufferUAV(uint32 NumElements, uint32 Stride, uint32 InFlags)
    {
        return CRHIBufferDesc(NumElements * Stride, Stride, InFlags | BufferFlag_SRV);
    }
    
    static CRHIBufferDesc CreateRWBuffer(uint32 NumElements, uint32 Stride, uint32 InFlags)
    {
        return CRHIBufferDesc(NumElements * Stride, Stride, InFlags | BufferFlags_RWBuffer);
    }
    
    CRHIBufferDesc()  = default;
    ~CRHIBufferDesc() = default;
    
    CRHIBufferDesc(uint32 InSizeInBytes, uint32 InStrideInBytes, uint32 InFlags)
        : SizeInBytes(InSizeInBytes)
        , StrideInBytes(InStrideInBytes)
        , Flags(InFlags)
    { }
    
    bool IsUAV() const
    {
        return (Flags & BufferFlag_UAV);
    }
    
    bool IsSRV() const
    {
        return (Flags & BufferFlag_SRV);
    }
    
    bool IsRWBuffer() const
    {
        return (Flags & BufferFlags_RWBuffer);
    }

    bool IsDynamic() const
    {
        return (Flags & BufferFlag_Dynamic);
    }

    bool IsReadBack() const
    {
        return (Flags & BufferFlag_Readback);
    }

    bool IsConstantBuffer() const
    {
        return (Flags & BufferFlag_ConstantBuffer);
    }
    
    bool operator==(const CRHIBufferDesc& RHS) const
    {
        return (Flags == RHS.Flags) && (SizeInBytes == RHS.SizeInBytes) && (StrideInBytes == RHS.StrideInBytes);
    }
    
    bool operator!=(const CRHIBufferDesc& RHS) const
    {
        return !(*this == RHS);
    }
    
    uint32 SizeInBytes   = 0;
    uint16 StrideInBytes = 0;
    uint16 Flags         = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBuffer

class CRHIBuffer : public CRHIResource
{
public:

    CRHIBuffer(const CRHIBufferDesc& InBufferDesc)
        : CRHIResource(ERHIResourceType::Buffer)
        , SizeInBytes(InBufferDesc.SizeInBytes)
        , StrideInBytes(InBufferDesc.StrideInBytes)
        , Flags(InBufferDesc.Flags)
    { }

    virtual CRHIBuffer* AsBuffer() { return this; }

    inline bool IsUAV() const { return (Flags & BufferFlag_UAV); }
    inline bool IsSRV() const { return (Flags & BufferFlag_SRV); }

    inline bool IsDynamic() const { return (Flags & BufferFlag_Dynamic); }

    inline uint32 GetSize()   const { return SizeInBytes; }
    inline uint16 GetStride() const { return StrideInBytes; }
    inline uint16 GetFlags()  const { return Flags; }

protected:
    uint32 SizeInBytes   = 0;
    uint16 StrideInBytes = 0;
    uint16 Flags         = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIConstantBufferFlags

enum ERHIConstantBufferFlags : uint16
{
    ConstantBufferFlag_None = 0,

    ConstantBufferFlag_Default = FLAG(1), // Default Device Memory
    ConstantBufferFlag_Dynamic = FLAG(2), // Dynamic Memory
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIConstantBuffer

class CRHIConstantBuffer : public CRHIResource
{
public:
    CRHIConstantBuffer(uint32 InSize, uint16 InFlags)
        : CRHIResource(ERHIResourceType::ConstantBuffer)
        , Size(InSize)
        , Flags(InFlags)
    { }

    virtual class CRHIConstantBuffer* AsConstantBuffer() { return this; }

    inline bool IsDynamic() const { return Flags & ConstantBufferFlag_Dynamic; }

    inline uint32 GetSize() const { return Size; }
    inline uint16 GetFlags() const { return Flags;}

private:
    uint32 Size; 
    uint16 Flags;
};