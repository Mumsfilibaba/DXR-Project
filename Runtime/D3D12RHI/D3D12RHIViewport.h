#pragma once
#include "Core/Containers/ArrayView.h"

#include "RHI/RHIViewport.h"

#include "Core/Windows/Windows.h"

#include "D3D12Core.h"
#include "D3D12RHITexture.h"
#include "D3D12RHIViews.h"
#include "D3D12RHICommandContext.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Viewport

class CD3D12Viewport : public CRHIViewport, public CD3D12DeviceObject
{
public:

    CD3D12Viewport(CD3D12Device* InDevice, CD3D12RHICommandContext* InCmdContext, HWND InHwnd, EFormat InFormat, uint32 InWidth, uint32 InHeight);
    ~CD3D12Viewport();

    bool Init();

    virtual bool Resize(uint32 Width, uint32 Height) override final;

    virtual bool Present(bool VerticalSync) override final;

    virtual void SetName(const String& Name) override final;

    virtual CRHIRenderTargetView* GetRenderTargetView() const override final
    {
        return BackBufferViews[BackBufferIndex].Get();
    }

    virtual CRHITexture2D* GetBackBuffer() const override final
    {
        return BackBuffers[BackBufferIndex].Get();
    }

    virtual bool IsValid() const
    {
        return SwapChain != nullptr;
    }

    virtual void* GetNativeResource() const
    {
        return reinterpret_cast<void*>(SwapChain.Get());
    }

private:
    bool RetriveBackBuffers();

    TComPtr<IDXGISwapChain3> SwapChain;

    CD3D12RHICommandContext* CmdContext;

    HWND Hwnd = 0;

    uint32 Flags = 0;
    uint32 NumBackBuffers = 0;
    uint32 BackBufferIndex = 0;

    HANDLE SwapChainWaitableObject = 0;

    TArray<TSharedRef<CD3D12Texture2D>>        BackBuffers;
    TArray<TSharedRef<CD3D12RenderTargetView>> BackBufferViews;
};