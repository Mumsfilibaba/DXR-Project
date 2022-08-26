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
// CMetalShaderResourceView

class CMetalShaderResourceView : public FRHIShaderResourceView, public CMetalView
{
public:
    explicit CMetalShaderResourceView(CMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : FRHIShaderResourceView(InResource)
        , CMetalView(InDeviceContext)
    { }

    ~CMetalShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalUnorderedAccessView

class CMetalUnorderedAccessView : public FRHIUnorderedAccessView, public CMetalView
{
public:
    explicit CMetalUnorderedAccessView(CMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : FRHIUnorderedAccessView(InResource)
        , CMetalView(InDeviceContext)
    { }

    ~CMetalUnorderedAccessView() = default;
};

#pragma clang diagnostic pop
