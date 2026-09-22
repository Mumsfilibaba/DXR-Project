#include "Core/Mac/MacThreadManager.h"
#include "Core/Platform/PlatformEvent.h"
#include "MetalRHI/MetalSwapChain.h"
#include "RHI/RHI.h"
#include <CoreGraphics/CoreGraphics.h>

@implementation FMetalWindowView

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    CHECK(self != nil);
    self.IsOpaqueSurface = YES;
    return self;
}

- (BOOL)isOpaque
{
    return self.IsOpaqueSurface;
}

- (BOOL)mouseDownCanMoveWindow
{
    // See FCocoaWindowView.
    return NO;
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

void* FMetalSwapChainRHI::GetRHINativeResourceFromIndex(uint32 Index) const
{
    UNREFERENCED_VARIABLE(Index);
    FRHITexture* Texture = BackBuffer.Get();
    return Texture ? Texture->GetRHINativeResource() : nullptr;
}

void* FMetalSwapChainRHI::GetRHINativeRenderTargetViewFromIndex(uint32 Index) const
{
    UNREFERENCED_VARIABLE(Index);
    FRHIRenderTargetView* View = GetRenderTargetView();
    return View ? View->GetRHINativeHandle() : nullptr;
}

void* FMetalSwapChainRHI::GetRHINativeUnorderedAccessViewFromIndex(uint32 Index) const
{
    UNREFERENCED_VARIABLE(Index);
    FRHIUnorderedAccessView* View = GetUnorderedAccessView();
    return View ? View->GetRHINativeHandle() : nullptr;
}

void* FMetalSwapChainRHI::GetRHINativeShaderResourceViewFromIndex(uint32 Index) const
{
    UNREFERENCED_VARIABLE(Index);
    FRHIShaderResourceView* View = GetShaderResourceView();
    return View ? View->GetRHINativeHandle() : nullptr;
}

FRHITexture* FMetalSwapChainRHI::GetBackBuffer() const
{
    return BackBuffer.Get();
}

FRHIRenderTargetView* FMetalSwapChainRHI::GetRenderTargetView() const
{
    return BackBuffer ? BackBuffer->GetRenderTargetView() : nullptr;
}

FRHIUnorderedAccessView* FMetalSwapChainRHI::GetUnorderedAccessView() const
{
    return BackBuffer ? BackBuffer->GetUnorderedAccessView() : nullptr;
}

FRHIShaderResourceView* FMetalSwapChainRHI::GetShaderResourceView() const
{
    return BackBuffer ? BackBuffer->GetShaderResourceView() : nullptr;
}

uint32 FMetalSwapChainRHI::GetNumResources() const
{
    return 1;
}

bool FMetalSwapChainRHI::IsFormatSupported(EFormat Format, EColorSpace ColorSpace) const
{
    const MTLPixelFormat PixelFormat = MetalRHI::ConvertFormat(Format);
    if (PixelFormat == MTLPixelFormatInvalid)
    {
        return false;
    }

    if (ColorSpace == EColorSpace::RGB_Full_G22_None_P709)
    {
        return PixelFormat == MTLPixelFormatBGRA8Unorm
            || PixelFormat == MTLPixelFormatBGRA8Unorm_sRGB
            || PixelFormat == MTLPixelFormatRGBA8Unorm
            || PixelFormat == MTLPixelFormatRGBA8Unorm_sRGB;
    }

    if (ColorSpace == EColorSpace::RGB_Full_G10_None_P709)
    {
        return PixelFormat == MTLPixelFormatRGBA16Float;
    }

    if (ColorSpace == EColorSpace::RGB_Full_G2084_None_P2020 || ColorSpace == EColorSpace::RGB_Full_G22_None_P2020)
    {
        FRHIDisplayHDRInfo DisplayInfo;
        if (!QueryDisplayHDRInfo(DisplayInfo))
        {
            return false;
        }

        return PixelFormat == MTLPixelFormatRGBA16Float || PixelFormat == MTLPixelFormatRGB10A2Unorm;
    }

    return false;
}

bool FMetalSwapChainRHI::QueryDisplayHDRInfo(FRHIDisplayHDRInfo& OutInfo) const
{
    OutInfo = FRHIDisplayHDRInfo();

    NSScreen* Screen = nil;
    if (Desc.WindowHandle)
    {
        FCocoaWindow* CocoaWindow = reinterpret_cast<FCocoaWindow*>(Desc.WindowHandle);
        Screen = CocoaWindow.screen;
    }

    if (!Screen)
    {
        Screen = [NSScreen mainScreen];
    }

    if (!Screen)
    {
        return false;
    }

    const CGFloat MaxEDR = Screen.maximumPotentialExtendedDynamicRangeColorComponentValue;
    if (MaxEDR <= 1.0)
    {
        return false;
    }

    NSColorSpace* ScreenColorSpace = Screen.colorSpace;
    CGColorSpaceRef CGSpace = ScreenColorSpace ? ScreenColorSpace.CGColorSpace : nullptr;
    const CFStringRef ColorSpaceName = CGSpace ? CGColorSpaceGetName(CGSpace) : nullptr;

    OutInfo.ColorSpace = EColorSpace::RGB_Full_G10_None_P709;
    if (ColorSpaceName && CFEqual(ColorSpaceName, kCGColorSpaceITUR_2100_PQ))
    {
        OutInfo.ColorSpace = EColorSpace::RGB_Full_G2084_None_P2020;
    }
    else if (ColorSpaceName && CFEqual(ColorSpaceName, kCGColorSpaceITUR_2020))
    {
        OutInfo.ColorSpace = EColorSpace::RGB_Full_G22_None_P2020;
    }

    OutInfo.MinLuminance          = 0.001f;
    OutInfo.MaxLuminance          = static_cast<float>(100.0 * MaxEDR);
    OutInfo.MaxFullFrameLuminance = OutInfo.MaxLuminance;
    OutInfo.BitsPerColor          = 10;
    OutInfo.WhitePoint            = { 0.3127f, 0.3290f };
    return true;
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

    if (Desc.ColorFormat == EFormat::Unknown)
    {
        Desc.ColorFormat = (RHI::DefaultSwapChainFormat != EFormat::Unknown) ? RHI::DefaultSwapChainFormat : EFormat::B8G8R8A8_Unorm;
    }
    else if (!IsFormatSupported(Desc.ColorFormat, Desc.ColorSpace))
    {
        METAL_ERROR("Requested back-buffer (%s, %s) is not supported on this device", ToString(Desc.ColorFormat), ToString(Desc.ColorSpace));
        return false;
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
        
        FCocoaWindow* CocoaWindow = reinterpret_cast<FCocoaWindow*>(Desc.WindowHandle);

        const BOOL bIsOpaqueSurface = Desc.IsTransparent() ? NO : YES;

        MetalView = [[FMetalWindowView alloc] initWithFrame:Frame];
        [MetalView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [MetalView setWantsLayer:YES];
        [MetalView setIsOpaqueSurface:bIsOpaqueSurface];

        NewMetalLayer = [CAMetalLayer new];
        NewMetalLayer.edgeAntialiasingMask       = 0;
        NewMetalLayer.masksToBounds              = YES;
        NewMetalLayer.opaque                     = bIsOpaqueSurface;
        NewMetalLayer.backgroundColor            = bIsOpaqueSurface ? CocoaWindow.backgroundColor.CGColor : nil;
        NewMetalLayer.presentsWithTransaction    = NO;
        NewMetalLayer.anchorPoint                = CGPointMake(0.5, 0.5);
        NewMetalLayer.frame                      = Frame;
        NewMetalLayer.magnificationFilter        = kCAFilterNearest;
        NewMetalLayer.minificationFilter         = kCAFilterNearest;
        NewMetalLayer.contentsGravity            = kCAGravityResize;
        NewMetalLayer.drawableSize               = CGSizeMake(Desc.Width, Desc.Height);
        NewMetalLayer.autoresizingMask           = kCALayerWidthSizable | kCALayerHeightSizable;
        NewMetalLayer.needsDisplayOnBoundsChange = YES;

        NewMetalLayer.actions = @{
            @"bounds"   : [NSNull null],
            @"position" : [NSNull null],
            @"contents" : [NSNull null],
        };

        [NewMetalLayer setDevice:GetDevice()->GetMTLDevice()];
        [NewMetalLayer setFramebufferOnly:NO];
        NewMetalLayer.allowsNextDrawableTimeout = YES;
        NewMetalLayer.pixelFormat = MetalRHI::ConvertFormat(Desc.ColorFormat);
        [NewMetalLayer removeAllAnimations];

        [MetalView setLayer:NewMetalLayer];
        [MetalView setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawDuringViewResize];
        [MetalView retain];

        [CocoaWindow setContentView:MetalView];
        [CocoaWindow makeFirstResponder:MetalView];

        bResult = true;
    }, NSDefaultRunLoopMode, true);
    
    if (!bResult)
    {
        return false;
    }

    MetalLayer = NewMetalLayer;

    if (!ApplyLayerColorSpace())
    {
        METAL_WARNING("Failed to apply swap-chain color space %s", ToString(Desc.ColorSpace));
    }

    if (Desc.ColorSpace == EColorSpace::RGB_Full_G2084_None_P2020 && !Desc.HDRMetadata.bIsValid)
    {
        FRHIHDRMetadata DefaultMetadata = RHI::GetDefaultHDRMetadata();

        FRHIDisplayHDRInfo DisplayInfo;
        if (RHI::ShouldUseDisplayLuminance() && QueryDisplayHDRInfo(DisplayInfo))
        {
            DefaultMetadata.MinMasteringLuminance     = DisplayInfo.MinLuminance;
            DefaultMetadata.MaxMasteringLuminance     = DisplayInfo.MaxLuminance;
            DefaultMetadata.MaxFrameAverageLightLevel = DisplayInfo.MaxFullFrameLuminance;
        }

        SetHDRMetadata(DefaultMetadata);
    }
    else
    {
        ApplyHDRMetadata();
    }

    return RefreshBackBuffer();
}

bool FMetalSwapChainRHI::RefreshBackBuffer()
{
    const ETextureUsageFlags Flags = ETextureUsageFlags::RenderTarget | ETextureUsageFlags::Presentable;
    FRHITextureDesc BackBufferDesc = FRHITextureDesc::CreateTexture2D(Desc.ColorFormat, Desc.Width, Desc.Height, 1, 1, Flags);
    BackBuffer = new FMetalTextureRHI(GetDevice(), BackBufferDesc);
    BackBuffer->SetSwapChain(this);
    if (!BackBuffer->CreateDefaultViews())
    {
        return false;
    }

    return true;
}

bool FMetalSwapChainRHI::ApplyLayerColorSpace()
{
    if (!MetalLayer)
    {
        return false;
    }

    __block bool bApplied = false;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        CFStringRef ColorSpaceName = kCGColorSpaceSRGB;
        BOOL bWantsEDR = NO;
        switch (Desc.ColorSpace)
        {
            case EColorSpace::RGB_Full_G10_None_P709:
                ColorSpaceName = kCGColorSpaceExtendedLinearSRGB;
                bWantsEDR = YES;
                break;
            case EColorSpace::RGB_Full_G2084_None_P2020:
                ColorSpaceName = kCGColorSpaceITUR_2100_PQ;
                bWantsEDR = YES;
                break;
            case EColorSpace::RGB_Full_G22_None_P2020:
                ColorSpaceName = kCGColorSpaceITUR_2020;
                bWantsEDR = YES;
                break;
            default:
                ColorSpaceName = kCGColorSpaceSRGB;
                break;
        }

        CGColorSpaceRef ColorSpace = CGColorSpaceCreateWithName(ColorSpaceName);
        MetalLayer.colorspace = ColorSpace;
        MetalLayer.wantsExtendedDynamicRangeContent = bWantsEDR;
        if (ColorSpace)
        {
            CGColorSpaceRelease(ColorSpace);
        }

        bApplied = true;
    }, NSDefaultRunLoopMode, true);

    return bApplied;
}

