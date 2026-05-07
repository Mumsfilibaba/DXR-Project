#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalRHI/MetalDeviceChild.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalView : public FMetalDeviceChild
{
public:
    FMetalView(FMetalDevice* InDevice);
    virtual ~FMetalView();

    id<MTLTexture> GetMTLTexture() const
    {
        return TextureView;
    }

private:
    id<MTLTexture> TextureView;
    // id<MTLBuffer>  Buffer;
};

class FMetalShaderResourceViewRHI : public FRHIShaderResourceView, public FMetalView
{
public:
    FMetalShaderResourceViewRHI(FMetalDevice* InDevice, FRHIResource* InResource);
    virtual ~FMetalShaderResourceViewRHI();

    // FRHIShaderResourceView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;
};

class FMetalUnorderedAccessViewRHI : public FRHIUnorderedAccessView, public FMetalView
{
public:
    FMetalUnorderedAccessViewRHI(FMetalDevice* InDevice, FRHIResource* InResource);
    virtual ~FMetalUnorderedAccessViewRHI();

    // FRHIUnorderedAccessView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;
};

class FMetalRenderTargetViewRHI : public FRHIRenderTargetView, public FMetalView
{
public:
    FMetalRenderTargetViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIRenderTargetViewDesc& InDesc);
    virtual ~FMetalRenderTargetViewRHI();

    // FRHIRenderTargetView Interface
    virtual void* GetRHINativeHandle() const override final;

    uint8  GetMipLevel()   const { return MipLevel; }
    uint16 GetArrayIndex() const { return ArrayIndex; }

private:
    uint8  MipLevel;
    uint16 ArrayIndex;
};

class FMetalDepthStencilViewRHI : public FRHIDepthStencilView, public FMetalView
{
public:
    FMetalDepthStencilViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIDepthStencilViewDesc& InDesc);
    virtual ~FMetalDepthStencilViewRHI();

    // FRHIDepthStencilView Interface
    virtual void* GetRHINativeHandle() const override final;

    uint8                  GetMipLevel()   const { return MipLevel; }
    uint16                 GetArrayIndex() const { return ArrayIndex; }
    EDepthStencilViewFlags GetFlags()      const { return Flags; }

private:
    uint8                  MipLevel;
    uint16                 ArrayIndex;
    EDepthStencilViewFlags Flags;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
