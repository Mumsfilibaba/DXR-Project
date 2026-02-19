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

    virtual FRHITexture* GetBackBuffer() const override final { return BackBufferProxy.Get(); }

    bool Initialize(FD3D12CommandContext* InCommandContext);
    bool Resize(FD3D12CommandContext* InCommandContext, uint32 Width, uint32 Height);
    bool Present(bool bVerticalSync);

    FD3D12TextureRHI* GetCurrentBackBuffer() const 
    { 
        return BackBuffers[BackBufferIndex].Get();
    }

private:
    bool RetrieveBackBuffers();

    TComPtr<IDXGISwapChain3>   SwapChain;
    FD3D12CommandContext*      CommandContext;
    FD3D12BackBufferTextureRef BackBufferProxy;
    TArray<FD3D12TextureRHIRef>   BackBuffers;
    HWND                       Hwnd;
    HANDLE                     SwapChainWaitableObject;
    uint32                     Flags;
    uint32                     NumBackBuffers;
    uint32                     BackBufferIndex;
};
