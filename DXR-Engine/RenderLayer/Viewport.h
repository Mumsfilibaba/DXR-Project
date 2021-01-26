#pragma once
#include "Resource.h"

class Viewport : public PipelineResource
{
public:
    Viewport(UInt32 InWidth, UInt32 InHeight, EFormat InPixelFormat)
        : Width(InWidth)
        , Height(InHeight)
        , PixelFormat(InPixelFormat)
    {
    }

    virtual Bool Resize(UInt32 Width, UInt32 Height) = 0;
    virtual Bool Present(Bool VerticalSync) = 0;

    virtual class RenderTargetView* GetRenderTargetView() const = 0;
    virtual class Texture2D* GetBackBuffer() const = 0;

    FORCEINLINE UInt32 GetWidth() const
    {
        return Width;
    }

    FORCEINLINE UInt32 GetHeight() const
    {
        return Height;
    }

    FORCEINLINE EFormat GetColorFormat() const
    {
        return PixelFormat;
    }

protected:
    UInt32 Width;
    UInt32 Height;
    EFormat PixelFormat;
};