bool FMetalSwapChainRHI::ApplyHDRMetadata()
{
    if (!MetalLayer)
    {
        return false;
    }

    __block bool bApplied = false;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        const bool bIsHDRColorSpace = (Desc.ColorSpace == EColorSpace::RGB_Full_G2084_None_P2020);
        if (!Desc.HDRMetadata.bIsValid || !bIsHDRColorSpace)
        {
            MetalLayer.EDRMetadata = nil;
            bApplied = true;
            return;
        }

        const FRHIHDRMetadata& Source = Desc.HDRMetadata;
        MetalLayer.EDRMetadata = [CAEDRMetadata HDR10MetadataWithMinLuminance:Source.MinMasteringLuminance
                                                                maxLuminance:Source.MaxMasteringLuminance
                                                          opticalOutputScale:100.0];
        MetalLayer.wantsExtendedDynamicRangeContent = YES;
        bApplied = MetalLayer.EDRMetadata != nil;
    }, NSDefaultRunLoopMode, true);

    return bApplied;
}

bool FMetalSwapChainRHI::SetHDRMetadata(const FRHIHDRMetadata& Metadata)
{
    Desc.HDRMetadata = Metadata;
    return ApplyHDRMetadata();
}

bool FMetalSwapChainRHI::Resize(uint32 InWidth, uint32 InHeight, EFormat Format, EColorSpace ColorSpace)
{
    SCOPED_AUTORELEASE_POOL();

    const uint32      ResolvedWidth       = (InWidth  > 0u) ? InWidth  : Desc.Width;
    const uint32      ResolvedHeight      = (InHeight > 0u) ? InHeight : Desc.Height;
    const EFormat     EffectiveFormat     = (Format     == EFormat::Unknown)     ? Desc.ColorFormat : Format;
    const EColorSpace EffectiveColorSpace = (ColorSpace == EColorSpace::Unknown) ? Desc.ColorSpace  : ColorSpace;

    const bool bSizeChanged       = (ResolvedWidth != Desc.Width || ResolvedHeight != Desc.Height) && ResolvedWidth > 0u && ResolvedHeight > 0u;
    const bool bFormatChanged     = (Format     != EFormat::Unknown)     && (EffectiveFormat     != Desc.ColorFormat);
    const bool bColorSpaceChanged = (ColorSpace != EColorSpace::Unknown) && (EffectiveColorSpace != Desc.ColorSpace);

    if (bFormatChanged || bColorSpaceChanged)
    {
        if (!IsFormatSupported(EffectiveFormat, EffectiveColorSpace))
        {
            METAL_WARNING("Resize: (%s, %s) not supported on this swap-chain; leaving unchanged.",
                ToString(EffectiveFormat), ToString(EffectiveColorSpace));
            return false;
        }
    }

    if (!bSizeChanged && !bFormatChanged && !bColorSpaceChanged)
    {
        return true;
    }

    if (Drawable)
    {
        [Drawable release];
        Drawable = nullptr;
    }

    if (bSizeChanged)
    {
        Desc.Width  = uint16(ResolvedWidth);
        Desc.Height = uint16(ResolvedHeight);
    }

    if (bFormatChanged)
    {
        Desc.ColorFormat = EffectiveFormat;
    }

    if (bColorSpaceChanged)
    {
        Desc.ColorSpace = EffectiveColorSpace;
    }

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        CAMetalLayer* Layer = GetMetalLayer();
        if (Layer)
        {
            Layer.drawableSize = CGSizeMake(Desc.Width, Desc.Height);
            Layer.pixelFormat  = MetalRHI::ConvertFormat(Desc.ColorFormat);
        }
    }, NSDefaultRunLoopMode, true);

    if (!ApplyLayerColorSpace())
    {
        return false;
    }

    ApplyHDRMetadata();
    return RefreshBackBuffer();
}

