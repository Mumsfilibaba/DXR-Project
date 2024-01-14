#include "MacWindow.h"
#include "CocoaWindow.h"
#include "CocoaWindowView.h"
#include "Core/Mac/Mac.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Mac/MacRunLoop.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

static CGFloat CocoaTransformY(CGFloat PosY)
{
    const CGRect DisplayBounds = CGDisplayBounds(CGMainDisplayID());
    return DisplayBounds.size.height - PosY - 1;
}

static void ConvertNSRect(NSScreen* Screen, NSRect* Rect)
{
    // NOTE: NSScreen is a Objective-C object, which is why we can use '.' on the pointer
    Rect->origin.y = Screen.frame.size.height - Rect->origin.y - Rect->size.height;
}

FMacWindow::FMacWindow(FMacApplication* InApplication)
    : FGenericWindow()
    , Application(InApplication)
    , Window(nullptr)
    , WindowView(nullptr)
{
}

FMacWindow::~FMacWindow()
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSSafeRelease(Window);
        NSSafeRelease(WindowView);
    }, NSDefaultRunLoopMode, true);
}

bool FMacWindow::Initialize(const FGenericWindowInitializer& InInitializer)
{
    NSUInteger WindowStyle = 0;
    if (InInitializer.Style != EWindowStyleFlag::None)
    {
        WindowStyle = NSWindowStyleMaskTitled;
        if (InInitializer.Style.IsClosable())
        {
            WindowStyle |= NSWindowStyleMaskClosable;
        }
        if (InInitializer.Style.IsResizeable())
        {
            WindowStyle |= NSWindowStyleMaskResizable;
        }
        if (InInitializer.Style.IsMinimizable())
        {
            WindowStyle |= NSWindowStyleMaskMiniaturizable;
        }
    }
    else
    {
        WindowStyle = NSWindowStyleMaskBorderless;
    }
    
    __block bool bResult = false;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        CGFloat Width     = static_cast<CGFloat>(InInitializer.Width);
        CGFloat Height    = static_cast<CGFloat>(InInitializer.Height);
        CGFloat PositionX = static_cast<CGFloat>(InInitializer.Position.x);
        CGFloat PositionY = static_cast<CGFloat>(InInitializer.Position.y);
        
        // Calculate the max size of the screen, then we clamp the size to this largest size
        NSScreen* MainScreen = [NSScreen mainScreen];
        if (MainScreen)
        {
            NSRect ScreenRect = [MainScreen visibleFrame];
            
            const CGFloat MaxWidth      = ScreenRect.size.width;
            const CGFloat MaxHeight     = ScreenRect.size.height;
            const CGFloat ScreenOriginX = ScreenRect.origin.x;
            const CGFloat ScreenOriginY = ScreenRect.origin.y;
            
            Width  = FMath::Min(Width, MaxWidth);
            Height = FMath::Min(Height, MaxHeight);
            
            PositionX = FMath::Clamp(ScreenOriginX, ScreenOriginX + MaxWidth, PositionX);
            PositionY = FMath::Clamp(ScreenOriginY, ScreenOriginY + MaxHeight, PositionY);
        }
        else
        {
            PositionY = CocoaTransformY(PositionY + Height - 1);
        }

        
        // Create the actual window
        const NSRect WindowRect = NSMakeRect(PositionX, PositionY, Width, Height);
        Window = [[FCocoaWindow alloc] initWithContentRect:WindowRect styleMask:WindowStyle backing:NSBackingStoreBuffered defer:NO];
        if (!Window)
        {
            LOG_ERROR("[FMacWindow]: Failed to create NSWindow");
            return;
        }
        
        
        const NSWindowLevel WindowLevel = InInitializer.Style.IsTopMost() ? NSFloatingWindowLevel : NSNormalWindowLevel;
        [Window setLevel:WindowLevel];
        
        if (InInitializer.Style.IsTitled())
        {
            Window.title = InInitializer.Title.GetNSString();
        }
        
        if (!InInitializer.Style.IsMinimizable())
        {
            [[Window standardWindowButton:NSWindowMiniaturizeButton] setEnabled:NO];
        }
        if (!InInitializer.Style.IsMaximizable())
        {
            [[Window standardWindowButton:NSWindowZoomButton] setEnabled:NO];
        }
        
        NSWindowCollectionBehavior Behavior = NSWindowCollectionBehaviorDefault | NSWindowCollectionBehaviorManaged | NSWindowCollectionBehaviorParticipatesInCycle;
        if (InInitializer.Style.IsResizeable())
        {
            Behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
        }
        else
        {
            Behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
        }
        
        Window.collectionBehavior = Behavior;
        
        // Create a window-view
        WindowView = [[FCocoaWindowView alloc] initWithFrame:WindowRect];
        
        // Set a default background
        NSColor* BackGroundColor = [NSColor colorWithSRGBRed:0.15f green:0.15f blue:0.15f alpha:1.0f];
        
        // Setting this to no disables any notifications about the window closing. Not documented.
        [Window setReleasedWhenClosed:NO];
        [Window setAcceptsMouseMovedEvents:YES];
        [Window setRestorable:NO];
        [Window setHasShadow: YES];
        [Window setDelegate:Window];
        [Window setBackgroundColor:BackGroundColor];
        [Window setContentView:WindowView];
        [Window makeFirstResponder:WindowView];
        
        [NSApp addWindowsItem:Window title:InInitializer.Title.GetNSString() filename:NO];
        
        // Disable tabbing mode
        if ([Window respondsToSelector:@selector(setTabbingMode:)])
        {
            [Window setTabbingMode:NSWindowTabbingModeDisallowed];
        }
        
        // Set styleflags
        StyleParams = InInitializer.Style;

        bResult = true;
    }, NSDefaultRunLoopMode, true);

    return bResult;
}

