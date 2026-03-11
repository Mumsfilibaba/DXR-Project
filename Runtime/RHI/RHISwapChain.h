#pragma once
#include "RHI/RHIResource.h"

struct FRHISwapChainInfo
{
    constexpr FRHISwapChainInfo() noexcept = default;

    constexpr FRHISwapChainInfo(void* InWindowHandle, EFormat InColorFormat, uint16 InWidth, uint16 InHeight) noexcept
        : WindowHandle(InWindowHandle)
        , ColorFormat(InColorFormat)
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    constexpr bool operator==(const FRHISwapChainInfo& Other) const noexcept = default;

    void*   WindowHandle  = nullptr;
    EFormat ColorFormat   = EFormat::Unknown;
    uint16  Width         = 0;
    uint16  Height        = 0;
    bool    bFramePacing  = false;
};

class FRHISwapChain : public FRHIResource
{
protected:
    explicit FRHISwapChain(const FRHISwapChainInfo& InSwapChainInfo)
        : FRHIResource()
        , Info(InSwapChainInfo)
    {
    }

    virtual ~FRHISwapChain() = default;

public:
    virtual FRHITexture* GetBackBuffer() const { return nullptr; }
    virtual void* GetNativeSwapChain() const { return nullptr; }
    virtual void* GetBackBufferRenderTargetView() { return nullptr; }

    EFormat GetColorFormat() const
    {
        return Info.ColorFormat;
    }

    uint32 GetWidth() const
    {
        return Info.Width;
    }

    uint32 GetHeight() const
    {
        return Info.Height;
    }

    const FRHISwapChainInfo& GetInfo() const
    {
        return Info;
    }

protected:
    FRHISwapChainInfo Info;
};