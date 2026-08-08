#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/ArrayView.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12BackBufferProxies.h"
#include "D3D12RHI/D3D12CommandContext.h"

class FD3D12CommandContext;

typedef TSharedRef<class FD3D12SwapChainRHI> FD3D12SwapChainRHIRef;

EFormat GetD3D12DefaultBackBufferFormat();

class FD3D12SwapChainRHI : public FRHISwapChain, public FD3D12DeviceChild
{
public:
    FD3D12SwapChainRHI(FD3D12Device* InDevice, FD3D12CommandContext* InCommandContext, const FRHISwapChainDesc& InSwapChainDesc);
    virtual ~FD3D12SwapChainRHI();

    // FRHISwapChain Interface
    virtual void* GetRHINativeHandle()                                             const override final;
    virtual void* GetRHINativeBackBufferResourceFromIndex(uint32 Index)            const override final;
    virtual void* GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 Index)    const override final;
    virtual void* GetRHINativeBackBufferUnorderedAccessViewFromIndex(uint32 Index) const override final;

    virtual FRHITexture* GetBackBuffer()                              const override final;
    virtual FRHITexture* GetBackBufferResourceFromIndex(uint32 Index) const override final;
    virtual uint32       GetNumBackBufferResources()                  const override final;

    virtual FRHIRenderTargetView*    GetBackBufferRenderTargetView()    const override final;
    virtual FRHIUnorderedAccessView* GetBackBufferUnorderedAccessView() const override final;

    virtual bool IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const override final;
    virtual bool QueryDisplayHDRInfo(FRHIDisplayHDRInfo& OutInfo) const override final;

    bool Initialize(FD3D12CommandContext* InCommandContext);
    
    bool Resize(FD3D12CommandContext* InCommandContext, uint32 Width, uint32 Height, EFormat NewFormat, EColorSpace NewColorSpace);
    bool Present(bool bVerticalSync);
    bool SetHDRMetadata(const FRHIHDRMetadata& Metadata);

    FD3D12TextureRHI* GetCurrentBackBuffer() const
    {
        return BackBuffers.IsValidIndex(static_cast<int32>(BackBufferIndex)) ? BackBuffers[BackBufferIndex].Texture.Get() : nullptr;
    }

    FD3D12RenderTargetViewRHI* GetCurrentBackBufferRenderTargetView() const
    {
        return BackBuffers.IsValidIndex(static_cast<int32>(BackBufferIndex)) ? BackBuffers[BackBufferIndex].RenderTargetView.Get() : nullptr;
    }

    FD3D12UnorderedAccessViewRHI* GetCurrentBackBufferUnorderedAccessView() const
    {
        return BackBuffers.IsValidIndex(static_cast<int32>(BackBufferIndex)) ? BackBuffers[BackBufferIndex].UnorderedAccessView.Get() : nullptr;
    }

    uint32 GetBackBufferCount() const
    {
        return static_cast<uint32>(BackBuffers.Size());
    }

    FD3D12TextureRHI* GetBackBufferAtIndex(uint32 Index) const
    {
        return BackBuffers.IsValidIndex(static_cast<int32>(Index)) ? BackBuffers[Index].Texture.Get() : nullptr;
    }

    FD3D12RenderTargetViewRHI* GetBackBufferRenderTargetViewAtIndex(uint32 Index) const
    {
        return BackBuffers.IsValidIndex(static_cast<int32>(Index)) ? BackBuffers[Index].RenderTargetView.Get() : nullptr;
    }

    FD3D12UnorderedAccessViewRHI* GetBackBufferUnorderedAccessViewAtIndex(uint32 Index) const
    {
        return BackBuffers.IsValidIndex(static_cast<int32>(Index)) ? BackBuffers[Index].UnorderedAccessView.Get() : nullptr;
    }

private:
    bool RetrieveBackBuffers();
    bool ApplyHDRMetadata();
    void ApplySettingsChanges();

    struct FBackBufferData
    {
        FD3D12TextureRHIRef             Texture;
        FD3D12RenderTargetViewRHIRef    RenderTargetView;
        FD3D12UnorderedAccessViewRHIRef UnorderedAccessView;
    };

    TComPtr<IDXGISwapChain3>                       SwapChain;
    TComPtr<IDXGISwapChain4>                       SwapChain4;
    FD3D12CommandContext*                          CommandContext;
    FD3D12BackBufferProxyTextureRHIRef             BackBufferProxy;
    FD3D12BackBufferProxyRenderTargetViewRHIRef    BackBufferProxyRenderTargetView;
    FD3D12BackBufferProxyUnorderedAccessViewRHIRef BackBufferProxyUnorderedAccessView;
    TArray<FBackBufferData>                        BackBuffers;
    HWND                                           Hwnd;
    HANDLE                                         SwapChainWaitableObject;
    EColorSpace                                    CurrentColorSpace;
    uint32                                         Flags;
    uint32                                         NumBackBuffers;
    uint32                                         ActiveFrameLatency;
    uint32                                         BackBufferIndex;
};
