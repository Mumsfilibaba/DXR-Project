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

struct FRHISamplerStateInfo
{
    NODISCARD static FRHISamplerStateInfo Create(ESamplerMode InSamplerMode, ESamplerFilter InFilter)
    {
        FRHISamplerStateInfo SamplerInfo;
        SamplerInfo.AddressU       = InSamplerMode;
        SamplerInfo.AddressV       = InSamplerMode;
        SamplerInfo.AddressW       = InSamplerMode;
        SamplerInfo.Filter         = InFilter;
        SamplerInfo.ComparisonFunc = EComparisonFunc::Unknown;
        SamplerInfo.MaxAnisotropy  = 1;
        SamplerInfo.MipLODBias     = 0.0f;
        SamplerInfo.MinLOD         = TNumericLimits<float>::Lowest();
        SamplerInfo.MaxLOD         = TNumericLimits<float>::Max();
        SamplerInfo.BorderColor    = { };
        return SamplerInfo;
    }

    NODISCARD constexpr bool IsComparisonSampler() const noexcept
    {
        return Filter >= ESamplerFilter::Comparison_MinMagMipPoint && Filter <= ESamplerFilter::Comparison_Anisotropic;
    }

    bool operator==(const FRHISamplerStateInfo& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHISamplerStateInfo& Value)
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
    FRHISamplerStateInfo GetSamplerStateInfo() const
    {
        FRHISamplerStateInfo Info;
        Info.AddressU       = AddressU;
        Info.AddressV       = AddressV;
        Info.AddressW       = AddressW;
        Info.Filter         = Filter;
        Info.ComparisonFunc = ComparisonFunc;
        Info.MaxAnisotropy  = MaxAnisotropy;
        Info.MipLODBias     = MipLODBias;
        Info.MinLOD         = MinLOD;
        Info.MaxLOD         = MaxLOD;
        Info.BorderColor    = BorderColor;
        return Info;
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
    explicit FRHISamplerState(const FRHISamplerStateInfo& InSamplerInfo)
        : Info(InSamplerInfo)
    {
    }

    virtual ~FRHISamplerState() = default;

public:
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    const FRHISamplerStateInfo& GetInfo() const
    {
        return Info;
    }

protected:
    FRHISamplerStateInfo Info;
};
