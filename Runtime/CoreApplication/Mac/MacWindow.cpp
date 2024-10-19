#include "MacWindow.h"
#include "CocoaWindow.h"
#include "CocoaWindowView.h"
#include "Core/Mac/Mac.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Mac/MacRunLoop.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

static void ConvertNSRect(NSScreen* Screen, NSRect* Rect)
{
    if (!Screen)
    {
        Screen = [NSScreen mainScreen];
    }

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
    NSWindowStyleMask WindowStyle = 0;
    if (InInitializer.Style != EWindowStyleFlags::None)
    {
        WindowStyle |= NSWindowStyleMaskTitled;

        if ((InInitializer.Style & EWindowStyleFlags::Closable) != EWindowStyleFlags::None)
        {
            WindowStyle |= NSWindowStyleMaskClosable;
        }
        if ((InInitializer.Style & EWindowStyleFlags::Resizeable) != EWindowStyleFlags::None)
        {
            WindowStyle |= NSWindowStyleMaskResizable;
        }
        if ((InInitializer.Style & EWindowStyleFlags::Minimizable) != EWindowStyleFlags::None)
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
        
        NSScreen* MainScreen = [NSScreen mainScreen];
        NSRect WindowRect = NSMakeRect(PositionX, PositionY, Width, Height);
        ConvertNSRect(MainScreen, &WindowRect);

        if (MainScreen)
        {
            const NSRect ScreenRect = [MainScreen visibleFrame];
            WindowRect.size.width  = FMath::Clamp(CGFloat(0), ScreenRect.size.width, WindowRect.size.width);
            WindowRect.size.height = FMath::Clamp(CGFloat(0), ScreenRect.size.height, WindowRect.size.height);
            WindowRect.origin.x    = FMath::Clamp(ScreenRect.origin.x, ScreenRect.origin.x + ScreenRect.size.width, WindowRect.origin.x);
            WindowRect.origin.y    = FMath::Clamp(ScreenRect.origin.y, ScreenRect.origin.y + ScreenRect.size.height, WindowRect.origin.y);
        }

        Window = [[FCocoaWindow alloc] initWithContentRect:WindowRect styleMask:WindowStyle backing:NSBackingStoreBuffered defer:NO];
        if (!Window)
        {
            LOG_ERROR("[FMacWindow]: Failed to create NSWindow");
            return;
        }

        const NSWindowLevel WindowLevel = (InInitializer.Style & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None ? NSFloatingWindowLevel : NSNormalWindowLevel;
        [Window setLevel:WindowLevel];

        if ((InInitializer.Style & EWindowStyleFlags::Titled) != EWindowStyleFlags::None)
        {
            Window.title = InInitializer.Title.GetNSString();
        }

        if ((InInitializer.Style & EWindowStyleFlags::Minimizable) != EWindowStyleFlags::None)
        {
            [[Window standardWindowButton:NSWindowCloseButton] setEnabled:YES];
        }
        else
        {
            [[Window standardWindowButton:NSWindowCloseButton] setEnabled:NO];
        }
        
        if ((InInitializer.Style & EWindowStyleFlags::Minimizable) != EWindowStyleFlags::None)
        {
            [[Window standardWindowButton:NSWindowMiniaturizeButton] setEnabled:YES];
        }
        else
        {
            [[Window standardWindowButton:NSWindowMiniaturizeButton] setEnabled:NO];
        }
        
        if ((InInitializer.Style & EWindowStyleFlags::Maximizable) != EWindowStyleFlags::None)
        {
            [[Window standardWindowButton:NSWindowZoomButton] setEnabled:YES];
        }
        else
        {
            [[Window standardWindowButton:NSWindowZoomButton] setEnabled:NO];
        }

        NSWindowCollectionBehavior Behavior = NSWindowCollectionBehaviorDefault | NSWindowCollectionBehaviorManaged | NSWindowCollectionBehaviorParticipatesInCycle;
        if ((InInitializer.Style & EWindowStyleFlags::Resizeable) != EWindowStyleFlags::None)
        {
            Behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
        }
        else
        {
            Behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
        }

        Window.collectionBehavior = Behavior;

        NSColor* BackGroundColor = [NSColor colorWithSRGBRed:0.15f green:0.15f blue:0.15f alpha:1.0f];
        WindowView = [[FCocoaWindowView alloc] initWithFrame:WindowRect];
        [Window setReleasedWhenClosed:NO];
        [Window setAcceptsMouseMovedEvents:YES];
        [Window setRestorable:NO];
        [Window setHasShadow: YES];
        [Window setDelegate:Window];
        [Window setBackgroundColor:BackGroundColor];
        [Window setContentView:WindowView];
        [Window makeFirstResponder:WindowView];

        [NSApp addWindowsItem:Window title:InInitializer.Title.GetNSString() filename:NO];

        if ([Window respondsToSelector:@selector(setTabbingMode:)])
        {
            [Window setTabbingMode:NSWindowTabbingModeDisallowed];
        }
        
        [Window setIsVisible:YES];
        
        const NSRect ContentRect  = NSMakeRect(0, 0, Width, Height);
        NSRect NewFrame = [Window frameRectForContentRect:ContentRect];
        NewFrame.origin.x = PositionX;
        NewFrame.origin.y = PositionY;
        ConvertNSRect(Window.screen, &NewFrame);
        [Window setFrame: NewFrame display: YES animate: YES];
       
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
        
        [Window setIsVisible:YES];

        if (bFocusOnActivate)
        {
            [Window orderFront:nil];
        }
        else
        {
            [Window makeKeyAndOrderFront:nil];
        }

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
        
        [Window close];
        Window = nil;
        
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
    if ((StyleParams & EWindowStyleFlags::Resizeable) != EWindowStyleFlags::None)
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
        [Window setTitle:Title];
        [Window setMiniwindowTitle:Title];
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

        NSRect WindowFrame = Window.frame;
        WindowFrame = NSMakeRect(x, y, WindowFrame.size.width, WindowFrame.size.height);
        ConvertNSRect(Window.screen, &WindowFrame);
        [Window setFrameOrigin:WindowFrame.origin];
        
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
       
        NSRect NewContentRect;
        if (bMove)
        {
            NewContentRect = NSMakeRect(Shape.Position.x, Shape.Position.y, Shape.Width, Shape.Height);
        }
        else
        {
            NSRect ContentRect = [Window contentRectForFrameRect:Window.frame];
            ConvertNSRect(Window.screen, &ContentRect);
            NewContentRect = NSMakeRect(ContentRect.origin.x, ContentRect.origin.y, Shape.Width, Shape.Height);
        }

        ConvertNSRect(Window.screen, &NewContentRect);
        const NSRect NewFrame = [NSWindow frameRectForContentRect:NewContentRect styleMask:[Window styleMask]];
        [Window setFrame: NewFrame display: YES animate: YES];
        
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
        ConvertNSRect(Window.screen, &ContentRect);
    }, NSDefaultRunLoopMode, true);

    OutWindowShape.Width      = ContentRect.size.width;
    OutWindowShape.Height     = ContentRect.size.height;
    OutWindowShape.Position.x = ContentRect.origin.x;
    OutWindowShape.Position.y = ContentRect.origin.y;
}

