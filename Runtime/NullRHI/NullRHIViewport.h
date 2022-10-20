#pragma once
#include "Core/Containers/ArrayView.h"

#include "RHI/RHIViewport.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

class FNullRHIViewport 
    : public FRHIViewport
{
public:
    FNullRHIViewport(const FRHIViewportInitializer& Initializer)
        : FRHIViewport(Initializer)
        , BackBuffer(nullptr)
    { 
        FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(
            Initializer.ColorFormat,
            Width,
            Height,
            1,
            1,
            ETextureUsageFlags::Presentable | ETextureUsageFlags::RenderTarget);

        BackBuffer = dbg_new FNullRHITexture(BackBufferDesc);
    }

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final
    {
        Width  = uint16(InWidth);
        Height = uint16(InHeight);
        return true;
    }

    virtual FRHITexture* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    TSharedRef<FNullRHITexture> BackBuffer;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
