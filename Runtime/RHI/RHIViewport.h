#pragma once
#include "RHIResources.h"
#include "RHIResourceViews.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIViewport> CRHIViewportRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIViewport

class CRHIViewport : public CRHIResource
{
public:
    
    CRHIViewport(ERHIFormat InColorFormat, uint16 InWidth, uint16 InHeight)
        : CRHIResource()
        , Width(InWidth)
        , Height(InHeight)
        , ColorFormat(InColorFormat)
    { }

    ~CRHIViewport() = default;

    virtual bool Resize(uint32 Width, uint32 Height) = 0;
    
    virtual CRHITexture* GetBackBuffer() const = 0;

    inline ERHIFormat GetColorFormat() const { return ColorFormat; }

    inline uint16 GetWidth()  const { return Width; }
    inline uint16 GetHeight() const { return Height; }

protected:
    ERHIFormat ColorFormat;
    uint16     Width;
    uint16     Height;
};
