#include "MetalViewport.h"

#include "Core/Threading/Mac/MacRunLoop.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalWindowView

@implementation CMetalWindowView

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    Check(self != nil);
    return self;
}

- (BOOL)isOpaque
{
    return YES;
}

- (BOOL)mouseDownCanMoveWindow
{
    return YES;
}

@end

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalViewport

CMetalViewport::CMetalViewport(CMetalDeviceContext* InDeviceContext, const CRHIViewportInitializer& Initializer)
    : CRHIViewport(Initializer)
    , BackBuffer(nullptr)
{
    MakeMainThreadCall(^
    {
        CCocoaWindow* WindowHandle = (CCocoaWindow*)(Initializer.WindowHandle);
        
        CMetalWindowView* MetalView = [[CMetalWindowView alloc] init:WindowHandle.Application];
        Check(MetalView != nil);
        
        WindowHandle.contentView = MetalView;
        [WindowHandle makeFirstResponder:MetalView];
    }, true);
    
    // Create BackBuffer
    CRHITexture2DInitializer BackBufferInitializer(Initializer.ColorFormat, Width, Height, 1, 1, ETextureUsageFlags::AllowRTV, EResourceAccess::Common);
    BackBuffer = dbg_new CMetalTexture2D(InDeviceContext, BackBufferInitializer);
}

bool CMetalViewport::Resize(uint32 InWidth, uint32 InHeight)
{
    Width  = uint16(InWidth);
    Height = uint16(InHeight);
    return true;
}
