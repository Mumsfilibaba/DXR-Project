#pragma once
#include "RHIResourceBase.h"

enum class EIndexFormat
{
    Unknown = 0,
    uint16 = 1,
    uint32 = 2,
};

inline const char* ToString( EIndexFormat IndexFormat )
{
    switch ( IndexFormat )
    {
        case EIndexFormat::uint16: return "uint16";
        case EIndexFormat::uint32: return "uint32";
        default: return "Unknown";
    }
}

inline EIndexFormat GetIndexFormatFromStride( uint32 StrideInBytes )
{
    if ( StrideInBytes == 2 )
    {
        return EIndexFormat::uint16;
    }
    else if ( StrideInBytes == 4 )
    {
        return EIndexFormat::uint32;
    }
    else
    {
        return EIndexFormat::Unknown;
    }
}

inline uint32 GetStrideFromIndexFormat( EIndexFormat IndexFormat )
{
    if ( IndexFormat == EIndexFormat::uint16 )
    {
        return 2;
    }
    else if ( IndexFormat == EIndexFormat::uint32 )
    {
        return 4;
    }
    else
    {
        return 0;
    }
}

enum EBufferFlags : uint32
{
    BufferFlag_None = 0,
    BufferFlag_Default = FLAG( 1 ), // Default Device Memory
    BufferFlag_Upload = FLAG( 2 ), // Upload Memory
    BufferFlag_UAV = FLAG( 3 ), // Can be used in UseUnorderedAccessViews
    BufferFlag_SRV = FLAG( 4 ), // Can be used in UseShaderResourceViews
    BufferFlags_RWBuffer = BufferFlag_UAV | BufferFlag_SRV
};

class CRHIBuffer : public CRHIResource
{
public:
    CRHIBuffer( uint32 InFlags )
        : CRHIResource()
        , Flags( InFlags )
    {
    }

    virtual class CRHIVertexBuffer* AsVertexBuffer()
    {
        return nullptr;
    }

    virtual class CRHIIndexBuffer* AsIndexBuffer()
    {
        return nullptr;
    }

    virtual class CRHIConstantBuffer* AsConstantBuffer()
    {
        return nullptr;
    }

    virtual class CRHIStructuredBuffer* AsStructuredBuffer()
    {
        return nullptr;
    }

    virtual void* Map( uint32 Offset, uint32 Size ) = 0;
    virtual void  Unmap( uint32 Offset, uint32 Size ) = 0;

    FORCEINLINE uint32 GetFlags() const
    {
        return Flags;
    }

    FORCEINLINE bool IsUpload() const
    {
        return (Flags & BufferFlag_Upload);
    }

    FORCEINLINE bool IsUAV() const
    {
        return (Flags & BufferFlag_UAV);
    }

    FORCEINLINE bool IsSRV() const
    {
        return (Flags & BufferFlag_SRV);
    }

private:
    uint32 Flags;
};

class CRHIVertexBuffer : public CRHIBuffer
{
public:
    CRHIVertexBuffer( uint32 InNumVertices, uint32 InStride, uint32 InFlags )
        : CRHIBuffer( InFlags )
        , NumVertices( InNumVertices )
        , Stride( InStride )
    {
    }

    virtual CRHIVertexBuffer* AsVertexBuffer() override
    {
        return this;
    }

    FORCEINLINE uint32 GetStride() const
    {
        return Stride;
    }

    FORCEINLINE uint32 GetNumVertices() const
    {
        return NumVertices;
    }

private:
    uint32 NumVertices;
    uint32 Stride;
};

class CRHIIndexBuffer : public CRHIBuffer
{
public:
    CRHIIndexBuffer( EIndexFormat InFormat, uint32 InNumIndicies, uint32 InFlags )
        : CRHIBuffer( InFlags )
        , Format( InFormat )
        , NumIndicies( InNumIndicies )
    {
    }

    virtual CRHIIndexBuffer* AsIndexBuffer() override
    {
        return this;
    }

    FORCEINLINE EIndexFormat GetFormat() const
    {
        return Format;
    }
    
    FORCEINLINE uint32 GetNumIndicies() const
    {
        return NumIndicies;
    }

private:
    EIndexFormat Format;
    uint32       NumIndicies;
};

class CRHIConstantBuffer : public CRHIBuffer
{
public:
    CRHIConstantBuffer( uint32 InSize, uint32 InFlags )
        : CRHIBuffer( InFlags )
        , Size( InSize )
    {
    }

    virtual CRHIConstantBuffer* AsConstantBuffer() override
    {
        return this;
    }

    FORCEINLINE uint32 GetSize() const
    {
        return Size;
    }

private:
    uint32 Size;
};

class CRHIStructuredBuffer : public CRHIBuffer
{
public:
    CRHIStructuredBuffer( uint32 InNumElements, uint32 InStride, uint32 InFlags )
        : CRHIBuffer( InFlags )
        , Stride( InStride )
        , NumElements( InNumElements )
    {
    }

    virtual CRHIStructuredBuffer* AsStructuredBuffer() override
    {
        return this;
    }

    FORCEINLINE uint32 GetStride() const
    {
        return Stride;
    }

    FORCEINLINE uint32 GetNumElements() const
    {
        return NumElements;
    }

private:
    uint32 Stride;
    uint32 NumElements;
};