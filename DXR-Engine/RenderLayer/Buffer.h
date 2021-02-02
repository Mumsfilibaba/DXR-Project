#pragma once
#include "ResourceBase.h"

enum class EIndexFormat
{
    Unknown = 0,
    UInt16  = 1,
    UInt32  = 2,
};

inline const Char* ToString(EIndexFormat IndexFormat)
{
    switch (IndexFormat)
    {
    case EIndexFormat::UInt16: return "UInt16";
    case EIndexFormat::UInt32: return "UInt32";
    default: return "Unknown";
    }
}

inline EIndexFormat GetIndexFormatFromStride(UInt32 StrideInBytes)
{
    if (StrideInBytes == 2)
    {
        return EIndexFormat::UInt16;
    }
    else if (StrideInBytes == 4)
    {
        return EIndexFormat::UInt32;
    }
    else
    {
        return EIndexFormat::Unknown;
    }
}

inline UInt32 GetStrideFromIndexFormat(EIndexFormat IndexFormat)
{
    if (IndexFormat == EIndexFormat::UInt16)
    {
        return 2;
    }
    else if (IndexFormat == EIndexFormat::UInt32)
    {
        return 4;
    }
    else
    {
        return 0;
    }
}

enum EBufferFlags : UInt32
{
    BufferFlag_None    = 0,
    BufferFlag_Default = FLAG(1), // Default Device Memory
    BufferFlag_Upload  = FLAG(2), // Upload Memory
    BufferFlag_UAV     = FLAG(3), // Can be used in UnorderedAccessViews
    BufferFlag_SRV     = FLAG(4), // Can be used in ShaderResourceViews
};

class Buffer : public Resource
{
public:
    Buffer(UInt32 InFlags)
        : Resource()
        , Flags(InFlags)
    {
    }

    ~Buffer() = default;

    virtual class VertexBuffer* AsVertexBuffer() { return nullptr; }
    virtual class IndexBuffer* AsIndexBuffer() { return nullptr; }
    virtual class ConstantBuffer* AsConstantBuffer() { return nullptr; }
    virtual class StructuredBuffer* AsStructuredBuffer() { return nullptr; }

    virtual void* Map(UInt32 Offset, UInt32 Size) = 0;
    virtual void  Unmap(UInt32 Offset, UInt32 Size) = 0;

    UInt32 GetFlags() const { return Flags; }

    Bool IsUpload() const { return (Flags & BufferFlag_Upload); }
    Bool IsUAV() const { return (Flags & BufferFlag_UAV); }
    Bool IsSRV() const { return (Flags & BufferFlag_SRV); }

private:
    UInt32 Flags;
};

class VertexBuffer : public Buffer
{
public:
    VertexBuffer(UInt32 InNumVertices, UInt32 InStride, UInt32 InFlags)
        : Buffer(InFlags)
        , Stride(InStride)
        , NumVertices(InNumVertices)
    {
    }

    ~VertexBuffer() = default;

    virtual VertexBuffer* AsVertexBuffer() override { return this; }

    UInt32 GetStride() const { return Stride; }
    UInt32 GetNumVertices() const { return NumVertices; }

private:
    UInt32 NumVertices;
    UInt32 Stride;
};

class IndexBuffer : public Buffer
{
public:
    IndexBuffer(EIndexFormat InFormat, UInt32 InNumIndicies, UInt32 InFlags)
        : Buffer(InFlags)
        , Format(InFormat)
        , NumIndicies(InNumIndicies)
    {
    }

    ~IndexBuffer() = default;

    virtual IndexBuffer* AsIndexBuffer() override { return this; }

    EIndexFormat GetFormat() const { return Format; }
    UInt32 GetNumIndicies() const { return NumIndicies; }

private:
    EIndexFormat Format;
    UInt32       NumIndicies;
};

class ConstantBuffer : public Buffer
{
public:
    ConstantBuffer(UInt32 InSizeInBytes, UInt32 InFlags)
        : Buffer(InFlags)
        , SizeInBytes(InSizeInBytes)
    {
    }

    virtual ConstantBuffer* AsConstantBuffer() override { return this; }

    UInt32 GetSizeInBytes() const { return SizeInBytes; }

private:
    UInt32 SizeInBytes;
};

class StructuredBuffer : public Buffer
{
public:
    StructuredBuffer(UInt32 InNumElements, UInt32 InStride, UInt32 InFlags)
        : Buffer(InFlags)
        , Stride(InStride)
        , NumElements(InNumElements)
    {
    }

    ~StructuredBuffer() = default;

    virtual StructuredBuffer* AsStructuredBuffer() override { return this; }

    UInt32 GetStride() const { return Stride; }
    UInt32 GetNumElements() const { return NumElements; }

private:
    UInt32 Stride;
    UInt32 NumElements;
};