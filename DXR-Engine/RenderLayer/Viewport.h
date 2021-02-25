#pragma once
#include "Resources.h"
#include "ResourceViews.h"

class Viewport : public Resource
{
public:
    Viewport(EFormat InFormat, UInt32 InWidth, UInt32 InHeight)
        : Resource()
        , Format(InFormat)
        , Width(InWidth)
        , Height(InHeight)
    {
    }

    ~Viewport() = default;

    virtual Bool Resize(UInt32 Width, UInt32 Height) = 0;
    virtual Bool Present(Bool VerticalSync) = 0;

    virtual RenderTargetView* GetRenderTargetView() const = 0;
    virtual Texture2D* GetBackBuffer() const = 0;

    UInt32 GetWidth()  const { return Width; }
    UInt32 GetHeight() const { return Height; }

    EFormat GetColorFormat() const { return Format; }

protected:
    UInt32  Width;
    UInt32  Height;
    EFormat Format;
};