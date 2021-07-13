#pragma once
#include "Core/Containers/ArrayView.h"

#include "RenderLayer/Viewport.h"

#include "Windows/Windows.h"

#include "D3D12Helpers.h"
#include "D3D12Texture.h"
#include "D3D12Views.h"
#include "D3D12CommandContext.h"

class D3D12Viewport : public Viewport, public D3D12DeviceChild
{
public:
    D3D12Viewport( D3D12Device* InDevice, D3D12CommandContext* InCmdContext, HWND InHwnd, EFormat InFormat, uint32 InWidth, uint32 InHeight );
    ~D3D12Viewport();

    bool Init();

    virtual bool Resize( uint32 Width, uint32 Height ) override final;

    virtual bool Present( bool VerticalSync ) override final;

    virtual void SetName( const std::string& Name ) override final;

    virtual RenderTargetView* GetRenderTargetView() const override final
    {
        return BackBufferViews[BackBufferIndex].Get();
    }

    virtual Texture2D* GetBackBuffer() const override final
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

    D3D12CommandContext* CmdContext;

    HWND Hwnd = 0;

    uint32 Flags = 0;
    uint32 NumBackBuffers = 0;
    uint32 BackBufferIndex = 0;

    HANDLE SwapChainWaitableObject = 0;

    TArray<TSharedRef<D3D12Texture2D>>        BackBuffers;
    TArray<TSharedRef<D3D12RenderTargetView>> BackBufferViews;
};