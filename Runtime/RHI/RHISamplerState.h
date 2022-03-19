#pragma once
#include "RHIResourceBase.h"

#include "Core/Math/Float.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class CRHISamplerState> CRHISamplerStateRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHISamplerMode

enum class ERHISamplerMode : uint8
{
    Unknown    = 0,
    Wrap       = 1,
    Mirror     = 2,
    Clamp      = 3,
    Border     = 4,
    MirrorOnce = 5,
};

inline const char* ToString(ERHISamplerMode SamplerMode)
{
    switch (SamplerMode)
    {
    case ERHISamplerMode::Wrap:       return "Wrap";
    case ERHISamplerMode::Mirror:     return "Mirror";
    case ERHISamplerMode::Clamp:      return "Clamp";
    case ERHISamplerMode::Border:     return "Border";
    case ERHISamplerMode::MirrorOnce: return "MirrorOnce";
    default:                          return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHISamplerFilter

enum class ERHISamplerFilter : uint8
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

inline const char* ToString(ERHISamplerFilter SamplerFilter)
{
    switch (SamplerFilter)
    {
    case ERHISamplerFilter::MinMagMipPoint:                          return "MinMagMipPoint";
    case ERHISamplerFilter::MinMagPoint_MipLinear:                   return "MinMagPoint_MipLinear";
    case ERHISamplerFilter::MinPoint_MagLinear_MipPoint:             return "MinPoint_MagLinear_MipPoint";
    case ERHISamplerFilter::MinPoint_MagMipLinear:                   return "MinPoint_MagMipLinear";
    case ERHISamplerFilter::MinLinear_MagMipPoint:                   return "MinLinear_MagMipPoint";
    case ERHISamplerFilter::MinLinear_MagPoint_MipLinear:            return "MinLinear_MagPoint_MipLinear";
    case ERHISamplerFilter::MinMagLinear_MipPoint:                   return "MinMagLinear_MipPoint";
    case ERHISamplerFilter::MinMagMipLinear:                         return "MinMagMipLinear";
    case ERHISamplerFilter::Anistrotopic:                            return "Anistrotopic";
    case ERHISamplerFilter::Comparison_MinMagMipPoint:               return "Comparison_MinMagMipPoint";
    case ERHISamplerFilter::Comparison_MinMagPoint_MipLinear:        return "Comparison_MinMagPoint_MipLinear";
    case ERHISamplerFilter::Comparison_MinPoint_MagLinear_MipPoint:  return "Comparison_MinPoint_MagLinear_MipPoint";
    case ERHISamplerFilter::Comparison_MinPoint_MagMipLinear:        return "Comparison_MinPoint_MagMipLinear";
    case ERHISamplerFilter::Comparison_MinLinear_MagMipPoint:        return "Comparison_MinLinear_MagMipPoint";
    case ERHISamplerFilter::Comparison_MinLinear_MagPoint_MipLinear: return "Comparison_MinLinear_MagPoint_MipLinear";
    case ERHISamplerFilter::Comparison_MinMagLinear_MipPoint:        return "Comparison_MinMagLinear_MipPoint";
    case ERHISamplerFilter::Comparison_MinMagMipLinear:              return "Comparison_MinMagMipLinear";
    case ERHISamplerFilter::Comparison_Anistrotopic:                 return "Comparison_Anistrotopic";
    default:                                                         return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHISamplerStateDesc

class CRHISamplerStateDesc
{
public:
    CRHISamplerStateDesc()  = default;
    ~CRHISamplerStateDesc() = default;

    bool operator==(const CRHISamplerStateDesc& RHS) const
    {
        return
            (AddressU       == RHS.AddressU)       &&
            (AddressV       == RHS.AddressV)       && 
            (AddressW       == RHS.AddressW)       &&
            (Filter         == RHS.Filter)         && 
            (ComparisonFunc == RHS.ComparisonFunc) && 
            (MipLODBias     == RHS.MipLODBias)     && 
            (MaxAnisotropy  == RHS.MaxAnisotropy)  && 
            (MinLOD         == RHS.MinLOD)         && 
            (MaxLOD         == RHS.MaxLOD)         &&
            (BorderColor    == RHS.BorderColor);
    }

    bool operator!=(const CRHISamplerStateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    ERHISamplerMode    AddressU       = ERHISamplerMode::Clamp;
    ERHISamplerMode    AddressV       = ERHISamplerMode::Clamp;
    ERHISamplerMode    AddressW       = ERHISamplerMode::Clamp;
    ERHISamplerFilter  Filter         = ERHISamplerFilter::MinMagMipLinear;
    ERHIComparisonFunc ComparisonFunc = ERHIComparisonFunc::Never;
    float              MipLODBias     = 0.0f;
    uint32             MaxAnisotropy  = 1;
    float              MinLOD         = -FLT_MAX;
    float              MaxLOD         = FLT_MAX;
    SColorF            BorderColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHISamplerState

class CRHISamplerState : public CRHIObject
{
};