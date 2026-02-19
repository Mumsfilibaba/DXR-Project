#pragma once
#include "Core/Containers/ArrayView.h"
#include "Core/Mac/MacEvent.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "CoreApplication/Mac/CocoaWindow.h"
#include "MetalRHI/MetalTexture.h"
#include "MetalRHI/MetalDeviceContext.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalSwapChain> FMetalSwapChainRef;

@interface FMetalWindowView : FCocoaWindowView
@end

class FMetalSwapChain : public FRHISwapChain, public FMetalDeviceChild
{
public:
    FMetalSwapChain(FMetalDeviceContext* InDeviceContext, const FRHISwapChainDesc& SwapChainDesc);
    ~FMetalSwapChain();

    virtual FRHITexture* GetBackBuffer() const override final { return BackBuffer.Get(); }

    bool Initialize();
    bool Resize(uint32 InWidth, uint32 InHeight);
    bool Present(bool bVerticalSync);

    /** @return Returns the current drawable, will release it during next call to present */
    id<CAMetalDrawable> GetDrawable();
    id<MTLTexture> GetDrawableTexture();
    
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
