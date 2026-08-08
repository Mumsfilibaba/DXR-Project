#pragma once
#include "Core/Containers/ArrayView.h"
#include "Core/Mac/MacPlatformEvent.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "CoreApplication/Mac/CocoaWindow.h"
#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalDevice.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalSwapChainRHI> FMetalSwapChainRef;

@interface FMetalWindowView : FCocoaWindowView
@end

class FMetalSwapChainRHI : public FRHISwapChain, public FMetalDeviceChild
{
public:
    FMetalSwapChainRHI(FMetalDevice* InDevice, const FRHISwapChainDesc& SwapChainDesc);
    virtual ~FMetalSwapChainRHI();

    // FRHISwapChain Interface
    virtual void* GetRHINativeHandle()                                             const override final;
    virtual void* GetRHINativeBackBufferResourceFromIndex(uint32 Index)            const override final;
    virtual void* GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 Index)    const override final;
    virtual void* GetRHINativeBackBufferUnorderedAccessViewFromIndex(uint32 Index) const override final;

    virtual FRHITexture* GetBackBuffer()                              const override final;
    virtual FRHITexture* GetBackBufferResourceFromIndex(uint32 Index) const override final;
    virtual uint32       GetNumBackBufferResources()                  const override final;

    virtual FRHIRenderTargetView*    GetBackBufferRenderTargetView()    const override final;
    virtual FRHIUnorderedAccessView* GetBackBufferUnorderedAccessView() const override final;

    virtual bool IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const override final;
    virtual bool QueryDisplayHDRInfo(FRHIDisplayHDRInfo& /*OutInfo*/) const override final { return false; }

    bool Initialize();
    
    bool Resize(uint32 InWidth, uint32 InHeight);
    bool Present(bool bVerticalSync);
    bool SetHDRMetadata(const FRHIHDRMetadata& /*Metadata*/) { return false; }

    id<CAMetalDrawable> GetDrawable();
    id<MTLTexture>      GetDrawableTexture();

    FMetalTextureRHI* GetCurrentBackBuffer() const
    {
        return BackBuffer.Get();
    }

    FRHIRenderTargetView* GetCurrentBackBufferRenderTargetView() const
    {
        return nullptr;
    }

    uint32 GetBackBufferCount() const
    {
        return 1;
    }

    FMetalTextureRHI* GetBackBufferAtIndex(uint32 Index) const
    {
        return (Index == 0) ? BackBuffer.Get() : nullptr;
    }

    FRHIRenderTargetView* GetBackBufferRenderTargetViewAtIndex(uint32 /*Index*/) const
    {
        return nullptr;
    }

    CAMetalLayer* GetMetalLayer() const
    {
        return MetalLayer;
    }

    FMetalWindowView* GetMetalView() const
    {
        return MetalView;
    }
    
private:
    FMetalTextureRef    BackBuffer;
    FMetalWindowView*   MetalView;
    CAMetalLayer*       MetalLayer;
    id<CAMetalDrawable> Drawable;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
