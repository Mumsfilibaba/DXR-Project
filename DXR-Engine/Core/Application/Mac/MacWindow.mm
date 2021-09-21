#if defined(PLATFORM_MACOS)
#include "MacWindow.h"
#include "ScopedAutoreleasePool.h"
#include "CocoaWindow.h"
#include "CocoaContentView.h"

#include "Core/Application/Platform/PlatformApplicationMisc.h"

CMacWindow::CMacWindow( CMacApplication* InApplication )
    : CGenericWindow()
    , Application( InApplication )
    , Window(nullptr)
    , View(nullptr)
{
}

CMacWindow::~CMacWindow()
{
    SCOPED_AUTORELEASE_POOL();
     
    [Window release];
    [View release];
}

bool CMacWindow::Init( const std::string& InTitle, uint32 Width, uint32 Height, SWindowStyle InStyle )
{
    SCOPED_AUTORELEASE_POOL();
        
    NSUInteger WindowStyle = 0;
    if (InStyle.Style)
    {
        WindowStyle = NSWindowStyleMaskTitled;
        if (InStyle.IsClosable())
        {
            WindowStyle |= NSWindowStyleMaskClosable;
        }
        if (InStyle.IsResizeable())
        {
            WindowStyle |= NSWindowStyleMaskResizable;
        }
        if (InStyle.IsMinimizable())
        {
            WindowStyle |= NSWindowStyleMaskMiniaturizable;
        }
    }
    else
    {
        WindowStyle = NSWindowStyleMaskBorderless;
    }
    
    const NSRect WindowRect = NSMakeRect(0.0f, 0.0f, CGFloat(Width), CGFloat(Height));
    Window = [[CCocoaWindow alloc] init:Application ContentRect:WindowRect StyleMask:WindowStyle Backing:NSBackingStoreBuffered Defer:NO];
    if (!Window)
    {
        LOG_ERROR("[CMacWindow]: Failed to create NSWindow");
        return false;
    }
    
    View = [[CCocoaContentView alloc] init:Application];
    if (!View)
    {
        LOG_ERROR("[CMacWindow]: Failed to create CocoaContentView");
        return false;
    }
    
    if (InStyle.IsTitled())
    {
        NSString* Title = [NSString stringWithUTF8String:InTitle.c_str()];
        [Window setTitle:Title];
    }
    
    [Window setAcceptsMouseMovedEvents:YES];
    [Window setContentView:View];
    [Window setRestorable:NO];
    [Window makeFirstResponder:View];
    
    // Disable fullscreen toggle if window is not resizeable
    NSWindowCollectionBehavior Behavior = NSWindowCollectionBehaviorManaged;
    if (InStyle.IsResizeable())
    {
        Behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
    }
    else
    {
        Behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
    }
    
    [Window setCollectionBehavior:Behavior];
    
    // Set styleflags
    Style = InStyle;

    return true;
}

void CMacWindow::Show( bool Maximized )
{
   //MacMainThread::MakeCall(^
   //{
       [Window makeKeyAndOrderFront:Window];
    
        if ( Maximized )
        {
            [Window zoom:Window];
        }
    
        PlatformApplicationMisc::PumpMessages( true );
   //}, true);
}

void CMacWindow::Close()
{
    if (Style.IsClosable())
    {
       //MacMainThread::MakeCall(^
       //{
           [Window performClose:Window];
       //}, true);
        
        PlatformApplicationMisc::PumpMessages( true );
   }
}

void CMacWindow::Minimize()
{
   if (Style.IsMinimizable())
   {
       //MacMainThread::MakeCall(^
       //{
           [Window miniaturize:Window];
       //}, true);
       
       PlatformApplicationMisc::PumpMessages( true );
   }
}

void CMacWindow::Maximize()
{
   if (Style.IsMaximizable())
   {
       //MacMainThread::MakeCall(^
       //{
           if ([Window isMiniaturized])
           {
               [Window deminiaturize:Window];
           }
           
           [Window zoom:Window];
       
       PlatformApplicationMisc::PumpMessages( true );
       //}, true);
   }
}

bool CMacWindow::IsActiveWindow() const
{
   NSWindow* KeyWindow = [NSApp keyWindow];
   return KeyWindow == Window;
}

void CMacWindow::Restore()
{
   //MacMainThread::MakeCall(^
   //{
       if ([Window isMiniaturized])
       {
           [Window deminiaturize:Window];
       }
       
       if ([Window isZoomed])
       {
           [Window zoom:Window];
       }
    
    PlatformApplicationMisc::PumpMessages( true );
   //}, true);
}

void CMacWindow::ToggleFullscreen()
{
   if (Style.IsResizeable())
   {
       //MacMainThread::MakeCall(^
       //{
           [Window toggleFullScreen:Window];
       //, true);
   }
}

void CMacWindow::SetTitle( const std::string& InTitle )
{
   SCOPED_AUTORELEASE_POOL();
   
   NSString* Title = [NSString stringWithUTF8String:InTitle.c_str()];
   if (Style.IsTitled())
   {
       //MacMainThread::MakeCall(^
       //{
           [Window setTitle:Title];
       //}, true);
   }
}

void CMacWindow::GetTitle( std::string& OutTitle )
{
    if (Style.IsTitled())
    {
        NSString* Title = [Window title];
        NSInteger Length = [Title length];
        OutTitle.resize(Length);
        
        const char* UTF8Title = [Title UTF8String];
        Memory::Memcpy(OutTitle.data(), UTF8Title, sizeof(char) * Length);
    }
}

void CMacWindow::SetWindowShape( const SWindowShape& Shape, bool Move )
{
    NSRect Frame = [Window frame];
    if (Style.IsResizeable())
    {
        Frame.size.width  = Shape.Width;
        Frame.size.height = Shape.Height;
        [Window setFrame: Frame display: YES animate: YES];
    }
    
    if (Move)
    {
        // TODO: Make sure this is correct
        [Window setFrameOrigin:NSMakePoint(Shape.Position.x, Shape.Position.y - Frame.size.height + 1)];
    }
    
    PlatformApplicationMisc::PumpMessages( true );
}

void CMacWindow::GetWindowShape( SWindowShape& OutWindowShape ) const
{
    NSRect Frame       = [Window frame];
    NSRect ContentRect = [Window contentRectForFrameRect:[Window frame]];
    OutWindowShape.Width      = ContentRect.size.width;
    OutWindowShape.Height     = ContentRect.size.height;
    OutWindowShape.Position.x = Frame.origin.x;
    OutWindowShape.Position.y = Frame.origin.y;
}

uint32 CMacWindow::GetWidth() const
{
   SCOPED_AUTORELEASE_POOL();
   
   /*__block*/ NSRect ContentRect;
   //MacMainThread::MakeCall(^
   //{
    ContentRect = [Window contentRectForFrameRect:[Window frame]];
   //}, true);
   
   return uint32(ContentRect.size.width);
}

uint32 CMacWindow::GetHeight() const
{
   SCOPED_AUTORELEASE_POOL();
   
   /*__block*/ NSRect ContentRect;
   //MacMainThread::MakeCall(^
   //{
    ContentRect = [Window contentRectForFrameRect:[Window frame]];
   //}, true);
   
   return uint32(ContentRect.size.height);
}

#endif
