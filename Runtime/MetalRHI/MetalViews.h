#pragma once
#include "MetalObject.h"

#include "RHI/RHIResourceViews.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalShaderResourceView

class CMetalShaderResourceView : public CMetalObject, public CRHIShaderResourceView
{
public:

    explicit CMetalShaderResourceView(CMetalDeviceContext* InDeviceContext, CRHIResource* InResource)
        : CMetalObject(InDeviceContext)
        , CRHIShaderResourceView(InResource)
    { }

    ~CMetalShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalUnorderedAccessView

class CMetalUnorderedAccessView : public CMetalObject, public CRHIUnorderedAccessView
{
public:

    explicit CMetalUnorderedAccessView(CMetalDeviceContext* InDeviceContext, CRHIResource* InResource)
        : CMetalObject(InDeviceContext)
        , CRHIUnorderedAccessView(InResource)
    { }

    ~CMetalUnorderedAccessView() = default;
};

#pragma clang diagnostic pop
