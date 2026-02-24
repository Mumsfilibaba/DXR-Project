#pragma once 
#include "Core/Containers/String.h" 
#include "RHI/RHIResource.h" 
 
DISABLE_UNREFERENCED_VARIABLE_WARNING

enum class EBufferFlags : uint16 
{ 
    None = 0,

    Default   = FLAG(1), // Default Device Memory
    Dynamic   = FLAG(2), // Dynamic Memory (D3D12 UploadHeap)
    ReadBack  = FLAG(3), // Read-Back from GPU
    Transient = FLAG(4), // Per-frame ephemeral memory, contents not preserved across frames

    ConstantBuffer        = FLAG(5), // Can be used as ConstantBuffer
    UnorderedAccessBuffer = FLAG(6), // Can be used in UnorderedAccessViews
    ShaderResourceBuffer  = FLAG(7), // Can be used in ShaderResourceViews
    VertexBuffer          = FLAG(8), // Can be used as VertexBuffer
    IndexBuffer           = FLAG(9), // Can be used as IndexBuffer

    RWBuffer = UnorderedAccessBuffer | ShaderResourceBuffer
};

ENUM_CLASS_OPERATORS(EBufferFlags);

struct FRHIBufferInfo
{
    NODISCARD constexpr bool IsDefault() const { return IsEnumFlagSet(Flags, EBufferFlags::Default); }
    NODISCARD constexpr bool IsDynamic() const { return IsEnumFlagSet(Flags, EBufferFlags::Dynamic); }
    NODISCARD constexpr bool IsReadBack() const { return IsEnumFlagSet(Flags, EBufferFlags::ReadBack); }
    NODISCARD constexpr bool IsTransient() const { return IsEnumFlagSet(Flags, EBufferFlags::Transient); }
    
    NODISCARD constexpr bool IsConstantBuffer() const { return IsEnumFlagSet(Flags, EBufferFlags::ConstantBuffer); }
    NODISCARD constexpr bool IsShaderResourceBuffer() const { return IsEnumFlagSet(Flags, EBufferFlags::ShaderResourceBuffer); }
    NODISCARD constexpr bool IsVertexBuffer() const { return IsEnumFlagSet(Flags, EBufferFlags::VertexBuffer); }
    NODISCARD constexpr bool IsIndexBuffer() const { return IsEnumFlagSet(Flags, EBufferFlags::IndexBuffer); }
    NODISCARD constexpr bool IsUnorderedAccessBuffer() const { return IsEnumFlagSet(Flags, EBufferFlags::UnorderedAccessBuffer); }

    EBufferFlags Flags = EBufferFlags::None;
    uint32 Stride = 0;
    uint64 Size   = 0;
};

class FRHIBuffer : public FRHIResource 
{ 
protected: 
    explicit FRHIBuffer(const FRHIBufferInfo& InBufferInfo) 
        : FRHIResource()
        , Info(InBufferInfo)
    {
    }

public: 
    virtual void* GetRHINativeHandle() const = 0; 
    virtual FRHIDescriptorHandle GetBindlessHandle() const = 0; 
 
    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) { return nullptr; } 
    virtual void Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) { } 
 
    virtual void SetDebugName(const FString& InName) = 0; 
    virtual FString GetDebugName() const = 0; 
 
    const FRHIBufferInfo& GetInfo() const 
    { 
        return Info; 
    } 

protected: 
    FRHIBufferInfo Info; 
}; 

ENABLE_UNREFERENCED_VARIABLE_WARNING
