#pragma once
#include "RHIResourceBase.h"

#include "Core/Math/Float.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class CRHISamplerState> CRHISamplerStateRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ESamplerMode

enum class ESamplerMode : uint8
{
    Unknown    = 0,
    Wrap       = 1,
    Mirror     = 2,
    Clamp      = 3,
    Border     = 4,
    MirrorOnce = 5,
};

inline const char* ToString(ESamplerMode SamplerMode)
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ESamplerFilter

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

inline const char* ToString(ESamplerFilter SamplerFilter)
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHISamplerStateCreateDesc

struct SRHISamplerStateCreateDesc
{
    SRHISamplerStateCreateDesc()
        : AddressU(ESamplerMode::Clamp)
        , AddressV(ESamplerMode::Clamp)
        , AddressW(ESamplerMode::Clamp)
        , Filter(ESamplerFilter::MinMagMipLinear)
        , ComparisonFunc(EComparisonFunc::Never)
        , MipLODBias(0.0f)
        , MaxAnisotropy(1)
        , MinLOD(-FLT_MAX)
        , MaxLOD(FLT_MAX)
        , BorderColor()
    { }

    SRHISamplerStateCreateDesc( ESamplerMode InAddressU
                              , ESamplerMode InAddressV
                              , ESamplerMode InAddressW
                              , ESamplerFilter InFilter
                              , EComparisonFunc InComparisonFunc
                              , float InMipLODBias
                              , uint32 InMaxAnisotropy
                              , float InMinLOD
                              , float InMaxLOD
                              , SColorF InBorderColor)
        : AddressU(InAddressU)
        , AddressV(InAddressV)
        , AddressW(InAddressW)
        , Filter(InFilter)
        , ComparisonFunc(InComparisonFunc)
        , MipLODBias(InMipLODBias)
        , MaxAnisotropy(InMaxAnisotropy)
        , MinLOD(InMinLOD)
        , MaxLOD(InMaxLOD)
        , BorderColor()
    { }

    bool operator==(const SRHISamplerStateCreateDesc& Rhs) const
    {
        return (AddressU       == Rhs.AddressU)
            && (AddressV       == Rhs.AddressV)
            && (AddressW       == Rhs.AddressW) 
            && (Filter         == Rhs.Filter)
            && (ComparisonFunc == Rhs.ComparisonFunc) 
            && (MipLODBias     == Rhs.MipLODBias) 
            && (MaxAnisotropy  == Rhs.MaxAnisotropy) 
            && (MinLOD         == Rhs.MinLOD) 
            && (MaxLOD         == Rhs.MaxLOD) 
            && (BorderColor    == Rhs.BorderColor);
    }

    bool operator!=(const SRHISamplerStateCreateDesc& Rhs) const
    {
        return !(*this == Rhs);
    }

    ESamplerMode    AddressU;
    ESamplerMode    AddressV;
    ESamplerMode    AddressW;
    ESamplerFilter  Filter;
    EComparisonFunc ComparisonFunc;
    float           MipLODBias;
    uint32          MaxAnisotropy;
    float           MinLOD;
    float           MaxLOD;
    SColorF         BorderColor;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHISamplerState

class CRHISamplerState : public CRHIResource
{
public:

    /**
     * @brief: Constructor
     * 
     * @param InDesc: Description for the SamplerState
     */
    CRHISamplerState(const SRHISamplerStateCreateDesc& InDesc)
        : CRHIResource(ERHIResourceType::SamplerState)
        , Desc(InDesc)
    { }

    /**
     * @brief: Retrieve the bindless descriptor-handle if the RHI-supports descriptor-handles
     * 
     * @return: Returns the bindless descriptor-handle if the RHI-supports descriptor-handles
     */
    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }

    /**
     * @brief: Retrieve the SamplerState description
     * 
     * @return: Returns the SamplerState description
     */
    const SRHISamplerStateCreateDesc& GetDesc() const { return Desc; }

private:
    SRHISamplerStateCreateDesc Desc;
};