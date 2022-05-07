#pragma once
#include "Core/Containers/ArrayView.h"

#include "RHI/RHIViewport.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalViewport

class CMetalViewport : public CRHIViewport
{
public:
    
    CMetalViewport(const CRHIViewportInitializer& Initializer)
        : CRHIViewport(Initializer)
        , BackBuffer(nullptr)
    { 
        CRHITexture2DInitializer BackBufferInitializer(Initializer.ColorFormat, Width, Height, 1, 1, ETextureUsageFlags::AllowRTV, EResourceAccess::Common);
        BackBuffer = dbg_new TMetalTexture<CMetalTexture2D>(BackBufferInitializer);
    }

    ~CMetalViewport() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIViewport Interface

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final
    {
        Width  = uint16(InWidth);
        Height = uint16(InHeight);
        return true;
    }

    virtual bool Present(bool bVerticalSync) override final { return true; }

    virtual CRHITexture2D* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    TSharedRef<CMetalTexture2D> BackBuffer;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
