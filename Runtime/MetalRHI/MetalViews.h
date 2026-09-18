#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalDeviceChild.h"

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

    id<MTLBuffer> GetMTLBuffer() const
    {
        return BufferView;
    }

    uint64 GetBufferOffset() const
    {
        return BufferOffset;
    }

    uint64 GetBufferSize() const
    {
        return BufferSize;
    }

protected:
    bool InitializeTextureView(FRHITexture* InTexture, EFormat InFormat, EViewDimension InViewDimension, 
        uint32 InFirstMip, uint32 InNumMips, uint32 InFirstSlice, uint32 InNumSlices);
    bool InitializeBufferView(FRHIBuffer* InBuffer, uint64 InOffset, uint64 InSize);

private:
    id<MTLTexture> TextureView;
    id<MTLBuffer>  BufferView;
    uint64         BufferOffset;
    uint64         BufferSize;
};

class FMetalShaderResourceViewRHI : public FRHIShaderResourceView, public FMetalView
{
public:
    FMetalShaderResourceViewRHI(FMetalDevice* InDevice, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc);
    virtual ~FMetalShaderResourceViewRHI();

    // FRHIShaderResourceView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    bool Initialize();
};

class FMetalUnorderedAccessViewRHI : public FRHIUnorderedAccessView, public FMetalView
{
public:
    FMetalUnorderedAccessViewRHI(FMetalDevice* InDevice, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc);
    virtual ~FMetalUnorderedAccessViewRHI();

    // FRHIUnorderedAccessView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    bool Initialize();
};

class FMetalRenderTargetViewRHI : public FRHIRenderTargetView, public FMetalView
{
public:
    FMetalRenderTargetViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIRenderTargetViewDesc& InDesc);
    virtual ~FMetalRenderTargetViewRHI();

    // FRHIRenderTargetView Interface
    virtual void* GetRHINativeHandle() const override final;

    bool Initialize();

    uint8 GetMipLevel() const
    {
        return MipLevel;
    }

    uint16 GetArrayIndex() const
    {
        return ArrayIndex;
    }

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

    bool Initialize();

    uint8 GetMipLevel() const
    {
        return MipLevel;
    }

    uint16 GetArrayIndex() const
    {
        return ArrayIndex;
    }

    EDepthStencilViewFlags GetFlags() const
    {
        return Flags;
    }

private:
    uint8                  MipLevel;
    uint16                 ArrayIndex;
    EDepthStencilViewFlags Flags;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
