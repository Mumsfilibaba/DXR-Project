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


FMetalViewport::FMetalViewport(FMetalDeviceContext* InDeviceContext, const FRHIViewportInfo& ViewportInfo)
    : FRHIViewport(ViewportInfo)
    , FMetalObject(InDeviceContext)
    , BackBuffer(nullptr)
    , MetalView(nullptr)
    , MetalLayer(nullptr)
    , Drawable(nullptr)
    , MainThreadEvent(nullptr)
{
}

FMetalViewport::~FMetalViewport()
{
    // The view is a UI object and needs to be released on the main-thread
    ExecuteOnMainThread(^
    {
        NSSafeRelease(MetalView);
        NSSafeRelease(MetalLayer);
    }, NSDefaultRunLoopMode, true);
}

bool FMetalViewport::Initialize()
{
    if (!Info.WindowHandle)
    {
        LOG_ERROR("WindowHandle cannot be null");
        return false;
    }

    __block bool bResult = false;
    __block CAMetalLayer* NewMetalLayer = nullptr;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        NSRect Frame;
        Frame.size.width  = Info.Width;
        Frame.size.height = Info.Height;
        Frame.origin.x    = 0;
        Frame.origin.y    = 0;
        
        MetalView = [[FMetalWindowView alloc] initWithFrame:Frame];
        [MetalView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [MetalView setWantsLayer:YES];
        
        const CGFloat BackgroundColor[] = { 0.0, 0.0, 0.0, 0.0 }; 
        NewMetalLayer = [CAMetalLayer new];
        NewMetalLayer.edgeAntialiasingMask    = 0;
        NewMetalLayer.masksToBounds           = YES;
        NewMetalLayer.backgroundColor         = CGColorCreate(CGColorSpaceCreateDeviceRGB(), BackgroundColor);
        NewMetalLayer.presentsWithTransaction = NO;
        NewMetalLayer.anchorPoint             = CGPointMake(0.5, 0.5);
        NewMetalLayer.frame                   = Frame;
        NewMetalLayer.magnificationFilter     = kCAFilterNearest;
        NewMetalLayer.minificationFilter      = kCAFilterNearest;

        [NewMetalLayer setDevice:GetDeviceContext()->GetMTLDevice()];
        [NewMetalLayer setFramebufferOnly:NO];
        [NewMetalLayer removeAllAnimations];

        [MetalView setLayer:NewMetalLayer];
        [MetalView retain];
        
        FCocoaWindow* CocoaWindow = reinterpret_cast<FCocoaWindow*>(Info.WindowHandle);
        [CocoaWindow setContentView:MetalView];
        [CocoaWindow makeFirstResponder:MetalView];

        bResult = true;
    }, NSDefaultRunLoopMode, true);
    
    if (!bResult)
    {
        return false;
    }

    // Set the metallayer
    MetalLayer = NewMetalLayer;

    // Create Event
    MainThreadEvent = static_cast<FMacEvent*>(FMacThreadMisc::CreateEvent(false));
    if (!MainThreadEvent)
    {
        return false;
    }

    // Create BackBuffer
    const ETextureUsageFlags Flags = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable;

    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(GetColorFormat(), Info.Width, Info.Height, 1, 1, Flags);
    BackBuffer = new FMetalTexture(GetDeviceContext(), BackBufferDesc);
    BackBuffer->SetViewport(this);
    return true;
}

bool FMetalViewport::Resize(uint32 InWidth, uint32 InHeight)
{
    SCOPED_AUTORELEASE_POOL();
    
    if (Info.Width != InWidth || Info.Height != InHeight)
    {
        ExecuteOnMainThread(^
        {
            CAMetalLayer* MetalLayer = GetMetalLayer();
            if (MetalLayer)
            {
                MetalLayer.drawableSize = CGSizeMake(InWidth, InHeight);
            }
        }, NSDefaultRunLoopMode, true);
        
        Info.Width  = uint16(InWidth);
        Info.Height = uint16(InHeight);
    }
    
    return true;
}

bool FMetalViewport::Present(bool bVerticalSync)
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
        [Drawable release];
        Drawable = nullptr;
    }
        
    return true;
}

id<CAMetalDrawable> FMetalViewport::GetDrawable()
{
    SCOPED_AUTORELEASE_POOL();
    
    // This can only be called on the mainthread
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
