#ifndef DESCRIPTOR_TYPES_HLSLI
#define DESCRIPTOR_TYPES_HLSLI

// Compile-time mirror of C++ EDescriptorType in Runtime/RHI/RHITypes.h. Keep in sync.
enum class EDescriptorType : uint
{
    Unknown         = 0,
    UnorderedAccess = 1,
    ShaderResource  = 2,
    ConstantBuffer  = 3,
    Sampler         = 4,
};

static const uint DESCRIPTOR_INDEX_MASK    = 0x00FFFFFFu;
static const uint DESCRIPTOR_TYPE_SHIFT    = 24u;
static const uint DESCRIPTOR_INVALID_INDEX = DESCRIPTOR_INDEX_MASK;

struct FDescriptorHandle
{
    static FDescriptorHandle FromPacked(uint InPacked)
    {
        FDescriptorHandle Result;
        Result.Packed = InPacked;
        return Result;
    }

    bool IsValid()
    {
        return GetDescriptorType() != EDescriptorType::Unknown && GetIndex() != DESCRIPTOR_INVALID_INDEX;
    }

    uint GetIndex()
    {
        return (Packed & DESCRIPTOR_INDEX_MASK);
    }

    EDescriptorType GetDescriptorType()
    {
        return (EDescriptorType)(Packed >> DESCRIPTOR_TYPE_SHIFT);
    }

    uint Packed;
};

#endif
