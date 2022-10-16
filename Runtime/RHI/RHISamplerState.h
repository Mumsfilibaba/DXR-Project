#pragma once
#include "RHIResourceBase.h"

#include "Core/Math/Float.h"
#include "Core/Templates/NumericLimits.h"

typedef TSharedRef<class FRHISamplerState> FRHISamplerStateRef;

enum class ESamplerMode : uint8
{
    Unknown    = 0,
    Wrap       = 1,
    Mirror     = 2,
    Clamp      = 3,
    Border     = 4,
    MirrorOnce = 5,
};

inline const CHAR* ToString(ESamplerMode SamplerMode)
{
    switch (SamplerMode)
    {
    case ESamplerMode::Wrap:       return "Wrap";
    case ESamplerMode::Mirror:     return "Mirror";
    case ESamplerMode::Clamp:      return "Clamp";
    case ESamplerMode::Border:     return "Border";
    case ESamplerMode::MirrorOnce: return "MirrorOnce";
    default:                       return "Unknown";
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
    Comparison_Anistrotopic                 = 18,
};

inline const CHAR* ToString(ESamplerFilter SamplerFilter)
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
    case ESamplerFilter::Comparison_Anistrotopic:                 return "Comparison_Anistrotopic";
    default:                                                      return "Unknown";
    }
}


struct FRHISamplerStateInitializer
{
    FRHISamplerStateInitializer()
        : AddressU(ESamplerMode::Clamp)
        , AddressV(ESamplerMode::Clamp)
        , AddressW(ESamplerMode::Clamp)
        , Filter(ESamplerFilter::MinMagMipLinear)
        , ComparisonFunc(EComparisonFunc::Never)
        , MaxAnisotropy(1)
        , MipLODBias(0.0f)
        , MinLOD(TNumericLimits<float>::Lowest())
        , MaxLOD(TNumericLimits<float>::Max())
        , BorderColor()
    { }

    FRHISamplerStateInitializer(ESamplerMode InAddressMode, ESamplerFilter InFilter)
        : AddressU(InAddressMode)
        , AddressV(InAddressMode)
        , AddressW(InAddressMode)
        , Filter(InFilter)
        , ComparisonFunc(EComparisonFunc::Unknown)
        , MaxAnisotropy(0)
        , MipLODBias(0.0f)
        , MinLOD(0.0f)
        , MaxLOD(TNumericLimits<float>::Max())
        , BorderColor(0.0f, 0.0f, 0.0f, 1.0f)
    { }

    FRHISamplerStateInitializer(
        ESamplerMode       InAddressU,
        ESamplerMode       InAddressV,
        ESamplerMode       InAddressW,
        ESamplerFilter     InFilter,
        EComparisonFunc    InComparisonFunc,
        float              InMipLODBias,
        uint8              InMaxAnisotropy,
        float              InMinLOD,
        float              InMaxLOD,
        const FFloatColor& InBorderColor)
        : AddressU(InAddressU)
        , AddressV(InAddressV)
        , AddressW(InAddressW)
        , Filter(InFilter)
        , ComparisonFunc(InComparisonFunc)
        , MaxAnisotropy(InMaxAnisotropy)
        , MipLODBias(InMipLODBias)
        , MinLOD(InMinLOD)
        , MaxLOD(InMaxLOD)
        , BorderColor(InBorderColor)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToUnderlying(AddressU);
        HashCombine(Hash, ToUnderlying(AddressV));
        HashCombine(Hash, ToUnderlying(AddressW));
        HashCombine(Hash, ToUnderlying(Filter));
        HashCombine(Hash, ToUnderlying(ComparisonFunc));
        HashCombine(Hash, MaxAnisotropy);
        HashCombine(Hash, MinLOD);
        HashCombine(Hash, MinLOD);
        HashCombine(Hash, MaxLOD);
        HashCombine(Hash, BorderColor.GetHash());
        return Hash;
    }

    bool operator==(const FRHISamplerStateInitializer& RHS) const
    {
        return (AddressU       == RHS.AddressU)
            && (AddressV       == RHS.AddressV)
            && (AddressW       == RHS.AddressW)
            && (Filter         == RHS.Filter)
            && (ComparisonFunc == RHS.ComparisonFunc)
            && (MipLODBias     == RHS.MipLODBias)
            && (MaxAnisotropy  == RHS.MaxAnisotropy)
            && (MinLOD         == RHS.MinLOD)
            && (MaxLOD         == RHS.MaxLOD)
            && (BorderColor    == RHS.BorderColor);
    }

    bool operator!=(const FRHISamplerStateInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    ESamplerMode    AddressU;
    ESamplerMode    AddressV;
    ESamplerMode    AddressW;
    
    ESamplerFilter  Filter;

    EComparisonFunc ComparisonFunc;

    uint8           MaxAnisotropy;

    float           MipLODBias;

    float           MinLOD;
    float           MaxLOD;

    FFloatColor     BorderColor;
};


class FRHISamplerState 
    : public FRHIResource
{
protected:
    explicit FRHISamplerState(const FRHISamplerStateInitializer& InInitializer)
        : Initializer(InInitializer)
    { }

public:

    /** @return - Returns the Bindless descriptor-handle if the RHI-supports descriptor-handles */
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    const FRHISamplerStateInitializer& GetInitializer() const { return Initializer; } 

private:
    FRHISamplerStateInitializer Initializer;
};
