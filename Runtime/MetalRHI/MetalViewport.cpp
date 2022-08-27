#include "MetalViewport.h"

#include "Core/Mac/MacRunLoop.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalWindowView

@implementation FMetalWindowView

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
// FMetalViewport

FMetalViewport::FMetalViewport(FMetalDeviceContext* InDeviceContext, const FRHIViewportInitializer& Initializer)
    : FMetalObject(InDeviceContext)
    , FRHIViewport(Initializer)
    , BackBuffer(nullptr)
    , MetalView(nullptr)
    , Drawable(nil)
{
    MakeMainThreadCall(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        FCocoaWindow* WindowHandle = (FCocoaWindow*)(Initializer.WindowHandle);
        
        NSRect Frame;
        Frame.size.width  = Initializer.Width;
        Frame.size.height = Initializer.Height;
        Frame.origin.x    = 0;
        Frame.origin.y    = 0;
        
        MetalView = [[FMetalWindowView alloc] initWithFrame:Frame];
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
    FRHITexture2DInitializer BackBufferInitializer(Initializer.ColorFormat, Width, Height, 1, 1, ETextureUsageFlags::AllowRTV, EResourceAccess::Common);
    BackBuffer = dbg_new FMetalTexture2D(InDeviceContext, BackBufferInitializer);
    
    BackBuffer->SetViewport(this);
}

FMetalViewport::~FMetalViewport()
{
    NSSafeRelease(MetalView);
}

bool FMetalViewport::Resize(uint32 InWidth, uint32 InHeight)
{
    SCOPED_AUTORELEASE_POOL();
    
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

bool FMetalViewport::Present(bool bVerticalSync)
{
    SCOPED_AUTORELEASE_POOL();
    
    id<MTLCommandQueue>  Queue  = GetDeviceContext()->GetMTLCommandQueue();
    id<MTLCommandBuffer> Buffer = [Queue commandBuffer];
    
    CAMetalLayer* MetalLayer = GetMetalLayer();
    if (MetalLayer)
    {
        MetalLayer.displaySyncEnabled = bVerticalSync;
    }
    
    id<MTLDrawable> CurrentDrawable = GetDrawable();
    if (CurrentDrawable)
    {
        [Buffer presentDrawable:CurrentDrawable];
        NSSafeRelease(Drawable);
    }
       
    [Buffer commit];
    
    return true;
}

id<CAMetalDrawable> FMetalViewport::GetDrawable()
{
    SCOPED_AUTORELEASE_POOL();
    
    if (!Drawable)
    {
        CAMetalLayer* MetalLayer = GetMetalLayer();
        Drawable = [MetalLayer nextDrawable];
        
        if (Drawable)
        {
            [Drawable retain];
        }
    }
    
    return Drawable;
}

id<MTLTexture> FMetalViewport::GetDrawableTexture()
{
    SCOPED_AUTORELEASE_POOL();
    
    id<CAMetalDrawable> Drawable = GetDrawable();
    return Drawable ? Drawable.texture : nil;
}