void FMacWindow::Show(bool bFocusOnActivate)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (bFocusOnActivate)
        {
            [Window orderFront:nil];
        }
        else
        {
            [Window makeKeyAndOrderFront:nil];
        }

        [Window setIsVisible:YES];

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Minimize()
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        [Window miniaturize:Window];
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Maximize()
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (Window.miniaturized)
        {
            [Window deminiaturize:Window];
        }

        [Window zoom:Window];

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Destroy()
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        [Window performClose:Window];
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Restore()
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (Window.miniaturized)
        {
            [Window deminiaturize:Window];
        }
        else if (Window.zoomed)
        {
            [Window zoom:Window];
        }
    
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::ToggleFullscreen()
{
    if (StyleParams.IsResizeable())
    {
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();
            [Window toggleFullScreen:Window];
        }, NSDefaultRunLoopMode, true);
    }
}

bool FMacWindow::IsActiveWindow() const
{
    __block bool bIsKeyWindow;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        bIsKeyWindow = Window.isKeyWindow;
    }, NSDefaultRunLoopMode, true);

    return bIsKeyWindow;
}

bool FMacWindow::IsValid() const
{
   return Window != nullptr;
}

bool FMacWindow::IsMinimized() const
{
    __block bool bIsMinimized;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        bIsMinimized = Window.miniaturized;
    }, NSDefaultRunLoopMode, true);

    return bIsMinimized;
}

bool FMacWindow::IsMaximized() const
{
    __block bool bIsMaximized;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        bIsMaximized = Window.zoomed;
    }, NSDefaultRunLoopMode, true);

    return bIsMaximized;
}

bool FMacWindow::IsChildWindow(const TSharedRef<FGenericWindow>& ChildWindow) const
{
    TSharedRef<FMacWindow> MacChildWindow = StaticCastSharedRef<FMacWindow>(ChildWindow);

    __block bool bIsChildWindow = false;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        for (NSWindow* ChildWindow in Window.childWindows)
        {
            FCocoaWindow* CocoaWindow = NSClassCast<FCocoaWindow>(ChildWindow);
            if (CocoaWindow && CocoaWindow == MacChildWindow->GetWindow())
            {
                bIsChildWindow = true;
                break;
            }
        }
    }, NSDefaultRunLoopMode, true);

    return bIsChildWindow;
}

