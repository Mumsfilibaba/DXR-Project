#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalRHI/MetalDeviceChild.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalView : public FMetalDeviceChild
{
public:
    explicit FMetalView(FMetalDeviceContext* InDeviceContext)
        : FMetalDeviceChild(InDeviceContext)
    {
    }

    id<MTLTexture> GetMTLTexture() const
    {
        return TextureView;
    }
    
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


class FMetalRenderTargetView : public FRHIRenderTargetView, public FMetalView
{
public:
    explicit FMetalRenderTargetView(FMetalDeviceContext* InDeviceContext, const FRHIRenderTargetViewDesc& InDesc)
        : FRHIRenderTargetView(InDesc.Texture)
        , FMetalView(InDeviceContext)
        , MipLevel(InDesc.MipLevel)
        , ArrayIndex(InDesc.ArrayIndex)
    {
    }

    ~FMetalRenderTargetView() = default;

    uint8  GetMipLevel()   const { return MipLevel; }
    uint16 GetArrayIndex() const { return ArrayIndex; }

private:
    uint8  MipLevel;
    uint16 ArrayIndex;
};


class FMetalDepthStencilView : public FRHIDepthStencilView, public FMetalView
{
public:
    explicit FMetalDepthStencilView(FMetalDeviceContext* InDeviceContext, const FRHIDepthStencilViewDesc& InDesc)
        : FRHIDepthStencilView(InDesc.Texture)
        , FMetalView(InDeviceContext)
        , MipLevel(InDesc.MipLevel)
        , ArrayIndex(InDesc.ArrayIndex)
    {
    }

    ~FMetalDepthStencilView() = default;

    uint8  GetMipLevel()   const { return MipLevel; }
    uint16 GetArrayIndex() const { return ArrayIndex; }

private:
    uint8  MipLevel;
    uint16 ArrayIndex;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
