#pragma once
#include "D3D12Core.h"
#include "D3D12Texture.h"
#include "D3D12ResourceViews.h"
#include "D3D12CommandContext.h"
#include "RHI/RHIResources.h"
#include "Core/Windows/Windows.h"
#include "Core/Containers/ArrayView.h"

typedef TSharedRef<class FD3D12Viewport> FD3D12ViewportRef;

class FD3D12Viewport 
    : public FRHIViewport
    , public FD3D12DeviceChild
{
public:
    FD3D12Viewport(FD3D12Device* InDevice, FD3D12CommandContext* InCmdContext, const FRHIViewportDesc& InDesc);
    ~FD3D12Viewport();

    bool Initialize();
    bool Present(bool VerticalSync);

    FD3D12Texture* GetCurrentBackBuffer() const { return BackBuffers[BackBufferIndex].Get(); }

    virtual bool Resize(uint32 Width, uint32 Height) override final;

    virtual FRHITexture* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    bool RetriveBackBuffers();

    TComPtr<IDXGISwapChain3>   SwapChain;
    FD3D12CommandContext*      CommandContext;

    FD3D12BackBufferTextureRef BackBuffer;
    TArray<FD3D12TextureRef>   BackBuffers;

    HWND   Hwnd = 0;
    HANDLE SwapChainWaitableObject = 0;

    uint32 Flags           = 0;
    uint32 NumBackBuffers  = 0;
    uint32 BackBufferIndex = 0;
};