#pragma once
#include "RHIResources.h"
#include "RHIResourceViews.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIViewport> CRHIViewportRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIViewport

class CRHIViewport : public CRHIObject
{
public:
    
    CRHIViewport(ERHIFormat InFormat, uint16 InWidth, uint16 InHeight)
        : CRHIObject()
        , Width(InWidth)
        , Height(InHeight)
        , Format(InFormat)
    { }

    ~CRHIViewport() = default;

    virtual bool Resize(uint32 Width, uint32 Height) = 0;

    virtual CRHIRenderTargetView* GetRenderTargetView() const = 0;
    
    virtual CRHITexture2D* GetBackBuffer() const = 0;

    inline uint16 GetWidth()  const { return Width; }
    inline uint16 GetHeight() const { return Height; }

    inline ERHIFormat GetColorFormat() const { return Format; }

protected:
    uint16     Width;
    uint16     Height;
    ERHIFormat Format;
};
