#pragma once
#include "RHI/RHIResourceViews.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalShaderResourceView

class CMetalShaderResourceView : public CRHIShaderResourceView
{
public:

    explicit CMetalShaderResourceView(CRHIResource* InResource)
        : CRHIShaderResourceView(InResource)
    { }

    ~CMetalShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalUnorderedAccessView

class CMetalUnorderedAccessView : public CRHIUnorderedAccessView
{
public:

    explicit CMetalUnorderedAccessView(CRHIResource* InResource)
        : CRHIUnorderedAccessView(InResource)
    { }

    ~CMetalUnorderedAccessView() = default;
};

#pragma clang diagnostic pop
