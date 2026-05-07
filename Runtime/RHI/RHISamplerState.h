#pragma once
#include "RHI/RHIResource.h"

enum class EShaderStage : uint8;

enum class ESamplerMode : uint8
{
    Unknown    = 0,
    Wrap       = 1,
    Mirror     = 2,
    Clamp      = 3,
    Border     = 4,
    MirrorOnce = 5,
};

NODISCARD constexpr const CHAR* ToString(ESamplerMode SamplerMode)
{
    switch (SamplerMode)
    {
        case ESamplerMode::Wrap:       return "Wrap";
        case ESamplerMode::Mirror:     return "Mirror";
        case ESamplerMode::Clamp:      return "Clamp";
        case ESamplerMode::Border:     return "Border";
        case ESamplerMode::MirrorOnce: return "MirrorOnce";
        
        default: return "Unknown";
    }
}

enum class ESamplerFilter : uint8
{
    Unknown                                 = 0,
    MinMagMipPoint                          = 1,
    MinMagPoint_MipLinear                   = 2,
    MinPoint_MagLinear_MipPoint             = 3,
    MinPoint_MagMipLinear                   = 4,
    MinLinear_MagMipPoint                   = 5,
    MinLinear_MagPoint_MipLinear            = 6,
    MinMagLinear_MipPoint                   = 7,
    MinMagMipLinear                         = 8,
    Anistrotopic                            = 9,
    Comparison_MinMagMipPoint               = 10,
    Comparison_MinMagPoint_MipLinear        = 11,
    Comparison_MinPoint_MagLinear_MipPoint  = 12,
    Comparison_MinPoint_MagMipLinear        = 13,
    Comparison_MinLinear_MagMipPoint        = 14,
    Comparison_MinLinear_MagPoint_MipLinear = 15,
    Comparison_MinMagLinear_MipPoint        = 16,
    Comparison_MinMagMipLinear              = 17,
    Comparison_Anisotropic                  = 18,
};

NODISCARD constexpr const CHAR* ToString(ESamplerFilter SamplerFilter)
{
    switch (SamplerFilter)
    {
        case ESamplerFilter::MinMagMipPoint:                          return "MinMagMipPoint";
        case ESamplerFilter::MinMagPoint_MipLinear:                   return "MinMagPoint_MipLinear";
        case ESamplerFilter::MinPoint_MagLinear_MipPoint:             return "MinPoint_MagLinear_MipPoint";
        case ESamplerFilter::MinPoint_MagMipLinear:                   return "MinPoint_MagMipLinear";
        case ESamplerFilter::MinLinear_MagMipPoint:                   return "MinLinear_MagMipPoint";
        case ESamplerFilter::MinLinear_MagPoint_MipLinear:            return "MinLinear_MagPoint_MipLinear";
        case ESamplerFilter::MinMagLinear_MipPoint:                   return "MinMagLinear_MipPoint";
        case ESamplerFilter::MinMagMipLinear:                         return "MinMagMipLinear";
        case ESamplerFilter::Anistrotopic:                            return "Anistrotopic";
        case ESamplerFilter::Comparison_MinMagMipPoint:               return "Comparison_MinMagMipPoint";
        case ESamplerFilter::Comparison_MinMagPoint_MipLinear:        return "Comparison_MinMagPoint_MipLinear";
        case ESamplerFilter::Comparison_MinPoint_MagLinear_MipPoint:  return "Comparison_MinPoint_MagLinear_MipPoint";
        case ESamplerFilter::Comparison_MinPoint_MagMipLinear:        return "Comparison_MinPoint_MagMipLinear";
        case ESamplerFilter::Comparison_MinLinear_MagMipPoint:        return "Comparison_MinLinear_MagMipPoint";
        case ESamplerFilter::Comparison_MinLinear_MagPoint_MipLinear: return "Comparison_MinLinear_MagPoint_MipLinear";
        case ESamplerFilter::Comparison_MinMagLinear_MipPoint:        return "Comparison_MinMagLinear_MipPoint";
        case ESamplerFilter::Comparison_MinMagMipLinear:              return "Comparison_MinMagMipLinear";
        case ESamplerFilter::Comparison_Anisotropic:                  return "Comparison_Anisotropic";
        
        default: return "Unknown";
    }
}

struct FRHISamplerStateDesc
{
    NODISCARD static FRHISamplerStateDesc Create(ESamplerMode InSamplerMode, ESamplerFilter InFilter)
    {
        FRHISamplerStateDesc SamplerDesc;
        SamplerDesc.AddressU       = InSamplerMode;
        SamplerDesc.AddressV       = InSamplerMode;
        SamplerDesc.AddressW       = InSamplerMode;
        SamplerDesc.Filter         = InFilter;
        SamplerDesc.ComparisonFunc = EComparisonFunc::Unknown;
        SamplerDesc.MaxAnisotropy  = 1;
        SamplerDesc.MipLODBias     = 0.0f;
        SamplerDesc.MinLOD         = TNumericLimits<float>::Lowest();
        SamplerDesc.MaxLOD         = TNumericLimits<float>::Max();
        SamplerDesc.BorderColor    = { };
        return SamplerDesc;
    }

