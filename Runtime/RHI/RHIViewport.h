#pragma once
#include "RHIResource.h"

struct FRHIViewportInfo
{
    constexpr FRHIViewportInfo() noexcept = default;

    constexpr FRHIViewportInfo(void* InWindowHandle, EFormat InColorFormat, uint16 InWidth, uint16 InHeight) noexcept
        : WindowHandle(InWindowHandle)
        , ColorFormat(InColorFormat)
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    constexpr bool operator==(const FRHIViewportInfo& Other) const noexcept = default;

    void*   WindowHandle = nullptr;
    EFormat ColorFormat  = EFormat::Unknown;
    uint16  Width        = 0;
    uint16  Height       = 0;
};

class FRHIViewport : public FRHIResource
{
protected:
    explicit FRHIViewport(const FRHIViewportInfo& InViewportInfo)
        : FRHIResource()
        , Info(InViewportInfo)
    {
    }

    virtual ~FRHIViewport() = default;

public:
    virtual FRHITexture* GetBackBuffer() const { return nullptr; };

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

    const FRHIViewportInfo& GetInfo() const
    {
        return Info;
    }

protected:
    FRHIViewportInfo Info;
};