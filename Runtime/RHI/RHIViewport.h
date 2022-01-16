#pragma once
#include "RHIResources.h"
#include "RHIResourceViews.h"

class CRHIViewport : public CRHIResource
{
public:

    CRHIViewport(EFormat InFormat, uint32 InWidth, uint32 InHeight)
        : CRHIResource()
        , Width(InWidth)
        , Height(InHeight)
        , Format(InFormat)
    {
    }

    ~CRHIViewport() = default;

    virtual bool Resize(uint32 Width, uint32 Height) = 0;
    virtual bool Present(bool VerticalSync) = 0;

    virtual CRHIRenderTargetView* GetRenderTargetView() const = 0;
    virtual CRHITexture2D* GetBackBuffer() const = 0;

    FORCEINLINE uint32 GetWidth()  const
    {
        return Width;
    }

    FORCEINLINE uint32 GetHeight() const
    {
        return Height;
    }

    FORCEINLINE EFormat GetColorFormat() const
    {
        return Format;
    }

protected:
    uint32  Width;
    uint32  Height;
    EFormat Format;
};