uint32 FMacWindow::GetWidth() const
{
    __block NSSize Size;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        const NSRect ContentRect = [Window contentRectForFrameRect:Window.frame];
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

        const NSRect ContentRect = [Window contentRectForFrameRect:Window.frame];
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

        MAYBE_UNUSED const CGFloat ScaleY = static_cast<CGFloat>(Pixels.size.height / Points.size.height);
        MAYBE_UNUSED const CGFloat ScaleX = static_cast<CGFloat>(Pixels.size.width / Points.size.width);
        
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

void FMacWindow::SetStyle(EWindowStyleFlags InStyle)
{
    NSWindowStyleMask WindowStyle = 0;
    if (InStyle != EWindowStyleFlags::None)
    {
        WindowStyle |= NSWindowStyleMaskTitled;

        if ((InStyle & EWindowStyleFlags::Closable) != EWindowStyleFlags::None)
        {
            WindowStyle |= NSWindowStyleMaskClosable;
        }
        if ((InStyle & EWindowStyleFlags::Resizeable) != EWindowStyleFlags::None)
        {
            WindowStyle |= NSWindowStyleMaskResizable;
        }
        if ((InStyle & EWindowStyleFlags::Minimizable) != EWindowStyleFlags::None)
        {
            WindowStyle |= NSWindowStyleMaskMiniaturizable;
        }
    }
    else
    {
        WindowStyle = NSWindowStyleMaskBorderless;
    }
    
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        const NSWindowLevel WindowLevel = (InStyle & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None ? NSFloatingWindowLevel : NSNormalWindowLevel;
        [Window setLevel:WindowLevel];
        
        if ((InStyle & EWindowStyleFlags::Closable) != EWindowStyleFlags::None)
        {
            [[Window standardWindowButton:NSWindowCloseButton] setEnabled:YES];
        }
        else
        {
            [[Window standardWindowButton:NSWindowCloseButton] setEnabled:NO];
        }
        
        if ((InStyle & EWindowStyleFlags::Minimizable) != EWindowStyleFlags::None)
        {
            [[Window standardWindowButton:NSWindowMiniaturizeButton] setEnabled:YES];
        }
        else
        {
            [[Window standardWindowButton:NSWindowMiniaturizeButton] setEnabled:NO];
        }
        
        if ((InStyle & EWindowStyleFlags::Maximizable) != EWindowStyleFlags::None)
        {
            [[Window standardWindowButton:NSWindowZoomButton] setEnabled:YES];
        }
        else
        {
            [[Window standardWindowButton:NSWindowZoomButton] setEnabled:NO];
        }
        
        NSWindowCollectionBehavior Behavior = NSWindowCollectionBehaviorDefault | NSWindowCollectionBehaviorManaged | NSWindowCollectionBehaviorParticipatesInCycle;
        if ((InStyle & EWindowStyleFlags::Resizeable) != EWindowStyleFlags::None)
        {
            Behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
        }
        else
        {
            Behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
        }
        
        [Window setStyleMask:WindowStyle];
        [Window setCollectionBehavior:Behavior];
        
        // Set styleflags
        StyleParams = InStyle;
    }, NSDefaultRunLoopMode, true);
}
