#pragma once
#include "Resource.h"
#include "RenderingCore.h"

enum class ESamplerMode : Byte
{
    SamplerMode_Unknown    = 0,
    SamplerMode_Wrap       = 1,
    SamplerMode_Mirror     = 2,
    SamplerMode_Clamp      = 3,
    SamplerMode_Border     = 4,
    SamplerMode_MirrorOnce = 5,
};

enum class ESamplerFilter : Byte
{
    SamplerFilter_Unknown                                 = 0,
    SamplerFilter_MinMagMipPoint                          = 1,
    SamplerFilter_MinMagPoint_MipLinear                   = 2,
    SamplerFilter_MinPoint_MagLinear_MipPoint             = 3,
    SamplerFilter_MinPoint_MagMipLinear                   = 4,
    SamplerFilter_MinLinear_MagMipPoint                   = 5,
    SamplerFilter_MinLinear_MagPoint_MipLinear            = 6,
    SamplerFilter_MinMagLinear_MipPoint                   = 7,
    SamplerFilter_MinMagMipLinear                         = 8,
    SamplerFilter_Anistrotopic                            = 9,
    SamplerFilter_Comparison_MinMagMipPoint               = 10,
    SamplerFilter_Comparison_MinMagPoint_MipLinear        = 11,
    SamplerFilter_Comparison_MinPoint_MagLinear_MipPoint  = 12,
    SamplerFilter_Comparison_MinPoint_MagMipLinear        = 13,
    SamplerFilter_Comparison_MinLinear_MagMipPoint        = 14,
    SamplerFilter_Comparison_MinLinear_MagPoint_MipLinear = 15,
    SamplerFilter_Comparison_MinMagLinear_MipPoint        = 16,
    SamplerFilter_Comparison_MinMagMipLinear              = 17,
    SamplerFilter_Comparison_Anistrotopic                 = 18,
};

struct SamplerStateCreateInfo
{
    ESamplerMode    AddressU       = ESamplerMode::SamplerMode_Clamp;
    ESamplerMode    AddressV       = ESamplerMode::SamplerMode_Clamp;
    ESamplerMode    AddressW       = ESamplerMode::SamplerMode_Clamp;
    ESamplerFilter  Filter         = ESamplerFilter::SamplerFilter_MinMagMipLinear;
    EComparisonFunc ComparisonFunc = EComparisonFunc::ComparisonFunc_Never;
    Float           MipLODBias     = 0.0f;
    UInt32          MaxAnisotropy  = 1;
    Float           BorderColor[4] = { 0.0f,0.0f, 0.0f, 0.0f };
    Float           MinLOD         = -FLT_MAX;
    Float           MaxLOD         = FLT_MAX;
};

class SamplerState : public PipelineResource
{
};