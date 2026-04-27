#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/ArrayView.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12Texture.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12CommandContext.h"

class FD3D12CommandContext;

typedef TSharedRef<class FD3D12SwapChainRHI> FD3D12SwapChainRHIRef;

class FD3D12SwapChainRHI : public FRHISwapChain, public FD3D12DeviceChild
{
public:
    FD3D12SwapChainRHI(FD3D12Device* InDevice, FD3D12CommandContext* InCommandContext, const FRHISwapChainDesc& InSwapChainDesc);
    virtual ~FD3D12SwapChainRHI();

    // FRHISwapChain Interface
    virtual void*  GetRHINativeHandle()                                          const override final;
    virtual void*  GetRHINativeBackBufferResourceFromIndex(uint32 Index)         const override final;
    virtual void*  GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 Index) const override final;
    virtual uint32 GetRHINativeBackBufferCount()                                 const override final;

    virtual FRHITexture*          GetBackBuffer()                 const override final;
    virtual FRHIRenderTargetView* GetBackBufferRenderTargetView() const override final;

    bool Initialize(FD3D12CommandContext* InCommandContext);
    bool Resize(FD3D12CommandContext* InCommandContext, uint32 Width, uint32 Height);
    bool Present(bool bVerticalSync);

    FD3D12TextureRHI* GetCurrentBackBuffer() const
    {
        return BackBuffers.IsValidIndex(static_cast<int32>(BackBufferIndex)) ? BackBuffers[BackBufferIndex].Texture.Get() : nullptr;
    }

    FD3D12RenderTargetViewRHI* GetCurrentBackBufferRenderTargetView() const
    {
        return BackBuffers.IsValidIndex(static_cast<int32>(BackBufferIndex)) ? BackBuffers[BackBufferIndex].RenderTargetView.Get() : nullptr;
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

private:
    bool RetrieveBackBuffers();
    void ApplySettingsChanges();

    struct FBackBufferData
    {
        FD3D12TextureRHIRef          Texture;
        FD3D12RenderTargetViewRHIRef RenderTargetView;
    };

    TComPtr<IDXGISwapChain3>                    SwapChain;
    FD3D12CommandContext*                       CommandContext;
    FD3D12BackBufferProxyTextureRHIRef          BackBufferProxy;
    FD3D12BackBufferProxyRenderTargetViewRHIRef BackBufferProxyRenderTargetView;
    TArray<FBackBufferData>                     BackBuffers;
    HWND                                        Hwnd;
    HANDLE                                      SwapChainWaitableObject;
    uint32                                      Flags;
    uint32                                      NumBackBuffers;
    uint32                                      ActiveFrameLatency;
    uint32                                      BackBufferIndex;
};
