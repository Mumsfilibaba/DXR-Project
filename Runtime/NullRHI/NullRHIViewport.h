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
// FNullRHIViewport

class FNullRHIViewport : public FRHIViewport
{
public:
    
    FNullRHIViewport(const FRHIViewportInitializer& Initializer)
        : FRHIViewport(Initializer)
        , BackBuffer(nullptr)
    { 
        FRHITexture2DInitializer BackBufferInitializer(Initializer.ColorFormat, Width, Height, 1, 1, ETextureUsageFlags::AllowRTV, EResourceAccess::Common);
        BackBuffer = dbg_new FNullRHITexture2D(BackBufferInitializer);
    }

    ~FNullRHIViewport() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIViewport Interface

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final
    {
        Width  = uint16(InWidth);
        Height = uint16(InHeight);
        return true;
    }

    virtual FRHITexture2D* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    TSharedRef<FNullRHITexture2D> BackBuffer;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
