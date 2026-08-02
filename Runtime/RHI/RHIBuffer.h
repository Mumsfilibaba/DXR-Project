#pragma once 
#include "Core/Containers/String.h" 
#include "RHI/RHIResource.h" 
 
DISABLE_UNREFERENCED_VARIABLE_WARNING

enum class EBufferFlags : uint16 
{ 
    None = 0,

    Default               = FLAG(1),  // Default device memory (memory class, not a resource-state policy)
    Dynamic               = FLAG(2),  // Dynamic memory (D3D12 upload heap)
    ReadBack              = FLAG(3),  // Read-back memory; CopyDest usage is implicit
    Transient             = FLAG(4),  // Per-frame ephemeral upload memory, contents not preserved across frames
    ConstantBuffer        = FLAG(5),  // Can be used as ConstantBuffer
    UnorderedAccessBuffer = FLAG(6),  // Can be used in UnorderedAccessViews
    ShaderResourceBuffer  = FLAG(7),  // Can be used in ShaderResourceViews
    VertexBuffer          = FLAG(8),  // Can be used as VertexBuffer
    IndexBuffer           = FLAG(9),  // Can be used as IndexBuffer
    StreamOutputBuffer    = FLAG(10), // Can be used as a StreamOutput target
    CopySource            = FLAG(11), // May be used explicitly as a runtime copy source
    CopyDest              = FLAG(12), // May be used explicitly as a runtime copy destination
    AccelerationStructure = FLAG(13), // Participates in AS copy/serialize/build ops; requires AS (256B) allocation alignment
    IndirectArguments     = FLAG(14), // May be consumed by indirect draw/dispatch commands

    RWBuffer = UnorderedAccessBuffer | ShaderResourceBuffer
};

ENUM_CLASS_OPERATORS(EBufferFlags);

struct FRHIBufferDesc
{
    NODISCARD constexpr bool IsDefault()               const { return IsEnumFlagSet(Flags, EBufferFlags::Default); }
    NODISCARD constexpr bool IsDynamic()               const { return IsEnumFlagSet(Flags, EBufferFlags::Dynamic); }
    NODISCARD constexpr bool IsReadBack()              const { return IsEnumFlagSet(Flags, EBufferFlags::ReadBack); }
    NODISCARD constexpr bool IsTransient()             const { return IsEnumFlagSet(Flags, EBufferFlags::Transient); }
    NODISCARD constexpr bool IsConstantBuffer()        const { return IsEnumFlagSet(Flags, EBufferFlags::ConstantBuffer); }
    NODISCARD constexpr bool IsShaderResourceBuffer()  const { return IsEnumFlagSet(Flags, EBufferFlags::ShaderResourceBuffer); }
    NODISCARD constexpr bool IsVertexBuffer()          const { return IsEnumFlagSet(Flags, EBufferFlags::VertexBuffer); }
    NODISCARD constexpr bool IsIndexBuffer()           const { return IsEnumFlagSet(Flags, EBufferFlags::IndexBuffer); }
    NODISCARD constexpr bool IsUnorderedAccessBuffer() const { return IsEnumFlagSet(Flags, EBufferFlags::UnorderedAccessBuffer); }
    NODISCARD constexpr bool IsStreamOutputBuffer()    const { return IsEnumFlagSet(Flags, EBufferFlags::StreamOutputBuffer); }
    NODISCARD constexpr bool IsCopySource()            const { return IsEnumFlagSet(Flags, EBufferFlags::CopySource); }
    NODISCARD constexpr bool IsCopyDest()              const { return IsEnumFlagSet(Flags, EBufferFlags::CopyDest); }
    NODISCARD constexpr bool IsAccelerationStructure() const { return IsEnumFlagSet(Flags, EBufferFlags::AccelerationStructure); }
    NODISCARD constexpr bool IsIndirectArguments()     const { return IsEnumFlagSet(Flags, EBufferFlags::IndirectArguments); }

    EBufferFlags                  Flags        = EBufferFlags::None;
    uint32                        Stride       = 0;
    uint64                        Size         = 0;
    ERHIResourceStateTrackingMode TrackingMode = ERHIResourceStateTrackingMode::Tracked;
};

class FRHIBuffer : public FRHIResource 
{ 
protected: 
    explicit FRHIBuffer(const FRHIBufferDesc& InBufferDesc) 
        : FRHIResource(ERHIResourceType::Buffer)
        , Desc(InBufferDesc)
    {
    }

public: 
    
    /** @return D3D12: ID3D12Resource*. Vulkan: VkBuffer. Metal: id<MTLBuffer>. Null: nullptr. */
    virtual void* GetRHINativeResource() const = 0; 
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const = 0; 
 
    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) = 0;
    virtual void  Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) = 0;
 
    virtual void SetDebugName(const String& InName) = 0; 
    virtual void GetDebugName(String& OutDebugName) const = 0; 
 
    NODISCARD const FRHIBufferDesc& GetDesc() const 
    { 
        return Desc; 
    } 

protected: 
    FRHIBufferDesc Desc; 
}; 

ENABLE_UNREFERENCED_VARIABLE_WARNING
