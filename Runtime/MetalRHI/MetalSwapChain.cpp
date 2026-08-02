#include "Core/Mac/MacThreadManager.h"
#include "Core/Platform/PlatformEvent.h"
#include "MetalRHI/MetalSwapChain.h"

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


FMetalSwapChainRHI::FMetalSwapChainRHI(FMetalDevice* InDevice, const FRHISwapChainDesc& SwapChainDesc)
    : FRHISwapChain(SwapChainDesc)
    , FMetalDeviceChild(InDevice)
    , BackBuffer(nullptr)
    , MetalView(nullptr)
    , MetalLayer(nullptr)
    , Drawable(nullptr)
{
}

FMetalSwapChainRHI::~FMetalSwapChainRHI()
{
    // The view is a UI object and needs to be released on the main-thread
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        [MetalView release];
        [MetalLayer release];
    }, NSDefaultRunLoopMode, true);
}

void* FMetalSwapChainRHI::GetRHINativeHandle() const
{
    return (__bridge void*)MetalLayer;
}

void* FMetalSwapChainRHI::GetRHINativeBackBufferResourceFromIndex(uint32 Index) const
{
    UNREFERENCED_VARIABLE(Index);
    FRHITexture* Texture = BackBuffer.Get();
    return Texture ? Texture->GetRHINativeResource() : nullptr;
}

void* FMetalSwapChainRHI::GetRHINativeBackBufferRenderTargetViewFromIndex(uint32 Index) const
{
    UNREFERENCED_VARIABLE(Index);
    return nullptr;
}

void* FMetalSwapChainRHI::GetRHINativeBackBufferUnorderedAccessViewFromIndex(uint32 Index) const
{
    UNREFERENCED_VARIABLE(Index);
    return nullptr;
}

FRHITexture* FMetalSwapChainRHI::GetBackBuffer() const
{
    return BackBuffer.Get();
}

FRHITexture* FMetalSwapChainRHI::GetBackBufferResourceFromIndex(uint32 Index) const
{
    UNREFERENCED_VARIABLE(Index);
    return BackBuffer.Get();
}

uint32 FMetalSwapChainRHI::GetNumBackBufferResources() const
{
    return 1;
}

FRHIRenderTargetView* FMetalSwapChainRHI::GetBackBufferRenderTargetView() const
{
    return nullptr;
}

FRHIUnorderedAccessView* FMetalSwapChainRHI::GetBackBufferUnorderedAccessView() const
{
    return nullptr;
}

bool FMetalSwapChainRHI::IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const
{
    return Format != EFormat::Unknown && ColorSpace == EColorSpace::RGB_Full_G22_None_P709;
}

bool FMetalSwapChainRHI::Initialize()
{
    if (!Desc.WindowHandle)
    {
        LOG_ERROR("WindowHandle cannot be null");
        return false;
    }

    if (Desc.ColorSpace == EColorSpace::Unknown)
    {
        Desc.ColorSpace = EColorSpace::RGB_Full_G22_None_P709;
    }

    __block bool bResult = false;
    __block CAMetalLayer* NewMetalLayer = nullptr;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        NSRect Frame;
        Frame.size.width  = Desc.Width;
        Frame.size.height = Desc.Height;
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

        [NewMetalLayer setDevice:GetDevice()->GetMTLDevice()];
        [NewMetalLayer setFramebufferOnly:NO];
        [NewMetalLayer removeAllAnimations];

        [MetalView setLayer:NewMetalLayer];
        [MetalView retain];
        
        FCocoaWindow* CocoaWindow = reinterpret_cast<FCocoaWindow*>(Desc.WindowHandle);
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

    // Create BackBuffer
    const ETextureUsageFlags Flags = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable;

    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(Desc.ColorFormat, Desc.Width, Desc.Height, 1, 1, Flags);
    BackBuffer = new FMetalTextureRHI(GetDevice(), BackBufferDesc);
    BackBuffer->SetSwapChain(this);
    return true;
}

bool FMetalSwapChainRHI::Resize(uint32 InWidth, uint32 InHeight)
{
    SCOPED_AUTORELEASE_POOL();
    
    if (Desc.Width != InWidth || Desc.Height != InHeight)
    {
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            CAMetalLayer* MetalLayer = GetMetalLayer();
            if (MetalLayer)
            {
                MetalLayer.drawableSize = CGSizeMake(InWidth, InHeight);
            }
        }, NSDefaultRunLoopMode, true);
        
        Desc.Width  = uint16(InWidth);
        Desc.Height = uint16(InHeight);
    }
    
    return true;
}

bool FMetalSwapChainRHI::Present(bool bVerticalSync)
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

id<CAMetalDrawable> FMetalSwapChainRHI::GetDrawable()
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

id<MTLTexture> FMetalSwapChainRHI::GetDrawableTexture()
{
    SCOPED_AUTORELEASE_POOL();
    
    id<CAMetalDrawable> CurrentDrawable = GetDrawable();
    return CurrentDrawable ? CurrentDrawable.texture : nil;
}
