#pragma once
#include "RHIResourceBase.h"

#include "Core/Templates/EnumUtilities.h"
#include "Core/Containers/SharedRef.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

typedef TSharedRef<class FRHIBuffer> FRHIBufferRef;

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


struct FRHIBufferDesc
{
    FRHIBufferDesc()
        : Size(0)
        , Stride(0)
        , UsageFlags(EBufferUsageFlags::None)
    { }

    FRHIBufferDesc(
        uint64 InSize,
        uint32 InStride,
        EBufferUsageFlags InUsageFlags)
        : Size(InSize)
        , Stride(InStride)
        , UsageFlags(InUsageFlags)
    { }

    bool IsDefault()  const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::Default); }
    bool IsDynamic()  const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::Dynamic); }
    bool IsReadBack() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ReadBack); }
    
    bool IsConstantBuffer() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ConstantBuffer); }
    bool IsShaderResource() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::ShaderResource); }
    bool IsVertexBuffer()   const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::VertexBuffer); }
    bool IsIndexBuffer()    const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::IndexBuffer); }
    
    bool IsUnorderedAccess() const { return IsEnumFlagSet(UsageFlags, EBufferUsageFlags::UnorderedAccess); }

    bool operator==(const FRHIBufferDesc& Other) const
    {
        return (Size == Other.Size) && (Stride == Other.Stride) && (UsageFlags == Other.UsageFlags);
    }

    bool operator!=(const FRHIBufferDesc& Other) const
    {
        return !(*this == Other);
    }

    uint64            Size;
    uint32            Stride;
    EBufferUsageFlags UsageFlags;
};


class FRHIBuffer 
    : public FRHIResource
{
protected:
    explicit FRHIBuffer(const FRHIBufferDesc& InDesc)
        : FRHIResource()
        , Desc(InDesc)
    { }

public:

    /** @return - Returns a pointer to the RHI implementation of RHIBuffer */
    virtual void* GetRHIBaseBuffer() { return nullptr; }

    /** @return - Returns the native resource-handle */
    virtual void* GetRHIBaseResource() const { return nullptr; }

    /** @return - Returns a ConstantBuffer Bindless-handle */
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    /** @return - Returns the name of the resource */
    virtual FString GetName() const { return ""; }

    /**
     * @brief        - Set the name of the resource
     * @param InName - New name of the resource
     */
    virtual void SetName(const FString& InName) { }
    
    /** @return - Returns the size of the buffer */
    FORCEINLINE uint64 GetSize() const { return Desc.Size; }
    
    /** @return - Returns the stride of each element in the buffer */
    FORCEINLINE uint32 GetStride() const { return Desc.Stride; }

    /** @return - Returns the flags of the buffer */
    FORCEINLINE EBufferUsageFlags GetFlags() const { return Desc.UsageFlags; }

    /** @return - Returns the description used to create the buffer */
    FORCEINLINE const FRHIBufferDesc& GetDesc() const { return Desc; }

protected:
    FRHIBufferDesc Desc;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
