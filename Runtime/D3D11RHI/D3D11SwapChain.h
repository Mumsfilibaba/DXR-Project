#pragma once
#include "Core/Windows/Windows.h"
#include "RHI/RHIResources.h"
#include "D3D11RHI/D3D11Core.h"
#include "D3D11RHI/D3D11Texture.h"

class FD3D11CommandContext;

typedef TSharedRef<class FD3D11SwapChainRHI> FD3D11SwapChainRHIRef;

class FD3D11SwapChainRHI : public FRHISwapChain, public FD3D11DeviceChild
{
public:
    static EFormat GetDefaultBackBufferFormat();

public:
    FD3D11SwapChainRHI(FD3D11Device* InDevice, FD3D11CommandContext* InCommandContext, const FRHISwapChainDesc& InSwapChainDesc);
    virtual ~FD3D11SwapChainRHI();

    // FRHISwapChain Interface
    virtual void* GetRHINativeHandle()                                   const override final;
    virtual void* GetRHINativeResourceFromIndex(uint32 Index)            const override final;
    virtual void* GetRHINativeRenderTargetViewFromIndex(uint32 Index)    const override final;
    virtual void* GetRHINativeUnorderedAccessViewFromIndex(uint32 Index) const override final;
    virtual void* GetRHINativeShaderResourceViewFromIndex(uint32 Index)  const override final;

    virtual FRHITexture*             GetBackBuffer()          const override final;
    virtual FRHIRenderTargetView*    GetRenderTargetView()    const override final;
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final;
    virtual FRHIShaderResourceView*  GetShaderResourceView()  const override final;

    virtual uint32 GetNumResources() const override final { return 1; }

    virtual bool IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const override final;
    virtual bool QueryDisplayHDRInfo(FRHIDisplayHDRInfo& OutInfo) const override final;

    bool Initialize();
    bool Resize(uint32 Width, uint32 Height, EFormat NewFormat, EColorSpace NewColorSpace);
    bool Present(bool bVerticalSync);
    bool SetHDRMetadata(const FRHIHDRMetadata& Metadata);

private:
    bool CreateBackBuffer();
    bool RetrieveBackBuffer();
    void ReleaseBackBufferResources();
    bool ApplyHDRMetadata();
    void ApplySettingsChanges();

    TComPtr<IDXGISwapChain3> SwapChain;
    TComPtr<IDXGISwapChain4> SwapChain4;
    FD3D11CommandContext*    CommandContext;
    FD3D11TextureRHIRef      BackBuffer;
    HWND                     Hwnd;
    HANDLE                   SwapChainWaitableObject;
    DXGI_HDR_METADATA_TYPE   AppliedHDRMetadataType;
    DXGI_HDR_METADATA_HDR10  AppliedHDR10Metadata;
    EColorSpace              CurrentColorSpace;
    uint32                   Flags;
    uint32                   NumBackBuffers;
    uint32                   ActiveFrameLatency;
};