    NODISCARD constexpr bool IsComparisonSampler() const noexcept
    {
        return Filter >= ESamplerFilter::Comparison_MinMagMipPoint && Filter <= ESamplerFilter::Comparison_Anisotropic;
    }

    bool operator==(const FRHISamplerStateDesc& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHISamplerStateDesc& Value)
    {
        uint64 Hash = UnderlyingTypeValue(Value.AddressU);
        HashCombine(Hash, UnderlyingTypeValue(Value.AddressV));
        HashCombine(Hash, UnderlyingTypeValue(Value.AddressW));
        HashCombine(Hash, UnderlyingTypeValue(Value.Filter));
        HashCombine(Hash, UnderlyingTypeValue(Value.ComparisonFunc));
        HashCombine(Hash, Value.MaxAnisotropy);
        HashCombine(Hash, Value.MinLOD);
        HashCombine(Hash, Value.MinLOD);
        HashCombine(Hash, Value.MaxLOD);
        HashCombine(Hash, GetHashForType(Value.BorderColor));
        return Hash;
    }

    ESamplerMode    AddressU       = ESamplerMode::Clamp;
    ESamplerMode    AddressV       = ESamplerMode::Clamp;
    ESamplerMode    AddressW       = ESamplerMode::Clamp;
    ESamplerFilter  Filter         = ESamplerFilter::MinMagMipLinear;
    EComparisonFunc ComparisonFunc = EComparisonFunc::Unknown;
    uint8           MaxAnisotropy  = 1;
    float           MipLODBias     = 0.0f;
    float           MinLOD         = TNumericLimits<float>::Lowest();
    float           MaxLOD         = TNumericLimits<float>::Max();
    FFloatColor     BorderColor    = { };
};

struct FRHIStaticSamplerInfo
{
    FRHISamplerStateDesc GetSamplerStateDesc() const
    {
        FRHISamplerStateDesc SamplerDesc;
        SamplerDesc.AddressU       = AddressU;
        SamplerDesc.AddressV       = AddressV;
        SamplerDesc.AddressW       = AddressW;
        SamplerDesc.Filter         = Filter;
        SamplerDesc.ComparisonFunc = ComparisonFunc;
        SamplerDesc.MaxAnisotropy  = MaxAnisotropy;
        SamplerDesc.MipLODBias     = MipLODBias;
        SamplerDesc.MinLOD         = MinLOD;
        SamplerDesc.MaxLOD         = MaxLOD;
        SamplerDesc.BorderColor    = BorderColor;
        return SamplerDesc;
    }

    NODISCARD constexpr bool IsComparisonSampler() const noexcept
    {
        return Filter >= ESamplerFilter::Comparison_MinMagMipPoint && Filter <= ESamplerFilter::Comparison_Anisotropic;
    }

    bool operator==(const FRHIStaticSamplerInfo& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHIStaticSamplerInfo& Value)
    {
        uint64 Hash = UnderlyingTypeValue(Value.AddressU);
        HashCombine(Hash, UnderlyingTypeValue(Value.AddressV));
        HashCombine(Hash, UnderlyingTypeValue(Value.AddressW));
        HashCombine(Hash, UnderlyingTypeValue(Value.Filter));
        HashCombine(Hash, UnderlyingTypeValue(Value.ComparisonFunc));
        HashCombine(Hash, Value.MaxAnisotropy);
        HashCombine(Hash, Value.MipLODBias);
        HashCombine(Hash, Value.MinLOD);
        HashCombine(Hash, Value.MaxLOD);
        HashCombine(Hash, GetHashForType(Value.BorderColor));
        HashCombine(Hash, UnderlyingTypeValue(Value.ShaderVisibility));
        HashCombine(Hash, Value.ShaderRegister);
        return Hash;
    }

    ESamplerMode    AddressU         = ESamplerMode::Clamp;
    ESamplerMode    AddressV         = ESamplerMode::Clamp;
    ESamplerMode    AddressW         = ESamplerMode::Clamp;
    ESamplerFilter  Filter           = ESamplerFilter::MinMagMipLinear;
    EComparisonFunc ComparisonFunc   = EComparisonFunc::Unknown;
    uint8           MaxAnisotropy    = 1;
    float           MipLODBias       = 0.0f;
    float           MinLOD           = TNumericLimits<float>::Lowest();
    float           MaxLOD           = TNumericLimits<float>::Max();
    FFloatColor     BorderColor      = { };
    EShaderStage    ShaderVisibility = EShaderStage{};
    uint16          ShaderRegister   = 0;
};

class FRHISamplerState : public FRHIResource
{
protected:
    explicit FRHISamplerState(const FRHISamplerStateDesc& InSamplerDesc)
        : FRHIResource(ERHIResourceType::SamplerState)
        , Desc(InSamplerDesc)
    {
    }

    virtual ~FRHISamplerState() = default;

public:

    // D3D12: D3D12_CPU_DESCRIPTOR_HANDLE::ptr. Vulkan: VkSampler. Metal: id<MTLSamplerState>. Null: nullptr.
    virtual void* GetRHINativeSampler() const = 0;

    virtual FRHIDescriptorHandle GetBindlessHandle() const = 0;

    const FRHISamplerStateDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHISamplerStateDesc Desc;
};
