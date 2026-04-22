#pragma once
#include "RHI/RHIResource.h"

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
    virtual FRHITexture* GetBackBuffer() const { return nullptr; }
    virtual void* GetNativeSwapChain() const { return nullptr; }
    virtual void* GetBackBufferRenderTargetView() { return nullptr; }

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