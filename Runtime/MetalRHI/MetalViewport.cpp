#include "MetalViewport.h"

#include "Core/Mac/MacRunLoop.h"
#include "Core/Mac/MacThreadMisc.h"


@implementation FMetalWindowView

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    CHECK(self != nil);
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


FMetalViewport::FMetalViewport(FMetalDeviceContext* InDeviceContext, const FRHIViewportDesc& Desc)
    : FRHIViewport(Desc)
    , FMetalObject(InDeviceContext)
    , BackBuffer(nullptr)
    , MetalView(nullptr)
    , Drawable(nil)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        FCocoaWindow* WindowHandle = (FCocoaWindow*)(Desc.WindowHandle);
        
        NSRect Frame;
        Frame.size.width  = Desc.Width;
        Frame.size.height = Desc.Height;
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
    }, NSDefaultRunLoopMode, true);
    
    // Create Event
    MainThreadEvent = static_cast<FMacEvent*>(FMacThreadMisc::CreateEvent(false));

    // Create BackBuffer
    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(
        GetColorFormat(),
        Desc.Width,
        Desc.Height,
        1,
        1,
        ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable);
    
    BackBuffer = new FMetalTexture(InDeviceContext, BackBufferDesc);
    BackBuffer->SetViewport(this);
}

FMetalViewport::~FMetalViewport()
{
    // The view is a UI object and needs to be released on the main-thread
    ExecuteOnMainThread(^
    {
        NSSafeRelease(MetalView);
    }, NSDefaultRunLoopMode, true);
}

bool FMetalViewport::Resize(uint32 InWidth, uint32 InHeight)
{
    SCOPED_AUTORELEASE_POOL();
    
    if (Desc.Width != InWidth || Desc.Height != InHeight)
    {
        ExecuteOnMainThread(^
        {
            CAMetalLayer* MetalLayer = (CAMetalLayer*)MetalView.layer;
            MetalLayer.drawableSize = CGSizeMake(InWidth, InHeight);
        }, NSDefaultRunLoopMode, true);
        
        Desc.Width  = uint16(InWidth);
        Desc.Height = uint16(InHeight);
    }
    
    return true;
}

bool FMetalViewport::Present(bool bVerticalSync)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
    
        CAMetalLayer* MetalLayer = GetMetalLayer();
        if (MetalLayer)
        {
            MetalLayer.displaySyncEnabled = bVerticalSync;
        }
        
        id<MTLDrawable> CurrentDrawable = GetDrawable();
        if (CurrentDrawable)
        {
            [CurrentDrawable present];
        }
    }, NSDefaultRunLoopMode, false);
    
    return true;
}

id<CAMetalDrawable> FMetalViewport::GetDrawable()
{
    SCOPED_AUTORELEASE_POOL();
    
    // This can only be called on the mainthread
    // Check(FPlatformThreadMisc::IsMainThread());

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
    
    id<CAMetalDrawable> CurrentDrawable = GetDrawable();
    return CurrentDrawable ? CurrentDrawable.texture : nil;
}
