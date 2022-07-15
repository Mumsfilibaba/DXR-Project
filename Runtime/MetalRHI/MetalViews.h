#pragma once
#include "MetalObject.h"

#include "RHI/RHIResourceViews.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalShaderResourceView

class FMetalShaderResourceView : public FMetalObject, public FRHIShaderResourceView
{
public:

    explicit FMetalShaderResourceView(FMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : FMetalObject(InDeviceContext)
        , FRHIShaderResourceView(InResource)
    { }

    ~FMetalShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalUnorderedAccessView

class FMetalUnorderedAccessView : public FMetalObject, public FRHIUnorderedAccessView
{
public:

    explicit FMetalUnorderedAccessView(FMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : FMetalObject(InDeviceContext)
        , FRHIUnorderedAccessView(InResource)
    { }

    ~FMetalUnorderedAccessView() = default;
};

#pragma clang diagnostic pop
