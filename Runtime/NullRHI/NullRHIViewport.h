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
// CNullRHIViewport

class CNullRHIViewport : public CRHIViewport
{
public:
    
    CNullRHIViewport(EFormat InFormat, uint32 InWidth, uint32 InHeight)
        : CRHIViewport(InFormat, InWidth, InHeight)
        , BackBuffer(dbg_new TNullRHITexture<CNullRHITexture2D>(InFormat, Width, Height, 1, 1, ETextureUsageFlags::None, SClearValue()))
        , BackBufferView(dbg_new CNullRHIRenderTargetView())
    { }

    ~CNullRHIViewport() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIViewport Interface

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final
    {
        Width  = InWidth;
        Height = InHeight;
        return true;
    }

    virtual bool Present(bool bVerticalSync) override final { return true; }

    virtual CRHIRenderTargetView* GetRenderTargetView() const override final { return BackBufferView.Get(); }

    virtual CRHITexture2D* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    TSharedRef<CNullRHITexture2D>        BackBuffer;
    TSharedRef<CNullRHIRenderTargetView> BackBufferView;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
