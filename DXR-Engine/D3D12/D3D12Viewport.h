#pragma once
#include <Containers/TArray.h>

#include "RenderLayer/Viewport.h"

#include "Windows/Windows.h"

#include "D3D12Helpers.h"
#include "D3D12Texture.h"
#include "D3D12Views.h"
#include "D3D12CommandContext.h"

class D3D12Viewport : public Viewport, public D3D12DeviceChild
{
public:
    D3D12Viewport(D3D12Device* InDevice, D3D12CommandContext* InCmdContext, HWND InHwnd, EFormat InFormat, UInt32 InWidth, UInt32 InHeight);
    ~D3D12Viewport();

    Bool Init();

    virtual Bool Resize(UInt32 Width, UInt32 Height) override final;

    virtual Bool Present(Bool VerticalSync) override final;

    virtual void SetName(const std::string& Name) override final;

    virtual RenderTargetView* GetRenderTargetView() const override final
    {
        return BackBufferViews[BackBufferIndex].Get();
    }

    virtual Texture2D* GetBackBuffer() const override final
    {
        return BackBuffers[BackBufferIndex].Get();
    }

    virtual Bool IsValid() const
    {
        return SwapChain != nullptr;
    }

    virtual void* GetNativeResource() const
    {
        return reinterpret_cast<void*>(SwapChain.Get());
    }

private:
    Bool RetriveBackBuffers();

    TComPtr<IDXGISwapChain3> SwapChain;
    D3D12CommandContext*     CmdContext;
    HWND   Hwnd  = 0;
    UInt32 Flags = 0;
    UInt32 NumBackBuffers  = 0;
    UInt32 BackBufferIndex = 0;

    TArray<TSharedRef<D3D12Texture2D>>        BackBuffers;
    TArray<TSharedRef<D3D12RenderTargetView>> BackBufferViews;
};