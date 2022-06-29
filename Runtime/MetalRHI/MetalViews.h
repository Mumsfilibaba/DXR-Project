#pragma once
#include "MetalObject.h"

#include "RHI/RHIResourceViews.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalShaderResourceView

class CMetalShaderResourceView : public CMetalObject, public FRHIShaderResourceView
{
public:

    explicit CMetalShaderResourceView(CMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : CMetalObject(InDeviceContext)
        , FRHIShaderResourceView(InResource)
    { }

    ~CMetalShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalUnorderedAccessView

class CMetalUnorderedAccessView : public CMetalObject, public FRHIUnorderedAccessView
{
public:

    explicit CMetalUnorderedAccessView(CMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : CMetalObject(InDeviceContext)
        , FRHIUnorderedAccessView(InResource)
    { }

    ~CMetalUnorderedAccessView() = default;
};

#pragma clang diagnostic pop
