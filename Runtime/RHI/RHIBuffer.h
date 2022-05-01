#pragma once
#include "RHIResourceBase.h"

#include "Core/Templates/EnumUtilities.h"
#include "Core/Containers/SharedRef.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIBuffer>         RHIBufferRef;

typedef TSharedRef<class CRHIVertexBuffer>   RHIVertexBufferRef;
typedef TSharedRef<class CRHIIndexBuffer>    RHIIndexBufferRef;
typedef TSharedRef<class CRHIGenericBuffer>  RHIGenericBufferRef;
typedef TSharedRef<class CRHIConstantBuffer> RHIConstantBufferRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EIndexFormat 

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
    default:                   return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Indices helpers

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EBufferUsageFlags 

enum class EBufferUsageFlags : uint32
{
    None     = 0,
    Default  = FLAG(1), // Default Device Memory
    Dynamic  = FLAG(2), // Dynamic Memory
    AllowUAV = FLAG(3), // Can be used in UnorderedAccessViews
    AllowSRV = FLAG(4), // Can be used in ShaderResourceViews

    RWBuffer = AllowUAV | AllowSRV
};

ENUM_CLASS_OPERATORS(EBufferUsageFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBuffer

class CRHIBuffer : public CRHIResource
{
protected:

    CRHIBuffer(EBufferUsageFlags InFlags)
        : CRHIResource()
        , Flags(InFlags)
    { }

public:

    /** @return: Returns a pointer to a VertexBuffer interface if implemented */
    virtual class CRHIVertexBuffer* GetVertexBuffer() { return nullptr; }

    /** @return: Returns a pointer to a IndexBuffer interface if implemented */
    virtual class CRHIIndexBuffer* GetIndexBuffer() { return nullptr; }

    /** @return: Returns a pointer to a ConstantBuffer interface if implemented */
    virtual class CRHIConstantBuffer* GetConstantBuffer() { return nullptr; }

    /** @return: Returns a pointer to a GenericBuffer interface if implemented */
    virtual class CRHIGenericBuffer* GetGenericBuffer() { return nullptr; }

    /** @return: Returns the native handle of the Buffer */
    virtual void* GetRHIBaseResource() const { return nullptr; }

    /** @return: Returns the RHI-backend buffer interface */
    virtual void* GetRHIBaseBuffer() { return nullptr; }

    /** @return: Returns true if the buffer is structured or not */
    virtual bool IsStructured() const { return false; }

    /** @return: Returns the Buffer Stride */
    virtual uint32 GetStride() const { return 1; }

    /** @return: Returns the Buffer Size */
    virtual uint32 GetSize() const { return 1; }

    /** @brief: Set the debug-name of the Texture */
    virtual void SetName(const String& InName) { }

    /** @return: Returns the name of the Texture */
    virtual String GetName() const { return ""; }

    /** @return: Returns the flags that the buffer was created with */
    EBufferUsageFlags GetFlags() const { return Flags; }

private:
    EBufferUsageFlags Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexBuffer

class CRHIVertexBuffer : public CRHIBuffer
{
protected:

    CRHIVertexBuffer(EBufferUsageFlags InFlags, uint32 InNumVertices, uint32 InStride)
        : CRHIBuffer(InFlags)
        , NumVertices(InNumVertices)
        , Stride(InStride)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual CRHIVertexBuffer* GetVertexBuffer() override final { return this; }

    virtual bool IsStructured() const override final { return true; }

    virtual uint32 GetStride() const override final { return Stride; }

    virtual uint32 GetSize() const override final { return GetStride() * NumVertices; }

public:

    /** @return: Returns the number of vertices in the VertexBuffer */
    uint32 GetNumVertices() const { return NumVertices; }

private:
    uint32 NumVertices;
    uint32 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIIndexBuffer

class CRHIIndexBuffer : public CRHIBuffer
{
protected:

    CRHIIndexBuffer(EBufferUsageFlags InFlags, EIndexFormat InFormat, uint32 InNumIndicies)
        : CRHIBuffer(InFlags)
        , Format(InFormat)
        , NumIndicies(InNumIndicies)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual CRHIIndexBuffer* GetIndexBuffer() override final { return this; }

    virtual uint32 GetStride() const override final { return GetStrideFromIndexFormat(Format); }

    virtual uint32 GetSize() const override final { return GetStride() * NumIndicies; }

public:

    /** @return: Returns the format that the IndexBuffer uses */
    EIndexFormat GetFormat() const { return Format; }

    /** @return: Returns the number of indices in the IndexBuffer */
    uint32 GetNumIndicies() const { return NumIndicies; }

private:
    EIndexFormat Format;
    uint32       NumIndicies;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIConstantBuffer

class CRHIConstantBuffer : public CRHIBuffer
{
protected:

    CRHIConstantBuffer(EBufferUsageFlags InFlags, uint32 InSize)
        : CRHIBuffer(InFlags)
        , Size(InSize)
        , Stride(1)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual CRHIConstantBuffer* GetConstantBuffer() override final { return this; }

    virtual uint32 GetStride() const override final { return Stride; }

    virtual uint32 GetSize() const override final { return Size; }

public:

    /** @return: Returns a Bindless descriptor-handle if the RHI supports it */
    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }

private:
    uint32 Size;
    uint32 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGenericBuffer

class CRHIGenericBuffer : public CRHIBuffer
{
protected:

    CRHIGenericBuffer(EBufferUsageFlags InFlags, uint32 InNumElements, uint32 InStride)
        : CRHIBuffer(InFlags)
        , Stride(InStride)
        , NumElements(InNumElements)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual CRHIGenericBuffer* GetGenericBuffer() override final { return this; }

    virtual uint32 GetStride() const override final { return Stride; }

    virtual uint32 GetSize() const override final { return NumElements * Stride; }

private:
    uint32 Stride;
    uint32 NumElements;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
