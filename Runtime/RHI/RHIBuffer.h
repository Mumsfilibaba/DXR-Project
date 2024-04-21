#pragma once
#include "RHIResource.h"
#include "Core/Containers/String.h"

enum class EBufferUsageFlags : uint16
{
    None = 0,

    Default  = FLAG(1), // Default Device Memory
    Dynamic  = FLAG(2), // Dynamic Memory (D3D12 UploadHeap)
    ReadBack = FLAG(3), // Read-Back from GPU

    ConstantBuffer  = FLAG(4), // Can be used as ConstantBuffer
    UnorderedAccess = FLAG(5), // Can be used in UnorderedAccessViews
    ShaderResource  = FLAG(6), // Can be used in ShaderResourceViews
    VertexBuffer    = FLAG(7), // Can be used as VertexBuffer
    IndexBuffer     = FLAG(8), // Can be used as IndexBuffer

    RWBuffer = UnorderedAccess | ShaderResource
};

ENUM_CLASS_OPERATORS(EBufferUsageFlags);

struct FRHIBufferInfo
{
    FRHIBufferInfo()
        : Size(0)
        , Stride(0)
        , UsageFlags(EBufferUsageFlags::None)
    {
    }

    FRHIBufferInfo(uint64 InSize, uint32 InStride, EBufferUsageFlags InUsageFlags)
        : Size(InSize)
        , Stride(InStride)
        , UsageFlags(InUsageFlags)
    {
    }

    bool IsDefault()  const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::Default); }
    bool IsDynamic()  const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::Dynamic); }
    bool IsReadBack() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ReadBack); }
    
    bool IsConstantBuffer()  const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ConstantBuffer); }
    bool IsShaderResource()  const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ShaderResource); }
    bool IsVertexBuffer()    const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::VertexBuffer); }
    bool IsIndexBuffer()     const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::IndexBuffer); }
    bool IsUnorderedAccess() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::UnorderedAccess); }

    bool operator==(const FRHIBufferInfo& Other) const
    {
        return Size == Other.Size && Stride == Other.Stride && UsageFlags == Other.UsageFlags;
    }

    bool operator!=(const FRHIBufferInfo& Other) const
    {
        return !(*this == Other);
    }

    uint64            Size;
    uint32            Stride;
    EBufferUsageFlags UsageFlags;
};

class FRHIBuffer : public FRHIResource
{
protected:
    explicit FRHIBuffer(const FRHIBufferInfo& InBufferInfo)
        : FRHIResource()
        , Info(InBufferInfo)
    {
    }

    virtual ~FRHIBuffer() = default;

public:
    virtual void* GetRHIBaseBuffer()         { return nullptr; }
    virtual void* GetRHIBaseResource() const { return nullptr; }

    // Returns a BindlessHandle if this buffer was created with a ConstantBuffer flag
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    virtual void SetDebugName(const FString& InName) { }
    virtual FString GetDebugName() const { return ""; }
    
    uint64 GetSize() const
    {
        return Info.Size;
    }
    
    uint32 GetStride() const
    {
        return Info.Stride;
    }

    EBufferUsageFlags GetFlags() const
    {
        return Info.UsageFlags;
    }

    const FRHIBufferInfo& GetInfo() const
    {
        return Info;
    }

protected:
    FRHIBufferInfo Info;
};