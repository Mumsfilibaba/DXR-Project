#pragma once
#include "MetalDeviceContext.h"
#include "MetalTexture.h"

#include "RHI/RHIViewport.h"

#include "Core/Containers/ArrayView.h"
#include "Core/Threading/Mac/MacRunLoop.h"

#include "CoreApplication/Mac/CocoaWindow.h"
#include "CoreApplication/Mac/CocoaWindowView.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalWindowView

@interface CMetalWindowView : CCocoaWindowView
@end

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalViewport

class CMetalViewport : public CMetalObject, public CRHIViewport
{
public:
    
    CMetalViewport(CMetalDeviceContext* InDeviceContext, const CRHIViewportInitializer& Initializer);
    ~CMetalViewport();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIViewport Interface

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final;

    // TODO: This needs to be a command for Vulkan and Metal since we can be using the texture and present will change the resource
    virtual bool Present(bool bVerticalSync) override final;

    virtual CRHITexture2D* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    TSharedRef<CMetalTexture2D> BackBuffer;
    CMetalWindowView*           MetalView;
};

#pragma clang diagnostic pop
