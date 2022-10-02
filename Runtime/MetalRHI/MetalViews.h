#pragma once
#include "MetalObject.h"

#include "RHI/RHIResourceViews.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalView

class FMetalView 
    : public FMetalObject
{
public:
    explicit FMetalView(FMetalDeviceContext* InDeviceContext)
        : FMetalObject(InDeviceContext)
    { }

    id<MTLTexture> GetMTLTexture() const { return TextureView; }
    
private:
    id<MTLTexture> TextureView;
    id<MTLBuffer>  Buffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalShaderResourceView

class FMetalShaderResourceView 
    : public FRHIShaderResourceView
    , public FMetalView
{
public:
    explicit FMetalShaderResourceView(FMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : FRHIShaderResourceView(InResource)
        , FMetalView(InDeviceContext)
    { }

    ~FMetalShaderResourceView() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalUnorderedAccessView

class FMetalUnorderedAccessView 
    : public FRHIUnorderedAccessView
    , public FMetalView
{
public:
    explicit FMetalUnorderedAccessView(FMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : FRHIUnorderedAccessView(InResource)
        , FMetalView(InDeviceContext)
    { }

    ~FMetalUnorderedAccessView() = default;
};

#pragma clang diagnostic pop
