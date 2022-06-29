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

typedef TSharedRef<class FRHIBuffer>         FRHIBufferRef;
typedef TSharedRef<class FRHIVertexBuffer>   FRHIVertexBufferRef;
typedef TSharedRef<class FRHIIndexBuffer>    FRHIIndexBufferRef;
typedef TSharedRef<class FRHIGenericBuffer>  FRHIGenericBufferRef;
typedef TSharedRef<class FRHIConstantBuffer> FRHIConstantBufferRef;

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
// FRHIBufferDataInitializer

class FRHIBufferDataInitializer
{
public:
    
    FRHIBufferDataInitializer()
        : BufferData(nullptr)
        , Size(0)
    { }

    explicit FRHIBufferDataInitializer(const void* InBufferData, uint32 InSize)
        : BufferData(InBufferData)
        , Size(InSize)
    { }

    bool operator==(const FRHIBufferDataInitializer& RHS) const
    {
        return (BufferData == RHS.BufferData) && (Size == RHS.Size);
    }

    bool operator!=(const FRHIBufferDataInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    const void* BufferData;
    uint32      Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIBufferInitializer

class FRHIBufferInitializer
{
public:

    FRHIBufferInitializer()
        : InitialData(nullptr)
	    , UsageFlags(EBufferUsageFlags::None)
        , InitialAccess(EResourceAccess::Common)
    { }

    FRHIBufferInitializer(EBufferUsageFlags InUsageFlags, EResourceAccess InInitialState, FRHIBufferDataInitializer* InInitialData = nullptr)
        : InitialData(InInitialData)
	    , UsageFlags(InUsageFlags)
        , InitialAccess(InInitialState)
    { }

    bool AllowSRV() const { return ((UsageFlags & EBufferUsageFlags::AllowSRV) != EBufferUsageFlags::None); }

    bool AllowUAV() const { return ((UsageFlags & EBufferUsageFlags::AllowUAV) != EBufferUsageFlags::None); }

    bool IsDynamic() const { return ((UsageFlags & EBufferUsageFlags::Dynamic) != EBufferUsageFlags::None); }

    bool operator==(const FRHIBufferInitializer& RHS) const
    {
        return (UsageFlags == RHS.UsageFlags) && (InitialAccess == RHS.InitialAccess);
    }

    bool operator!=(const FRHIBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIBufferDataInitializer* InitialData;

    EBufferUsageFlags          UsageFlags;
    EResourceAccess            InitialAccess;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIVertexBufferInitializer

class FRHIVertexBufferInitializer : public FRHIBufferInitializer
{
public:

    FRHIVertexBufferInitializer()
        : FRHIBufferInitializer()
        , NumVertices(0)
        , Stride(0)
    { }

    FRHIVertexBufferInitializer( EBufferUsageFlags InUsageFlags
                               , uint32 InNumVertices
                               , uint16 InStride
                               , EResourceAccess InInitialState = EResourceAccess::VertexAndConstantBuffer
                               , FRHIBufferDataInitializer* InInitialData = nullptr)
        : FRHIBufferInitializer(InUsageFlags, InInitialState, InInitialData)
        , NumVertices(InNumVertices)
        , Stride(InStride)
    { }

    uint32 GetSize() const { return NumVertices * GetStride(); }

    uint32 GetStride() const { return Stride; }

    bool operator==(const FRHIVertexBufferInitializer& RHS) const
    {
        return FRHIBufferInitializer::operator==(RHS)
            && (NumVertices == RHS.NumVertices)
            && (Stride      == RHS.Stride);
    }

    bool operator!=(const FRHIVertexBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint32 NumVertices;
    uint16 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIIndexBufferInitializer

class FRHIIndexBufferInitializer : public FRHIBufferInitializer
{
public:

    FRHIIndexBufferInitializer()
        : FRHIBufferInitializer()
        , IndexFormat(EIndexFormat::Unknown)
        , NumIndicies(0)
    { }

    FRHIIndexBufferInitializer( EBufferUsageFlags InUsageFlags
                              , EIndexFormat InIndexFormat
                              , uint32 InNumIndicies
                              , EResourceAccess InInitialState = EResourceAccess::IndexBuffer
                              , FRHIBufferDataInitializer* InInitialData = nullptr)
        : FRHIBufferInitializer(InUsageFlags, InInitialState, InInitialData)
        , IndexFormat(InIndexFormat)
        , NumIndicies(InNumIndicies)
    { }

    uint32 GetSize() const { return NumIndicies * GetStride(); }

    uint32 GetStride() const { return GetStrideFromIndexFormat(IndexFormat); }

    bool operator==(const FRHIIndexBufferInitializer& RHS) const
    {
        return FRHIBufferInitializer::operator==(RHS)
            && (IndexFormat == RHS.IndexFormat)
            && (NumIndicies == RHS.NumIndicies);
    }

    bool operator!=(const FRHIIndexBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    EIndexFormat IndexFormat;
    uint32       NumIndicies;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIGenericBufferInitializer

class FRHIGenericBufferInitializer : public FRHIBufferInitializer
{
public:

    FRHIGenericBufferInitializer()
        : FRHIBufferInitializer()
        , Size(0)
        , Stride(0)
    { }

    FRHIGenericBufferInitializer( EBufferUsageFlags InUsageFlags
                                , uint32 InSize
                                , EResourceAccess InInitialState = EResourceAccess::Common
                                , FRHIBufferDataInitializer* InInitialData = nullptr)
        : FRHIBufferInitializer(InUsageFlags, InInitialState)
        , Size(InSize)
        , Stride(InSize)
    { }

    FRHIGenericBufferInitializer( EBufferUsageFlags InUsageFlags
                                , uint32 InNumElements
                                , uint32 InStride
                                , EResourceAccess InInitialState = EResourceAccess::Common
                                , FRHIBufferDataInitializer* InInitialData = nullptr)
        : FRHIBufferInitializer(InUsageFlags, InInitialState)
        , Size(InNumElements * InStride)
        , Stride(InStride)
    { }

    bool operator==(const FRHIGenericBufferInitializer& RHS) const
    {
        return FRHIBufferInitializer::operator==(RHS)
            && (Size   == RHS.Size)
            && (Stride == RHS.Stride);
    }

    bool operator!=(const FRHIGenericBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint32 Size;
    uint32 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIConstantBufferInitializer

class FRHIConstantBufferInitializer : public FRHIBufferInitializer
{
public:

    FRHIConstantBufferInitializer()
        : FRHIBufferInitializer()
        , Size(0)
        , Stride(0)
    { }

    FRHIConstantBufferInitializer( EBufferUsageFlags InUsageFlags
                                 , uint32 InSize
                                 , EResourceAccess InInitialState = EResourceAccess::VertexAndConstantBuffer
                                 , FRHIBufferDataInitializer* InInitialData = nullptr)
        : FRHIBufferInitializer(InUsageFlags, InInitialState, InInitialData)
        , Size(InSize)
        , Stride(InSize)
    { }

    FRHIConstantBufferInitializer( EBufferUsageFlags InUsageFlags
                                 , uint32 InStride
                                 , uint32 InNumElements
                                 , EResourceAccess InInitialState = EResourceAccess::VertexAndConstantBuffer
                                 , FRHIBufferDataInitializer* InInitialData = nullptr)
        : FRHIBufferInitializer(InUsageFlags, InInitialState, InInitialData)
        , Size(InNumElements * InStride)
        , Stride(InStride)
    { }

    bool operator==(const FRHIConstantBufferInitializer& RHS) const
    {
        return FRHIBufferInitializer::operator==(RHS)
            && (Size   == RHS.Size)
            && (Stride == RHS.Stride);
    }

    bool operator!=(const FRHIConstantBufferInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    uint32 Size;
    uint32 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIBuffer

class FRHIBuffer : public FRHIResource
{
protected:

    explicit FRHIBuffer(const FRHIBufferInitializer& Initializer)
        : FRHIResource()
        , Flags(Initializer.UsageFlags)
    { }

public:

    virtual class FRHIVertexBuffer* GetVertexBuffer() { return nullptr; }

    virtual class FRHIIndexBuffer* GetIndexBuffer() { return nullptr; }

    virtual class FRHIConstantBuffer* GetConstantBuffer() { return nullptr; }

    virtual class FRHIGenericBuffer* GetGenericBuffer() { return nullptr; }

    virtual void* GetRHIBaseBuffer() { return nullptr; }

    virtual void* GetRHIBaseResource() const { return nullptr; }

    virtual uint32 GetSize() const { return 1; }

    virtual uint32 GetStride() const { return 1; }

    virtual void SetName(const String& InName) { }

    virtual String GetName() const { return ""; }

public:
    
    EBufferUsageFlags GetFlags() const { return Flags; }

private:
    EBufferUsageFlags Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIVertexBuffer

class FRHIVertexBuffer : public FRHIBuffer
{
protected:

    explicit FRHIVertexBuffer(const FRHIVertexBufferInitializer& Initializer)
        : FRHIBuffer(Initializer)
        , NumVertices(Initializer.NumVertices)
        , Stride(Initializer.Stride)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIBuffer Interface

    virtual FRHIVertexBuffer* GetVertexBuffer() override final { return this; }

    virtual uint32 GetSize() const override final { return GetStride() * NumVertices; }

    virtual uint32 GetStride() const override final { return Stride; }

public:

    uint32 GetNumVertices() const { return NumVertices; }

private:
    uint32 NumVertices;
    uint32 Stride;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIIndexBuffer

class FRHIIndexBuffer : public FRHIBuffer
{
protected:

    explicit FRHIIndexBuffer(const FRHIIndexBufferInitializer& Initializer)
        : FRHIBuffer(Initializer)
        , Format(Initializer.IndexFormat)
        , NumIndicies(Initializer.NumIndicies)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIBuffer Interface

    virtual FRHIIndexBuffer* GetIndexBuffer() override final { return this; }

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
// FRHIGenericBuffer

class FRHIGenericBuffer : public FRHIBuffer
{
protected:

    explicit FRHIGenericBuffer(const FRHIGenericBufferInitializer& Initializer)
        : FRHIBuffer(Initializer)
        , Stride(Initializer.Stride)
        , Size(Initializer.Size)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIBuffer Interface

    virtual FRHIGenericBuffer* GetGenericBuffer() override final { return this; }

    virtual uint32 GetSize() const override final { return Size; }

    virtual uint32 GetStride() const override final { return Stride; }

private:
    uint32 Stride;
    uint32 Size;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIConstantBuffer

class FRHIConstantBuffer : public FRHIBuffer
{
protected:

    explicit FRHIConstantBuffer(const FRHIConstantBufferInitializer& Initializer)
        : FRHIBuffer(Initializer)
        , Size(Initializer.Size)
        , Stride(Initializer.Stride)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIBuffer Interface

    virtual FRHIConstantBuffer* GetConstantBuffer() override final { return this; }

    virtual uint32 GetSize() const override final { return Size; }

    virtual uint32 GetStride() const override final { return Stride; }

public:

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

private:
    uint32 Size;
    uint32 Stride;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