bool FMetalSwapChainRHI::Present(id<MTLCommandBuffer> CommandBuffer, bool bVerticalSync)
{
    SCOPED_AUTORELEASE_POOL();

    CAMetalLayer* Layer = GetMetalLayer();
    if (Layer)
    {
        Layer.displaySyncEnabled = bVerticalSync;
    }

    id<CAMetalDrawable> CurrentDrawable = GetDrawable();
    if (!CurrentDrawable)
    {
        return false;
    }

    if (CommandBuffer)
    {
        [CommandBuffer presentDrawable:CurrentDrawable];
    }

    [Drawable release];
    Drawable = nullptr;
    return CommandBuffer != nil;
}

void FMetalSwapChainRHI::AcquireNextBackBuffer()
{
    SCOPED_AUTORELEASE_POOL();

    if (Drawable)
    {
        return;
    }

    CAMetalLayer* Layer = GetMetalLayer();
    if (!Layer)
    {
        METAL_ERROR("AcquireNextBackBuffer: CAMetalLayer is nil");
        return;
    }

    Layer.allowsNextDrawableTimeout = YES;
    Drawable = [Layer nextDrawable];
    if (!Drawable)
    {
        METAL_ERROR("AcquireNextBackBuffer: nextDrawable returned nil");
        return;
    }

    [Drawable retain];
}

id<CAMetalDrawable> FMetalSwapChainRHI::GetDrawable()
{
    return Drawable;
}

id<MTLTexture> FMetalSwapChainRHI::GetDrawableTexture()
{
    SCOPED_AUTORELEASE_POOL();

    id<CAMetalDrawable> CurrentDrawable = GetDrawable();
    return CurrentDrawable ? CurrentDrawable.texture : nil;
}
