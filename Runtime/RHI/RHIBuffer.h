#pragma once
#include "RHIResourceBase.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIIndexFormat 

enum class ERHIIndexFormat
{
    Unknown = 0,
    uint16 = 1,
    uint32 = 2,
};

inline const char* ToString(ERHIIndexFormat IndexFormat)
{
    switch (IndexFormat)
    {
    case ERHIIndexFormat::uint16: return "uint16";
    case ERHIIndexFormat::uint32: return "uint32";
    default: return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Indices helpers

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

enum ERHIBufferFlags : uint32
{
    BufferFlag_None = 0,
    BufferFlag_Default = FLAG(1), // Default Device Memory
    BufferFlag_Dynamic = FLAG(2), // Dynamic Memory
    BufferFlag_UAV = FLAG(3), // Can be used in UnorderedAccessViews
    BufferFlag_SRV = FLAG(4), // Can be used in ShaderResourceViews

    BufferFlags_RWBuffer = BufferFlag_UAV | BufferFlag_SRV
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBuffer

class CRHIBuffer : public CRHIResource
{
public:

    /**
     * Constructor taking in flags for the buffer
     * 
     * @param InFlags: Flags that the buffer was created with
     */
    CRHIBuffer(uint32 InFlags)
        : CRHIResource()
        , Flags(InFlags)
    {
    }

    /**
     * Cast resource to a Buffer
     * 
     * @return: Returns a pointer to a buffer interface if the object implements it
     */
    virtual CRHIBuffer* AsBuffer() { return this; }

    /**
     * Cast to VertexBuffer 
     * 
     * @return: Returns a pointer to a VertexBuffer interface if the object implements it
     */
    virtual class CRHIVertexBuffer* AsVertexBuffer() { return nullptr; }

    /**
     * Cast to IndexBuffer
     *
     * @return: Returns a pointer to a IndexBuffer interface if the object implements it
     */
    virtual class CRHIIndexBuffer* AsIndexBuffer() { return nullptr; }

    /**
     * Cast to ConstantBuffer
     *
     * @return: Returns a pointer to a ConstantBuffer interface if the object implements it
     */
    virtual class CRHIConstantBuffer* AsConstantBuffer() { return nullptr; }

    /**
     * Cast to StructuredBuffer
     *
     * @return: Returns a pointer to a StructuredBuffer interface if the object implements it
     */
    virtual class CRHIStructuredBuffer* AsStructuredBuffer() { return nullptr; }

    /**
     * Map GPU buffer memory to the CPU. Setting both size and offset to zero indicate the whole resource.
     * 
     * @param Offset: Offset in the buffer were the buffer should start mapping
     * @param Size: Size of the range to map
     * @return: Returns a pointer to the data, or nullptr if mapping was unsuccessful
     */
    virtual void* Map(uint32 Offset = 0, uint32 Size = 0) = 0;

    /**
     * Unmap GPU buffer memory to the CPU. Setting both size and offset to zero indicate the whole resource.
     *
     * @param Offset: Offset in the buffer were the buffer were used on the CPU
     * @param Size: Size of the range that were used on the CPU
     */
    virtual void Unmap(uint32 Offset = 0, uint32 Size = 0) = 0;

    /**
     * Check if the buffer is considered dynamic
     * 
     * @return: Returns true if the buffer is dynamic
     */
    FORCEINLINE bool IsDynamic() const
    {
        return (Flags & BufferFlag_Dynamic);
    }

    /**
     * Check if the buffer can be used with UnorderedAccessViews
     *
     * @return: Returns true if the buffer can be used with UnorderedAccessViews
     */
    FORCEINLINE bool IsUAV() const
    {
        return (Flags & BufferFlag_UAV);
    }

    /**
     * Check if the buffer can be used with ShaderResourceViews
     *
     * @return: Returns true if the buffer can be used with ShaderResourceViews
     */
    FORCEINLINE bool IsSRV() const
    {
        return (Flags & BufferFlag_SRV);
    }

    /**
     * Retrieve the flags that the buffer was created with
     * 
     * @return: Returns the flags that the buffer was created with
     */
    FORCEINLINE uint32 GetFlags() const
    {
        return Flags;
    }

private:
    uint32 Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexBuffer

class CRHIVertexBuffer : public CRHIBuffer
{
public:

    /**
     * Constructor taking the parameters that the buffer was created with
     * 
     * @param InNumVertices: The number of vertices within the buffer
     * @param InStride: Stride of the each vertex
     * @param InFlags: Flags that the buffer was created with
     */
    CRHIVertexBuffer(uint32 InNumVertices, uint32 InStride, uint32 InFlags)
        : CRHIBuffer(InFlags)
        , NumVertices(InNumVertices)
        , Stride(InStride)
    {
    }

    /**
     * Cast to VertexBuffer
     *
     * @return: Returns a pointer to a VertexBuffer interface if the object implements it
     */
    virtual CRHIVertexBuffer* AsVertexBuffer() override { return this; }


    /**
     * Retrieve the stride of the VertexBuffer
     * 
     * @return: Returns the stride for each vertex in the VertexBuffer
     */
    FORCEINLINE uint32 GetStride() const
    {
        return Stride;
    }

    /**
     * Retrieve the number of vertices in the buffer
     *
     * @return: Returns the number of vertices in the VertexBuffer
     */
    FORCEINLINE uint32 GetNumVertices() const
    {
        return NumVertices;
    }

private:
    uint32 NumVertices;
    uint32 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIIndexBuffer

class CRHIIndexBuffer : public CRHIBuffer
{
public:

    /**
     * Constructor taking parameters for the IndexBuffer
     * 
     * @param InFormat: Format that the IndexBuffer uses
     * @param InNumIndices: Number of indices in the IndexBuffer
     * @param InFlags: Flags that the buffer was created with
     */
    CRHIIndexBuffer(ERHIIndexFormat InFormat, uint32 InNumIndicies, uint32 InFlags)
        : CRHIBuffer(InFlags)
        , Format(InFormat)
        , NumIndicies(InNumIndicies)
    {
    }

    /**
     * Cast to IndexBuffer
     *
     * @return: Returns a pointer to a IndexBuffer interface if the object implements it
     */
    virtual CRHIIndexBuffer* AsIndexBuffer() override { return this; }

    /**
     * Retrieve the format that the IndexBuffer was created with
     * 
     * @return: Returns the format that the IndexBuffer uses
     */
    FORCEINLINE ERHIIndexFormat GetFormat() const
    {
        return Format;
    }

    /**
     * Retrieve the number of indices that the IndexBuffer contains
     * 
     * @return: Returns the number of indices in the IndexBuffer
     */
    FORCEINLINE uint32 GetNumIndicies() const
    {
        return NumIndicies;
    }

private:
    ERHIIndexFormat Format;
    uint32       NumIndicies;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIConstantBuffer

class CRHIConstantBuffer : public CRHIBuffer
{
public:

    /**
     * Constructor taking create params
     * 
     * @param InSize: Size of the ConstantBuffer
     * @param InFlags: Flags of the ConstantBuffer
     */
    CRHIConstantBuffer(uint32 InSize, uint32 InFlags)
        : CRHIBuffer(InFlags)
        , Size(InSize)
    {
    }

    /**
     * Cast to ConstantBuffer
     *
     * @return: Returns a pointer to a ConstantBuffer interface if the object implements it
     */
    virtual CRHIConstantBuffer* AsConstantBuffer() override { return this; }

    /**
     * Retrieve the size of the ConstantBuffer
     * 
     * @return: Returns the Size of the ConstantBuffer
     */
    FORCEINLINE uint32 GetSize() const
    {
        return Size;
    }

private:
    uint32 Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIStructuredBuffer

class CRHIStructuredBuffer : public CRHIBuffer
{
public:

    /**
     * Constructor taking parameters for creating a StructuredBuffer
     * 
     * @param InNumElements: Number of elements in the StructuredBuffer
     * @param InStride: Stride of each element in the buffer
     * @param InFlags: Flags for the buffer
     */
    CRHIStructuredBuffer(uint32 InNumElements, uint32 InStride, uint32 InFlags)
        : CRHIBuffer(InFlags)
        , Stride(InStride)
        , NumElements(InNumElements)
    {
    }

    /**
     * Cast to StructuredBuffer
     *
     * @return: Returns a pointer to a StructuredBuffer interface if the object implements it
     */
    virtual CRHIStructuredBuffer* AsStructuredBuffer() override { return this; }

    /**
     * Retrieve the stride of the StructuredBuffer
     * 
     * @return: Returns the stride of the StructuredBuffer
     */
    FORCEINLINE uint32 GetStride() const
    {
        return Stride;
    }

    /**
     * Retrieve the number of elements of the StructuredBuffer
     *
     * @return: Returns the number of elements of the StructuredBuffer
     */
    FORCEINLINE uint32 GetNumElements() const
    {
        return NumElements;
    }

private:
    uint32 Stride;
    uint32 NumElements;
};
