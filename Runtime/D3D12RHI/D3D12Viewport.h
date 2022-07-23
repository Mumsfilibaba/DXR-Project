#pragma once
#include "Core/Containers/ArrayView.h"

#include "RHI/RHIViewport.h"

#include "Core/Windows/Windows.h"

#include "D3D12Core.h"
#include "D3D12Texture.h"
#include "D3D12ResourceViews.h"
#include "D3D12CommandContext.h"

typedef TSharedRef<class FD3D12Viewport> FD3D12ViewportRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Viewport

class FD3D12Viewport : public FRHIViewport, public FD3D12DeviceChild
{
public:

    FD3D12Viewport(FD3D12Device* InDevice, FD3D12CommandContext* InCmdContext, const FRHIViewportInitializer& Initializer);
    ~FD3D12Viewport();

    bool Initialize();

    bool Present(bool VerticalSync);

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIViewport Interface

    virtual bool Resize(uint32 Width, uint32 Height) override final;

    virtual FRHITexture2D* GetBackBuffer() const override final { return BackBuffers[BackBufferIndex].Get(); }

private:
    bool RetriveBackBuffers();

    TComPtr<IDXGISwapChain3>   SwapChain;

    FD3D12CommandContext*      CommandContext;

    HWND                       Hwnd = 0;

    uint32                     Flags           = 0;
    uint32                     NumBackBuffers  = 0;
    uint32                     BackBufferIndex = 0;

    HANDLE                     SwapChainWaitableObject = 0;

    TArray<FD3D12Texture2DRef> BackBuffers;
};