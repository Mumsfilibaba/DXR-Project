#pragma once
#include "MetalObject.h"

#include "RHI/RHIResourceViews.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalShaderResourceView

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

class CMetalShaderResourceView : public CRHIShaderResourceView, public CMetalView
{
public:
    explicit CMetalShaderResourceView(CMetalDeviceContext* InDeviceContext, CRHIResource* InResource)
        : CRHIShaderResourceView(InResource)
        , CMetalView(InDeviceContext)
    { }

    ~CMetalShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalUnorderedAccessView

class CMetalUnorderedAccessView : public CRHIUnorderedAccessView, public CMetalView
{
public:
    explicit CMetalUnorderedAccessView(CMetalDeviceContext* InDeviceContext, CRHIResource* InResource)
        : CRHIUnorderedAccessView(InResource)
        , CMetalView(InDeviceContext)
    { }

    ~CMetalUnorderedAccessView() = default;
};

#pragma clang diagnostic pop