void FMacWindow::SetWindowFocus()
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        [Window makeKeyAndOrderFront:Window];
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::SetTitle(const FString& InTitle)
{
    SCOPED_AUTORELEASE_POOL();

    __block NSString* Title = InTitle.GetNSString();
    ExecuteOnMainThread(^
    {
        Window.title = Title;
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::GetTitle(FString& OutTitle) const
{
    SCOPED_AUTORELEASE_POOL();
    
    __block NSString* Title;
    ExecuteOnMainThread(^
    {
        Title = Window.title;
    }, NSDefaultRunLoopMode, true);

    OutTitle = FString(Title);
}

void FMacWindow::SetWindowPos(int32 x, int32 y)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        NSRect Frame      = Window.frame;
        NSRect WindowRect = NSMakeRect(x, y, Frame.size.width, Frame.size.height);
        ConvertNSRect(Window.screen, &WindowRect);
        [Window setFrameOrigin:WindowRect.origin];

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::SetWindowOpacity(float Alpha)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        Window.alphaValue = Alpha;
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::SetWindowShape(const FWindowShape& Shape, bool bMove)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        NSRect Frame = Window.frame;
        Frame.size.width  = Shape.Width;
        Frame.size.height = Shape.Height;
        [Window setFrame: Frame display: YES animate: YES];
        
        if (bMove)
        {
            NSRect WindowRect = NSMakeRect(Shape.Position.x, Shape.Position.y, Frame.size.width, Frame.size.height);
            ConvertNSRect(Window.screen, &WindowRect);
            [Window setFrameOrigin:WindowRect.origin];
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::GetWindowShape(FWindowShape& OutWindowShape) const
{
    __block NSRect ContentRect;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        ContentRect = [Window contentRectForFrameRect:Window.frame];
    }, NSDefaultRunLoopMode, true);

    OutWindowShape.Width      = ContentRect.size.width;
    OutWindowShape.Height     = ContentRect.size.height;
    OutWindowShape.Position.x = ContentRect.origin.x;
    OutWindowShape.Position.y = CocoaTransformY(ContentRect.origin.y + ContentRect.size.height - 1);
}

uint32 FMacWindow::GetWidth() const
{
    __block NSSize Size;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        const NSRect ContentRect = Window.contentView.frame;
        Size = ContentRect.size;
    }, NSDefaultRunLoopMode, true);

    return uint32(Size.width);
}

uint32 FMacWindow::GetHeight() const
{
    __block NSSize Size;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        const NSRect ContentRect = Window.contentView.frame;
        Size = ContentRect.size;
    }, NSDefaultRunLoopMode, true);

    return uint32(Size.height);
}

void FMacWindow::GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const
{
    __block NSRect Frame;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        NSScreen* Screen = Window.screen;
        Frame = Screen.frame;
    }, NSDefaultRunLoopMode, true);

    OutWidth  = Frame.size.width;
    OutHeight = Frame.size.height;
}

float FMacWindow::GetWindowDpiScale() const
{
    __block CGFloat Scale;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        const NSRect Points = WindowView.frame;
        const NSRect Pixels = [WindowView convertRectToBacking:Points];

        const CGFloat ScaleX = static_cast<CGFloat>(Pixels.size.width / Points.size.width);
        const CGFloat ScaleY = static_cast<CGFloat>(Pixels.size.height / Points.size.height);
        CHECK(ScaleX == ScaleY);
        Scale = ScaleX;
    }, NSDefaultRunLoopMode, true);

    return static_cast<float>(Scale);
}

void FMacWindow::SetPlatformHandle(void* InPlatformHandle)
{
    if (InPlatformHandle)
    {
        ExecuteOnMainThread(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            // Make sure that the handle sent in is of correct type
            if (FCocoaWindow* NewWindow = NSClassCast<FCocoaWindow>(reinterpret_cast<NSObject*>(InPlatformHandle)))
            {
                if (FCocoaWindowView* NewWindowView = NSClassCast<FCocoaWindowView>(NewWindow.contentView))
                {
                    Window     = NewWindow;
                    WindowView = NewWindowView;
                }
                else
                {
                    LOG_ERROR("WindowView is not of the expected type");
                }
            }
            else
            {
                LOG_ERROR("WindowView is not of the expected type");
            }
        }, NSDefaultRunLoopMode, true);
    }
}

void FMacWindow::SetStyle(FWindowStyle InStyle)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        const NSWindowLevel WindowLevel = InStyle.IsTopMost() ? NSFloatingWindowLevel : NSNormalWindowLevel;
        [Window setLevel:WindowLevel];
        
        const BOOL bMinimizable = InStyle.IsMinimizable() ? YES : NO;
        [[Window standardWindowButton:NSWindowMiniaturizeButton] setEnabled:bMinimizable];

        const BOOL bMaximizable = InStyle.IsMaximizable() ? YES : NO;
        [[Window standardWindowButton:NSWindowZoomButton] setEnabled:bMaximizable];
        
        NSWindowCollectionBehavior Behavior = NSWindowCollectionBehaviorDefault | NSWindowCollectionBehaviorManaged | NSWindowCollectionBehaviorParticipatesInCycle;
        if (InStyle.IsResizeable())
        {
            Behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
        }
        else
        {
            Behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
        }
        
        Window.collectionBehavior = Behavior;
        
        // Set styleflags
        StyleParams = InStyle;
    }, NSDefaultRunLoopMode, true);
}
