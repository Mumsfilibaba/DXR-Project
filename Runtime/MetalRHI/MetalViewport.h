#pragma once
#include "MetalDeviceContext.h"
#include "MetalTexture.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Mac/MacEvent.h"
#include "Core/Mac/MacRunLoop.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "CoreApplication/Mac/CocoaWindow.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalViewport> FMetalViewportRef;

@interface FMetalWindowView : FCocoaWindowView
@end

class FMetalViewport : public FRHIViewport, public FMetalDeviceChild
{
public:
    FMetalViewport(FMetalDeviceContext* InDeviceContext, const FRHIViewportInfo& ViewportInfo);
    ~FMetalViewport();

    virtual FRHITexture* GetBackBuffer() const override final { return BackBuffer.Get(); }

    bool Initialize();
    bool Resize(uint32 InWidth, uint32 InHeight);
    bool Present(bool bVerticalSync);

    /** @return - Returns the current drawable, will release it during next call to present */
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
