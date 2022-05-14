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
    : CMetalObject(InDeviceContext)
    , CRHIViewport(Initializer)
    , BackBuffer(nullptr)
{
    MakeMainThreadCall(^
    {
        CCocoaWindow* WindowHandle = (CCocoaWindow*)(Initializer.WindowHandle);
        
        NSRect Frame;
        Frame.size.width  = Initializer.Width;
        Frame.size.height = Initializer.Height;
        Frame.origin.x    = 0;
        Frame.origin.y    = 0;
        
        MetalView = [[CMetalWindowView alloc] initWithFrame:Frame];
        [MetalView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [MetalView setWantsLayer:YES];
        
        const CGFloat BackgroundColor[] = { 0.0, 0.0, 0.0, 0.0 };
        
        CAMetalLayer* MetalLayer = [CAMetalLayer new];
        MetalLayer.edgeAntialiasingMask    = 0;
        MetalLayer.masksToBounds           = YES;
        MetalLayer.backgroundColor         = CGColorCreate(CGColorSpaceCreateDeviceRGB(), BackgroundColor);
        MetalLayer.presentsWithTransaction = NO;
        MetalLayer.anchorPoint             = CGPointMake(0.5, 0.5);
        MetalLayer.frame                   = Frame;
        MetalLayer.magnificationFilter     = kCAFilterNearest;
        MetalLayer.minificationFilter      = kCAFilterNearest;

        [MetalLayer setDevice:InDeviceContext->GetMTLDevice()];
        
        [MetalLayer setFramebufferOnly:NO];
        [MetalLayer removeAllAnimations];

        [MetalView setLayer:MetalLayer];
        [MetalView retain];
        
        WindowHandle.contentView = MetalView;
        [WindowHandle makeFirstResponder:MetalView];
    }, true);
    
    // Create BackBuffer
    CRHITexture2DInitializer BackBufferInitializer(Initializer.ColorFormat, Width, Height, 1, 1, ETextureUsageFlags::AllowRTV, EResourceAccess::Common);
    BackBuffer = dbg_new CMetalTexture2D(InDeviceContext, BackBufferInitializer);
}

CMetalViewport::~CMetalViewport()
{
    NSSafeRelease(MetalView);
}

bool CMetalViewport::Resize(uint32 InWidth, uint32 InHeight)
{
    if ((Width != InWidth) || (Height != InHeight))
    {
        MakeMainThreadCall(^
        {
            CAMetalLayer* MetalLayer = (CAMetalLayer*)MetalView.layer;
            MetalLayer.drawableSize = CGSizeMake(InWidth, InHeight);
        }, true);
        
        Width  = uint16(InWidth);
        Height = uint16(InHeight);
    }
    
    return true;
}

bool CMetalViewport::Present(bool bVerticalSync)
{
    UNREFERENCED_VARIABLE(bVerticalSync);
    
    id<MTLCommandQueue>  Queue  = GetDeviceContext()->GetMTLCommandQueue();
    id<MTLCommandBuffer> Buffer = [Queue commandBuffer];
    
    CAMetalLayer*   MetalLayer = (CAMetalLayer*)MetalView.layer;
    id<MTLDrawable> CurrentDrawable = nil;
    
    if (MetalLayer)
    {
        MetalLayer.displaySyncEnabled = bVerticalSync;
        CurrentDrawable = [MetalLayer nextDrawable];
    }
    
    if (CurrentDrawable)
    {
        [Buffer presentDrawable:CurrentDrawable];
    }
       
    [Buffer commit];
    [Buffer waitUntilCompleted];
    
    return true;
}
