#pragma once
#include "RHIResource.h"

struct FRHIViewportInfo
{
    FRHIViewportInfo()
        : WindowHandle(nullptr)
        , ColorFormat(EFormat::Unknown)
        , Width(0)
        , Height(0)
    {
    }

    FRHIViewportInfo(void* InWindowHandle, EFormat InColorFormat, uint16 InWidth, uint16 InHeight)
        : WindowHandle(InWindowHandle)
        , ColorFormat(InColorFormat)
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    bool operator==(const FRHIViewportInfo& Other) const
    {
        return WindowHandle == Other.WindowHandle && ColorFormat == Other.ColorFormat && Width == Other.Width && Height == Other.Height;
    }

    bool operator!=(const FRHIViewportInfo& Other) const
    {
        return !(*this == Other);
    }

    void*   WindowHandle;
    EFormat ColorFormat;
    uint16  Width;
    uint16  Height;
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