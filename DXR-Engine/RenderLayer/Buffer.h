#pragma once
#include "ResourceBase.h"

enum class EIndexFormat
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
    default: return "Unknown";
    }
}

inline EIndexFormat GetIndexFormatFromStride(uint32 StrideInBytes)
{
    if (StrideInBytes == 2)
    {
        return EIndexFormat::uint16;
    }
    else if (StrideInBytes == 4)
    {
        return EIndexFormat::uint32;
    }
    else
    {
        return EIndexFormat::Unknown;
    }
}

inline uint32 GetStrideFromIndexFormat(EIndexFormat IndexFormat)
{
    if (IndexFormat == EIndexFormat::uint16)
    {
        return 2;
    }
    else if (IndexFormat == EIndexFormat::uint32)
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
    BufferFlag_None    = 0,
    BufferFlag_Default = FLAG(1), // Default Device Memory
    BufferFlag_Upload  = FLAG(2), // Upload Memory
    BufferFlag_UAV     = FLAG(3), // Can be used in UseUnorderedAccessViews
    BufferFlag_SRV     = FLAG(4), // Can be used in UseShaderResourceViews
    BufferFlags_RWBuffer = BufferFlag_UAV | BufferFlag_SRV
};

class Buffer : public Resource
{
public:
    Buffer(uint32 InFlags)
        : Resource()
        , Flags(InFlags)
    {
    }

    virtual class VertexBuffer*     AsVertexBuffer()     { return nullptr; }
    virtual class IndexBuffer*      AsIndexBuffer()      { return nullptr; }
    virtual class ConstantBuffer*   AsConstantBuffer()   { return nullptr; }
    virtual class StructuredBuffer* AsStructuredBuffer() { return nullptr; }

    virtual void* Map(uint32 Offset, uint32 Size)   = 0;
    virtual void  Unmap(uint32 Offset, uint32 Size) = 0;

    uint32 GetFlags() const { return Flags; }

    bool IsUpload() const { return (Flags & BufferFlag_Upload); }
    bool IsUAV() const    { return (Flags & BufferFlag_UAV); }
    bool IsSRV() const    { return (Flags & BufferFlag_SRV); }

private:
    uint32 Flags;
};

class VertexBuffer : public Buffer
{
public:
    VertexBuffer(uint32 InNumVertices, uint32 InStride, uint32 InFlags)
        : Buffer(InFlags)
        , Stride(InStride)
        , NumVertices(InNumVertices)
    {
    }

    virtual VertexBuffer* AsVertexBuffer() override { return this; }

    uint32 GetStride() const      { return Stride; }
    uint32 GetNumVertices() const { return NumVertices; }

private:
    uint32 NumVertices;
    uint32 Stride;
};

class IndexBuffer : public Buffer
{
public:
    IndexBuffer(EIndexFormat InFormat, uint32 InNumIndicies, uint32 InFlags)
        : Buffer(InFlags)
        , Format(InFormat)
        , NumIndicies(InNumIndicies)
    {
    }

    virtual IndexBuffer* AsIndexBuffer() override { return this; }

    EIndexFormat GetFormat() const      { return Format; }
    uint32       GetNumIndicies() const { return NumIndicies; }

private:
    EIndexFormat Format;
    uint32       NumIndicies;
};

class ConstantBuffer : public Buffer
{
public:
    ConstantBuffer(uint32 InSize, uint32 InFlags)
        : Buffer(InFlags)
        , Size(InSize)
    {
    }

    virtual ConstantBuffer* AsConstantBuffer() override { return this; }

    uint32 GetSize() const { return Size; }

private:
    uint32 Size;
};

class StructuredBuffer : public Buffer
{
public:
    StructuredBuffer(uint32 InNumElements, uint32 InStride, uint32 InFlags)
        : Buffer(InFlags)
        , Stride(InStride)
        , NumElements(InNumElements)
    {
    }

    virtual StructuredBuffer* AsStructuredBuffer() override { return this; }

    uint32 GetStride()      const { return Stride; }
    uint32 GetNumElements() const { return NumElements; }

private:
    uint32 Stride;
    uint32 NumElements;
};