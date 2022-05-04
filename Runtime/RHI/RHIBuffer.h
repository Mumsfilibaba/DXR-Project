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
        case EIndexFormat::uint16: return 2;
        case EIndexFormat::uint32: return 4;
        default:                   return 0;
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EBufferUsageFlags 

enum class EBufferUsageFlags : uint8
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
// CRHIBufferDataInitializer

class CRHIBufferDataInitializer
{
public:
    
    CRHIBufferDataInitializer()
        : BufferData(nullptr)
        , Size(0)
    { }

    explicit CRHIBufferDataInitializer(const void* InBufferData, uint32 InSize)
        : BufferData(InBufferData)
        , Size(InSize)
    { }

    bool operator==(const CRHIBufferDataInitializer& RHS) const
    {
        return (BufferData == RHS.BufferData) && (Size == RHS.Size);
    }

    bool operator!=(const CRHIBufferDataInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    const void* BufferData;
    uint32      Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBufferInitializer

class CRHIBufferInitializer
{
public:

    CRHIBufferInitializer()
        : UsageFlags(EBufferUsageFlags::None)
        , InitialAccess(EResourceAccess::Common)
        , InitialData(nullptr)
    { }

    CRHIBufferInitializer(EBufferUsageFlags InUsageFlags, EResourceAccess InInitialState, CRHIBufferDataInitializer* InInitialData = nullptr)
        : UsageFlags(InUsageFlags)
        , InitialAccess(InInitialState)
        , InitialData(InInitialData)
    { }

    bool AllowSRV() const { return ((UsageFlags & EBufferUsageFlags::AllowSRV) != EBufferUsageFlags::None); }

    bool AllowUAV() const { return ((UsageFlags & EBufferUsageFlags::AllowUAV) != EBufferUsageFlags::None); }

    bool IsDynamic() const { return ((UsageFlags & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None); }

    bool operator==(const CRHIBufferInitializer& RHS) const
    {
        return (UsageFlags == RHS.UsageFlags) && (InitialAccess == RHS.InitialAccess);
    }

    bool operator!=(const CRHIBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIBufferDataInitializer* InitialData;

    EBufferUsageFlags          UsageFlags;
    EResourceAccess            InitialAccess;
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

    CRHIVertexBufferInitializer( EBufferUsageFlags InUsageFlags
                               , uint32 InNumVertices
                               , uint16 InStride
                               , EResourceAccess InInitialState = EResourceAccess::VertexAndConstantBuffer
                               , CRHIBufferDataInitializer* InInitialData = nullptr)
        : CRHIBufferInitializer(InUsageFlags, InInitialState, InInitialData)
        , NumVertices(InNumVertices)
        , Stride(InStride)
    { }

    uint32 GetSize() const { return NumVertices * GetStride(); }

    uint32 GetStride() const { return Stride; }

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
// CRHIIndexBufferInitializer

class CRHIIndexBufferInitializer : public CRHIBufferInitializer
{
public:

    CRHIIndexBufferInitializer()
        : CRHIBufferInitializer()
        , IndexFormat(EIndexFormat::Unknown)
        , NumIndicies(0)
    { }

    CRHIIndexBufferInitializer( EBufferUsageFlags InUsageFlags
                              , EIndexFormat InIndexFormat
                              , uint32 InNumIndicies
                              , EResourceAccess InInitialState = EResourceAccess::IndexBuffer
                              , CRHIBufferDataInitializer* InInitialData = nullptr)
        : CRHIBufferInitializer(InUsageFlags, InInitialState, InInitialData)
        , IndexFormat(InIndexFormat)
        , NumIndicies(InNumIndicies)
    { }

    uint32 GetSize() const { return NumIndicies * GetStride(); }

    uint32 GetStride() const { return GetStrideFromIndexFormat(IndexFormat); }

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
// CRHIGenericBufferInitializer

class CRHIGenericBufferInitializer : public CRHIBufferInitializer
{
public:

    CRHIGenericBufferInitializer()
        : CRHIBufferInitializer()
        , Size(0)
        , Stride(0)
    { }

    CRHIGenericBufferInitializer( EBufferUsageFlags InUsageFlags
                                , uint32 InSize
                                , EResourceAccess InInitialState = EResourceAccess::Common
                                , CRHIBufferDataInitializer* InInitialData = nullptr)
        : CRHIBufferInitializer(InUsageFlags, InInitialState)
        , Size(InSize)
        , Stride(InSize)
    { }

    CRHIGenericBufferInitializer( EBufferUsageFlags InUsageFlags
                                , uint32 InNumElements
                                , uint32 InStride
                                , EResourceAccess InInitialState = EResourceAccess::Common
                                , CRHIBufferDataInitializer* InInitialData = nullptr)
        : CRHIBufferInitializer(InUsageFlags, InInitialState)
        , Size(InNumElements * InStride)
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
// CRHIConstantBufferInitializer

class CRHIConstantBufferInitializer : public CRHIBufferInitializer
{
public:

    CRHIConstantBufferInitializer()
        : CRHIBufferInitializer()
        , Size(0)
        , Stride(0)
    { }

    CRHIConstantBufferInitializer( EBufferUsageFlags InUsageFlags
                                 , uint32 InSize
                                 , EResourceAccess InInitialState = EResourceAccess::VertexAndConstantBuffer
                                 , CRHIBufferDataInitializer* InInitialData = nullptr)
        : CRHIBufferInitializer(InUsageFlags, InInitialState, InInitialData)
        , Size(InSize)
        , Stride(InSize)
    { }

    CRHIConstantBufferInitializer( EBufferUsageFlags InUsageFlags
                                 , uint32 InStride
                                 , uint32 InNumElements
                                 , EResourceAccess InInitialState = EResourceAccess::VertexAndConstantBuffer
                                 , CRHIBufferDataInitializer* InInitialData = nullptr)
        : CRHIBufferInitializer(InUsageFlags, InInitialState, InInitialData)
        , Size(InNumElements * InStride)
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
// CRHIBuffer

class CRHIBuffer : public CRHIResource
{
protected:

    explicit CRHIBuffer(const CRHIBufferInitializer& Initializer)
        : CRHIResource()
        , Flags(Initializer.UsageFlags)
    { }

public:

    virtual class CRHIVertexBuffer* GetVertexBuffer() { return nullptr; }

    virtual class CRHIIndexBuffer* GetIndexBuffer() { return nullptr; }

    virtual class CRHIConstantBuffer* GetConstantBuffer() { return nullptr; }

    virtual class CRHIGenericBuffer* GetGenericBuffer() { return nullptr; }

    virtual void* GetRHIBaseBuffer() { return nullptr; }

    virtual void* GetRHIBaseResource() const { return nullptr; }

    virtual bool IsStructured() const { return false; }

    virtual uint32 GetSize() const { return 1; }

    virtual uint32 GetStride() const { return 1; }

    virtual void SetName(const String& InName) { }

    virtual String GetName() const { return ""; }

    EBufferUsageFlags GetFlags() const { return Flags; }

private:
    EBufferUsageFlags Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexBuffer

class CRHIVertexBuffer : public CRHIBuffer
{
protected:

    explicit CRHIVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
        : CRHIBuffer(Initializer)
        , NumVertices(Initializer.NumVertices)
        , Stride(Initializer.Stride)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual CRHIVertexBuffer* GetVertexBuffer() override final { return this; }

    virtual bool IsStructured() const override final { return true; }

    virtual uint32 GetSize() const override final { return GetStride() * NumVertices; }

    virtual uint32 GetStride() const override final { return Stride; }

public:

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

    explicit CRHIIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
        : CRHIBuffer(Initializer)
        , Format(Initializer.IndexFormat)
        , NumIndicies(Initializer.NumIndicies)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual CRHIIndexBuffer* GetIndexBuffer() override final { return this; }

    virtual uint32 GetSize() const override final { return GetStride() * NumIndicies; }

    virtual uint32 GetStride() const override final { return GetStrideFromIndexFormat(Format); }

public:

    EIndexFormat GetFormat() const { return Format; }

    uint32 GetNumIndicies() const { return NumIndicies; }

private:
    EIndexFormat Format;
    uint32       NumIndicies;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGenericBuffer

class CRHIGenericBuffer : public CRHIBuffer
{
protected:

    explicit CRHIGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
        : CRHIBuffer(Initializer)
        , Stride(Initializer.Stride)
        , Size(Initializer.Size)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual CRHIGenericBuffer* GetGenericBuffer() override final { return this; }

    virtual bool IsStructured() const override final { return true; }

    virtual uint32 GetSize() const override final { return Size; }

    virtual uint32 GetStride() const override final { return Stride; }

private:
    uint32 Stride;
    uint32 Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIConstantBuffer

class CRHIConstantBuffer : public CRHIBuffer
{
protected:

    explicit CRHIConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
        : CRHIBuffer(Initializer)
        , Size(Initializer.Size)
        , Stride(Initializer.Stride)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual CRHIConstantBuffer* GetConstantBuffer() override final { return this; }

    virtual uint32 GetSize() const override final { return Size; }

    virtual uint32 GetStride() const override final { return Stride; }

public:

    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }

private:
    uint32 Size;
    uint32 Stride;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
