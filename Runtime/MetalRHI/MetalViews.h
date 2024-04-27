#pragma once
#include "MetalDeviceChild.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalView : public FMetalDeviceChild
{
public:
    explicit FMetalView(FMetalDeviceContext* InDeviceContext)
        : FMetalDeviceChild(InDeviceContext)
    {
    }

    id<MTLTexture> GetMTLTexture() const { return TextureView; }
    
private:
    id<MTLTexture> TextureView;
    // id<MTLBuffer>  Buffer;
};


class FMetalShaderResourceView : public FRHIShaderResourceView, public FMetalView
{
public:
    explicit FMetalShaderResourceView(FMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : FRHIShaderResourceView(InResource)
        , FMetalView(InDeviceContext)
    {
    }

    ~FMetalShaderResourceView() = default;
};


class FMetalUnorderedAccessView : public FRHIUnorderedAccessView, public FMetalView
{
public:
    explicit FMetalUnorderedAccessView(FMetalDeviceContext* InDeviceContext, FRHIResource* InResource)
        : FRHIUnorderedAccessView(InResource)
        , FMetalView(InDeviceContext)
    {
    }

    ~FMetalUnorderedAccessView() = default;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
