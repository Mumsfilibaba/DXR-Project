#pragma once
#include "Resource.h"
#include "ResourceViews.h"

class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;
class StructuredBuffer;

enum EBufferUsage : UInt32
{
    BufferUsage_None    = 0,
    BufferUsage_Default = FLAG(1), // GPU Memory
    BufferUsage_Dynamic = FLAG(2), // CPU Memory
    BufferUsage_UAV     = FLAG(3), // Can be used in UnorderedAccessViews
    BufferUsage_SRV     = FLAG(4), // Can be used in ShaderResourceViews
};

struct Range
{
    Range() = default;

    Range(UInt32 InOffset, UInt32 InSize)
        : Offset(InOffset)
        , Size(InSize)
    {
    }

    UInt32 Offset = 0;
    UInt32 Size   = 0;
};

class Buffer : public Resource
{
public:
    Buffer(UInt32 InSizeInBytes, UInt32 InUsage)
        : SizeInBytes(InSizeInBytes)
        , Usage(InUsage)
    {
    }

    virtual Buffer* AsBuffer() override
    {
        return this;
    }

    virtual const Buffer* AsBuffer() const override
    {
        return this;
    }

    virtual VertexBuffer* AsVertexBuffer()
    {
        return nullptr;
    }

    virtual const VertexBuffer* AsVertexBuffer() const
    {
        return nullptr;
    }

    virtual IndexBuffer* AsIndexBuffer()
    {
        return nullptr;
    }

    virtual const IndexBuffer* AsIndexBuffer() const
    {
        return nullptr;
    }

    virtual ConstantBuffer* AsConstantBuffer()
    {
        return nullptr;
    }

    virtual const ConstantBuffer* AsConstantBuffer() const
    {
        return nullptr;
    }

    virtual StructuredBuffer* AsStructuredBuffer()
    {
        return nullptr;
    }

    virtual const StructuredBuffer* AsStructuredBuffer() const
    {
        return nullptr;
    }

    virtual UInt64 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

    virtual UInt64 GetRequiredAlignment() const
    {
        return 0;
    }

    virtual Void* Map(const Range* MappedRange)    = 0;
    virtual void  Unmap(const Range* WrittenRange) = 0;

    FORCEINLINE UInt32 GetUsage() const
    {
        return Usage;
    }

    FORCEINLINE bool HasShaderResourceUsage() const
    {
        return Usage & BufferUsage_SRV;
    }

    FORCEINLINE bool HasUnorderedAccessUsage() const
    {
        return Usage & BufferUsage_UAV;
    }

    FORCEINLINE bool HasDynamicUsage() const
    {
        return Usage & BufferUsage_Dynamic;
    }

protected:
    UInt32 SizeInBytes;
    UInt32 Usage;
};

class VertexBuffer : public Buffer
{
public:
    VertexBuffer(UInt32 InSizeInBytes, UInt32 InStride, UInt32 Usage)
        : Buffer(InSizeInBytes, Usage)
        , Stride(InStride)
        , NumElements(InSizeInBytes / InStride)
    {
    }

    virtual VertexBuffer* AsVertexBuffer() override
    {
        return this;
    }

    virtual const VertexBuffer* AsVertexBuffer() const override
    {
        return this;
    }

    FORCEINLINE UInt32 GetStride() const
    {
        return Stride;
    }

    FORCEINLINE UInt32 GetNumElements() const
    {
        return NumElements;
    }

protected:
    UInt32 Stride;
    Int32 NumElements;
};

enum class EIndexFormat
{
    IndexFormat_UInt16 = 1,
    IndexFormat_UInt32 = 2
};

class IndexBuffer : public Buffer
{
public:
    inline IndexBuffer(UInt32 InSizeInBytes, EIndexFormat InIndexFormat, UInt32 Usage)
        : Buffer(InSizeInBytes, Usage)
        , IndexFormat(InIndexFormat)
        , NumElements(InSizeInBytes / (InIndexFormat == EIndexFormat::IndexFormat_UInt32 ? 4 : 2))
    {
    }
    
    virtual IndexBuffer* AsIndexBuffer() override
    {
        return this;
    }

    virtual const IndexBuffer* AsIndexBuffer() const override
    {
        return this;
    }

    FORCEINLINE EIndexFormat GetIndexFormat() const
    {
        return IndexFormat;
    }

    FORCEINLINE UInt32 GetNumElements() const
    {
        return NumElements;
    }

protected:
    EIndexFormat IndexFormat;
    UInt32 NumElements;
};

class ConstantBuffer : public Buffer
{
public:
    ConstantBuffer(UInt32 SizeInBytes, UInt32 Usage)
        : Buffer(SizeInBytes, Usage)
    {
    }
    
    virtual ConstantBuffer* AsConstantBuffer() override
    {
        return this;
    }

    virtual const ConstantBuffer* AsConstantBuffer() const override
    {
        return this;
    }
};

class StructuredBuffer : public Buffer
{
public:
    StructuredBuffer(UInt32 InSizeInBytes, UInt32 InStride, UInt32 Usage)
        : Buffer(InSizeInBytes, Usage)
        , Stride(InStride)
        , NumElements(InSizeInBytes / InStride)
    {
    }

    virtual StructuredBuffer* AsStructuredBuffer() override
    {
        return this;
    }

    virtual const StructuredBuffer* AsStructuredBuffer() const override
    {
        return this;
    }

    FORCEINLINE UInt32 GetStride() const
    {
        return Stride;
    }

    FORCEINLINE UInt32 GetNumElements() const
    {
        return NumElements;
    }

protected:
    UInt32 Stride;
    UInt32 NumElements;
};