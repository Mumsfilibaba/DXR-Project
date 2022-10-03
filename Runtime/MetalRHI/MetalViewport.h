#pragma once
#include "MetalDeviceContext.h"
#include "MetalTexture.h"

#include "RHI/RHIViewport.h"

#include "Core/Containers/ArrayView.h"
#include "Core/Mac/MacEvent.h"
#include "Core/Mac/MacRunLoop.h"
#include "Core/Platform/PlatformThreadMisc.h"

#include "CoreApplication/Mac/CocoaWindow.h"
#include "CoreApplication/Mac/CocoaWindowView.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalWindowView

@interface FMetalWindowView : FCocoaWindowView
@end

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalViewport

class FMetalViewport 
    : public FMetalObject
    , public FRHIViewport
{
public:
    FMetalViewport(FMetalDeviceContext* InDeviceContext, const FRHIViewportInitializer& Initializer);
    ~FMetalViewport();

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final;

    virtual FRHITexture2D* GetBackBuffer() const override final { return BackBuffer.Get(); }
    
    // TODO: This needs to be a command for Vulkan and Metal since we can be using the texture and present will change the resource
    bool Present(bool bVerticalSync);

public:
    
    /** @return: Returns the current drawable, will release it during next call to present */
    id<CAMetalDrawable> GetDrawable();
    id<MTLTexture>      GetDrawableTexture();
    
    CAMetalLayer* GetMetalLayer() const
    {
        Check(FPlatformThreadMisc::IsMainThread());
        return MetalView ? (CAMetalLayer*)MetalView.layer : nil;
    }

    FMetalWindowView* GetMetalView() const { return MetalView; }
    
private:
    TSharedRef<FMetalTexture2D> BackBuffer;
    
    FMetalWindowView*   MetalView;
    id<CAMetalDrawable> Drawable;
    
    FMacEventRef        MainThreadEvent;
};

#pragma clang diagnostic pop
