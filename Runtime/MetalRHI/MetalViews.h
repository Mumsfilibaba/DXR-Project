#pragma once
#include "MetalObject.h"

#include "RHI/RHIResourceViews.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalView

class CMetalView : public CMetalObject
{
public:
    explicit CMetalView(CMetalDeviceContext* InDeviceContext)
        : CMetalObject(InDeviceContext)
    { }

    id<MTLTexture> GetMTLTexture() const { return TextureView; }
    
private:
    id<MTLTexture> TextureView;
    id<MTLBuffer>  Buffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalShaderResourceView

class FMetalShaderResourceView : public FRHIShaderResourceView, public CMetalView
{
public:

    explicit FMetalShaderResourceView(FMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : FRHIShaderResourceView(InResource)
        , CMetalView(InDeviceContext)
    { }

    ~FMetalShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalUnorderedAccessView

class FMetalUnorderedAccessView : public FRHIUnorderedAccessView, public CMetalView
{
public:
    explicit FMetalUnorderedAccessView(CMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : FRHIUnorderedAccessView(InResource)
        , CMetalView(InDeviceContext)
    { }

    ~FMetalUnorderedAccessView() = default;
};

#pragma clang diagnostic pop
