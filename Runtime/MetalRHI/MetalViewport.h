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

class CMetalViewport : public CRHIViewport
{
public:
    
    CMetalViewport(CMetalDeviceContext* InDeviceContext, const CRHIViewportInitializer& Initializer);
    ~CMetalViewport() = default;

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIViewport Interface

    virtual bool Resize(uint32 InWidth, uint32 InHeight) override final;

    virtual bool Present(bool bVerticalSync) override final { return true; }

    virtual CRHITexture2D* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    TSharedRef<CMetalTexture2D> BackBuffer;
};

#pragma clang diagnostic pop
