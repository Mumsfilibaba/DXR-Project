#pragma once
#include "Resources.h"
#include "ResourceViews.h"

class Viewport : public Resource
{
public:
    Viewport( EFormat InFormat, uint32 InWidth, uint32 InHeight )
        : Resource()
        , Format( InFormat )
        , Width( InWidth )
        , Height( InHeight )
    {
    }

    ~Viewport() = default;

    virtual bool Resize( uint32 Width, uint32 Height ) = 0;
    virtual bool Present( bool VerticalSync ) = 0;

    virtual RenderTargetView* GetRenderTargetView() const = 0;
    virtual Texture2D* GetBackBuffer() const = 0;

    uint32 GetWidth()  const
    {
        return Width;
    }
    uint32 GetHeight() const
    {
        return Height;
    }

    EFormat GetColorFormat() const
    {
        return Format;
    }

protected:
    uint32  Width;
    uint32  Height;
    EFormat Format;
};