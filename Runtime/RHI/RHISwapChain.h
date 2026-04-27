#pragma once
#include "RHI/RHIResource.h"

class FRHIRenderTargetView;

struct FRHISwapChainDesc
{
    constexpr FRHISwapChainDesc() noexcept = default;

    constexpr FRHISwapChainDesc(void* InWindowHandle, EFormat InColorFormat, uint16 InWidth, uint16 InHeight) noexcept
        : WindowHandle(InWindowHandle)
        , ColorFormat(InColorFormat)
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    constexpr bool operator==(const FRHISwapChainDesc& Other) const noexcept = default;

    void*   WindowHandle  = nullptr;
    EFormat ColorFormat   = EFormat::Unknown;
    uint16  Width         = 0;
    uint16  Height        = 0;
    bool    bFramePacing  = false;
};

class FRHISwapChain : public FRHIResource
{
protected:
    explicit FRHISwapChain(const FRHISwapChainDesc& InSwapChainDesc)
        : FRHIResource()
        , Desc(InSwapChainDesc)
    {
    }

    virtual ~FRHISwapChain() = default;

public:
    virtual FRHITexture*          GetBackBuffer()                 const = 0;
    virtual FRHIRenderTargetView* GetBackBufferRenderTargetView() const = 0;

    // D3D12: IDXGISwapChain*. Vulkan: VkSwapchainKHR. Metal: CAMetalLayer*. Null: nullptr.
    virtual void* GetRHINativeHandle() const = 0;
    
    // Native back-buffer resource at Index. Same semantics as FRHITexture::GetRHINativeResource.
    virtual void* GetRHINativeBackBufferResourceFromIndex(uint32 Index) const = 0;
    
    // Native RTV handle for the back-buffer at Index. Same semantics as FRHIRenderTargetView::GetRHINativeHandle.
    virtual void* GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 Index) const = 0;

    // Number of back-buffers owned by this swap-chain.
    virtual uint32 GetRHINativeBackBufferCount() const = 0;

    EFormat GetColorFormat() const
    {
        return Desc.ColorFormat;
    }

    uint32 GetWidth() const
    {
        return Desc.Width;
    }

    uint32 GetHeight() const
    {
        return Desc.Height;
    }

    const FRHISwapChainDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHISwapChainDesc Desc;
};