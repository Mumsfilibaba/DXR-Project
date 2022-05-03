#pragma once
#include "Core/Containers/ArrayView.h"

#include "RHI/RHIViewport.h"

#include "Core/Windows/Windows.h"

#include "D3D12Core.h"
#include "D3D12Texture.h"
#include "D3D12Views.h"
#include "D3D12CommandContext.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Viewport

class CD3D12Viewport : public CRHIViewport, public CD3D12DeviceChild
{
public:

    CD3D12Viewport(CD3D12Device* InDevice, CD3D12CommandContext* InCmdContext, const CRHIViewportInitializer& Initializer);
    ~CD3D12Viewport();

    bool Init();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIViewport Interface

    virtual bool Resize(uint32 Width, uint32 Height) override final;

    virtual bool Present(bool VerticalSync) override final;

    virtual CRHIRenderTargetView* GetRenderTargetView() const override final { return BackBufferViews[BackBufferIndex].Get(); }

    virtual CRHITexture2D* GetBackBuffer() const override final { return BackBuffers[BackBufferIndex].Get(); }

private:
    bool RetriveBackBuffers();

    TComPtr<IDXGISwapChain3> SwapChain;

    CD3D12CommandContext*    CmdContext;

    HWND                     Hwnd = 0;

    uint32                   Flags           = 0;
    uint32                   NumBackBuffers  = 0;
    uint32                   BackBufferIndex = 0;

    HANDLE                   SwapChainWaitableObject = 0;

    TArray<TSharedRef<CD3D12Texture2D>>     BackBuffers;
    TArray<TSharedRef<CD3D12RenderTargetView>> BackBufferViews;
};