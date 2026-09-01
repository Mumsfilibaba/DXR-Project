#include "Core/Mac/Mac.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Threading/ScopedLock.h"
#include "CoreApplication/Mac/MacWindow.h"
#include "CoreApplication/Mac/CocoaWindow.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

static NSWindowStyleMask GetCocoaWindowStyle(EWindowStyleFlags InStyle)
{
    const EWindowStyleFlags DecorationMask =
        EWindowStyleFlags::Titled |
        EWindowStyleFlags::Closable |
        EWindowStyleFlags::Resizable |
        EWindowStyleFlags::Minimizable |
        EWindowStyleFlags::Maximizable;

    if ((InStyle & DecorationMask) == EWindowStyleFlags::None)
    {
        return NSWindowStyleMaskBorderless;
    }

    NSWindowStyleMask WindowStyle = NSWindowStyleMaskTitled;

    if ((InStyle & EWindowStyleFlags::Closable) != EWindowStyleFlags::None)
    {
        WindowStyle |= NSWindowStyleMaskClosable;
    }

    if ((InStyle & EWindowStyleFlags::Resizable) != EWindowStyleFlags::None)
    {
        WindowStyle |= NSWindowStyleMaskResizable;
    }

    if ((InStyle & EWindowStyleFlags::Minimizable) != EWindowStyleFlags::None)
    {
        WindowStyle |= NSWindowStyleMaskMiniaturizable;
    }

    // The content view spans the whole frame so the application can draw over the caption.
    if ((InStyle & EWindowStyleFlags::CustomTitleBar) != EWindowStyleFlags::None)
    {
        WindowStyle |= NSWindowStyleMaskFullSizeContentView;
    }

    return WindowStyle;
}

TSharedRef<FMacWindow> FMacWindow::Create(FMacApplication* InApplication)
{
    TSharedRef<FMacWindow> NewWindow = new FMacWindow(InApplication);
    return NewWindow;
}

FMacWindow::FMacWindow(FMacApplication* InApplication)
    : FRefCountedBase()
    , Application(InApplication)
    , CocoaWindow(nullptr)
    , CocoaWindowView(nullptr)
    , StyleParams(EWindowStyleFlags::None)
    , bAcceptsInput(true)
{
}

FMacWindow::~FMacWindow()
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        [CocoaWindowView release];
    }, NSDefaultRunLoopMode, true);
}

bool FMacWindow::Initialize(const FPlatformWindowDesc& InDesc)
{
    const NSWindowStyleMask WindowStyle = GetCocoaWindowStyle(InDesc.Style);
    const bool bIsTransientPopup = WindowStyle == NSWindowStyleMaskBorderless;

    __block bool bResult = false;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        CGFloat Width     = static_cast<CGFloat>(InDesc.Width);
        CGFloat Height    = static_cast<CGFloat>(InDesc.Height);
        CGFloat PositionX = static_cast<CGFloat>(InDesc.Position.X);
        CGFloat PositionY = static_cast<CGFloat>(InDesc.Position.Y);

        const NSRect WindowRect = FMacApplication::ConvertEngineRectToCocoa(Width, Height, PositionX, PositionY);
        CocoaWindow = [[FCocoaWindow alloc] initWithContentRect:WindowRect styleMask:WindowStyle backing:NSBackingStoreBuffered defer:NO];
        if (!CocoaWindow)
        {
            LOG_ERROR("[FMacWindow]: Failed to create NSWindow");
            return;
        }

        CocoaWindow.IsTransientPopup = bIsTransientPopup ? YES : NO;

        const NSWindowLevel WindowLevel = (InDesc.Style & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None ? NSFloatingWindowLevel : NSNormalWindowLevel;
        [CocoaWindow setLevel:WindowLevel];

        bAcceptsInput = InDesc.bAcceptsInput;
        [CocoaWindow setIgnoresMouseEvents:!bAcceptsInput];

        if ((InDesc.Style & EWindowStyleFlags::Titled) != EWindowStyleFlags::None)
        {
            CocoaWindow.title = InDesc.Title.GetNSString();
        }

        // AppKit keeps compositing the traffic lights above the content view, so they survive this untouched.
        if ((InDesc.Style & EWindowStyleFlags::CustomTitleBar) != EWindowStyleFlags::None)
        {
            [CocoaWindow setTitlebarAppearsTransparent:YES];
            [CocoaWindow setTitleVisibility:NSWindowTitleHidden];
        }

        if ((InDesc.Style & EWindowStyleFlags::Closable) != EWindowStyleFlags::None)
        {
            [[CocoaWindow standardWindowButton:NSWindowCloseButton] setEnabled:YES];
        }
        else
        {
            [[CocoaWindow standardWindowButton:NSWindowCloseButton] setEnabled:NO];
        }
        
        if ((InDesc.Style & EWindowStyleFlags::Minimizable) != EWindowStyleFlags::None)
        {
            [[CocoaWindow standardWindowButton:NSWindowMiniaturizeButton] setEnabled:YES];
        }
        else
        {
            [[CocoaWindow standardWindowButton:NSWindowMiniaturizeButton] setEnabled:NO];
        }
        
        if ((InDesc.Style & EWindowStyleFlags::Maximizable) != EWindowStyleFlags::None)
        {
            [[CocoaWindow standardWindowButton:NSWindowZoomButton] setEnabled:YES];
        }
        else
        {
            [[CocoaWindow standardWindowButton:NSWindowZoomButton] setEnabled:NO];
        }

        NSWindowCollectionBehavior Behavior = NSWindowCollectionBehaviorManaged;
        if (bIsTransientPopup)
        {
            Behavior |= NSWindowCollectionBehaviorTransient;
            Behavior |= NSWindowCollectionBehaviorIgnoresCycle;
        }
        else
        {
            Behavior |= NSWindowCollectionBehaviorParticipatesInCycle;
        }

        CocoaWindow.collectionBehavior = Behavior;

        if ((InDesc.Style & EWindowStyleFlags::Opaque) == EWindowStyleFlags::None)
        {
            [CocoaWindow setOpaque:NO];
            [CocoaWindow setHasShadow:NO];
        }
        else
        {
            [CocoaWindow setHasShadow: YES];
        }
        
        NSColor* BackGroundColor = [NSColor colorWithSRGBRed:0.15f green:0.15f blue:0.15f alpha:1.0f];
        CocoaWindowView = [[FCocoaWindowView alloc] initWithFrame:WindowRect];
        
        [CocoaWindow setReleasedWhenClosed:NO];
        [CocoaWindow setAcceptsMouseMovedEvents:YES];
        [CocoaWindow setRestorable:NO];
        [CocoaWindow setDelegate:CocoaWindow];
        [CocoaWindow setBackgroundColor:BackGroundColor];
        [CocoaWindow setContentView:CocoaWindowView];
        [CocoaWindow makeFirstResponder:CocoaWindowView];

        if (InDesc.ParentWindow)
        {
            FMacWindow* ParentMacWindow = static_cast<FMacWindow*>(InDesc.ParentWindow);
            if (FCocoaWindow* ParentCocoaWindow = ParentMacWindow->GetCocoaWindow())
            {
                [ParentCocoaWindow addChildWindow:CocoaWindow ordered:NSWindowAbove];
            }
        }

        if ((InDesc.Style & EWindowStyleFlags::NoTaskBarIcon) == EWindowStyleFlags::None)
        {
            [NSApp addWindowsItem:CocoaWindow title:InDesc.Title.GetNSString() filename:NO];
        }

        if ([CocoaWindow respondsToSelector:@selector(setTabbingMode:)])
        {
            [CocoaWindow setTabbingMode:NSWindowTabbingModeDisallowed];
        }
        
        Position    = IntVector2(static_cast<int32>(WindowRect.origin.x), static_cast<int32>(WindowRect.origin.y));
        StyleParams = InDesc.Style;
        bResult     = true;
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);

    return bResult;
}

void FMacWindow::Destroy()
{
    if (CocoaWindow)
    {
        SCOPED_AUTORELEASE_POOL();

        FCocoaWindow* WindowToRemove = CocoaWindow;
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            if (NSWindow* ParentCocoaWindow = [WindowToRemove parentWindow])
            {
                [ParentCocoaWindow removeChildWindow:WindowToRemove];
            }

            [NSApp removeWindowsItem:WindowToRemove];
        }, NSDefaultRunLoopMode, true);

        TSharedRef<FMacWindow> ThisWindow = MakeSharedRef<FMacWindow>(this);
        Application->OnWindowDestroyed(ThisWindow);
        CocoaWindow = nullptr;
    }
}

void FMacWindow::Show(bool bFocus)
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            [CocoaWindow setIsVisible:YES];

            if (bFocus)
            {
                [CocoaWindow makeKeyAndOrderFront:nil];
            }
            else
            {
                [CocoaWindow orderFront:nil];
            }
        }

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Minimize()
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            [CocoaWindow miniaturize:CocoaWindow];
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Maximize()
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            if (CocoaWindow.miniaturized)
            {
                [CocoaWindow deminiaturize:CocoaWindow];
            }

            [CocoaWindow zoom:CocoaWindow];
        }

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Restore()
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            if (CocoaWindow.miniaturized)
            {
                [CocoaWindow deminiaturize:CocoaWindow];
            }
            else if (CocoaWindow.zoomed)
            {
                [CocoaWindow zoom:CocoaWindow];
            }
        }
    
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::ToggleFullscreen()
{
    if ((StyleParams & EWindowStyleFlags::Resizable) != EWindowStyleFlags::None)
    {
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            if (CocoaWindow)
            {
                [CocoaWindow toggleFullScreen:CocoaWindow];
            }
            
            FPlatformApplicationMisc::PumpMessages(true);
        }, NSDefaultRunLoopMode, true);
    }
}

void FMacWindow::SetWindowFocus()
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

bool FMacWindow::IsValid() const
{
   return CocoaWindow != nullptr;
}

bool FMacWindow::IsActiveWindow() const
{
    __block bool bIsKeyWindow = false;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            bIsKeyWindow = CocoaWindow.isKeyWindow;
        }
        
    }, NSDefaultRunLoopMode, true);

    return bIsKeyWindow;
}

bool FMacWindow::IsMinimized() const
{
    __block bool bIsMinimized = false;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            bIsMinimized = CocoaWindow.miniaturized;
        }
        
    }, NSDefaultRunLoopMode, true);

    return bIsMinimized;
}

bool FMacWindow::IsMaximized() const
{
    __block bool bIsMaximized = false;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            bIsMaximized = CocoaWindow.zoomed;
        }
        
    }, NSDefaultRunLoopMode, true);

    return bIsMaximized;
}

bool FMacWindow::IsChildWindow(const TSharedRef<IPlatformWindow>& ChildWindow) const
{
    TSharedRef<FMacWindow> MacChildWindow = StaticCastSharedRef<FMacWindow>(ChildWindow);
    if (!MacChildWindow || !CocoaWindow)
    {
        return false;
    }

    __block bool bIsChildWindow = false;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        for (NSWindow* Ancestor = [MacChildWindow->GetCocoaWindow() parentWindow]; Ancestor; Ancestor = [Ancestor parentWindow])
        {
            if (Ancestor == CocoaWindow)
            {
                bIsChildWindow = true;
                break;
            }
        }
    }, NSDefaultRunLoopMode, true);

    return bIsChildWindow;
}

void FMacWindow::SetTitle(const String& InTitle)
{
    SCOPED_AUTORELEASE_POOL();

    __block NSString* Title = InTitle.GetNSString();
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        if (CocoaWindow)
        {
            [CocoaWindow setTitle:Title];
            [CocoaWindow setMiniwindowTitle:Title];
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::GetTitle(String& OutTitle) const
{
    SCOPED_AUTORELEASE_POOL();
    
    __block NSString* Title;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        if (CocoaWindow)
        {
            Title = CocoaWindow.title;
        }
    }, NSDefaultRunLoopMode, true);

    OutTitle = String(Title);
}

void FMacWindow::SetWindowPos(int32 x, int32 y)
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        if (CocoaWindow)
        {
            const NSRect ContentRect = [CocoaWindow contentRectForFrameRect:CocoaWindow.frame];
            NSRect NewContentRect = NSMakeRect(x, y, ContentRect.size.width, ContentRect.size.height);
            NewContentRect = FMacApplication::ConvertEngineRectToCocoa(NewContentRect.size.width, NewContentRect.size.height, NewContentRect.origin.x, NewContentRect.origin.y);
            
            const NSRect WindowFrame = [CocoaWindow frameRectForContentRect:NewContentRect];
            [CocoaWindow setFrameOrigin:WindowFrame.origin];
            
            Position = IntVector2(static_cast<int32>(WindowFrame.origin.x), static_cast<int32>(WindowFrame.origin.y));
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::SetWindowShape(const FWindowShape& Shape, bool bMove)
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
       
        if (CocoaWindow)
        {
            NSRect NewContentRect;
            if (bMove)
            {
                NewContentRect = NSMakeRect(Shape.Position.X, Shape.Position.Y, Shape.Width, Shape.Height);
            }
            else
            {
                NSRect ContentRect = [CocoaWindow contentRectForFrameRect:CocoaWindow.frame];
                ContentRect = FMacApplication::ConvertCocoaRectToEngine(ContentRect.size.width, ContentRect.size.height, ContentRect.origin.x, ContentRect.origin.y);
                NewContentRect = NSMakeRect(ContentRect.origin.x, ContentRect.origin.y, Shape.Width, Shape.Height);
            }
            
            NewContentRect = FMacApplication::ConvertEngineRectToCocoa(NewContentRect.size.width, NewContentRect.size.height, NewContentRect.origin.x, NewContentRect.origin.y);
            const NSRect NewFrame = [NSWindow frameRectForContentRect:NewContentRect styleMask:[CocoaWindow styleMask]];
            [CocoaWindow setFrame: NewFrame display: YES];
            
            Position = IntVector2(static_cast<int32>(NewFrame.origin.x), static_cast<int32>(NewFrame.origin.y));
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::GetWindowShape(FWindowShape& OutWindowShape) const
{
    __block NSRect ContentRect = NSMakeRect(0, 0, 0, 0);
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            ContentRect = [CocoaWindow contentRectForFrameRect:CocoaWindow.frame];
            ContentRect = FMacApplication::ConvertCocoaRectToEngine(ContentRect.size.width, ContentRect.size.height, ContentRect.origin.x, ContentRect.origin.y);
        }
    }, NSDefaultRunLoopMode, true);

    OutWindowShape.Width      = ContentRect.size.width;
    OutWindowShape.Height     = ContentRect.size.height;
    OutWindowShape.Position.X = ContentRect.origin.x;
    OutWindowShape.Position.Y = ContentRect.origin.y;
}

uint32 FMacWindow::GetWidth() const
{
    __block NSSize Size = NSMakeSize(0, 0);
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        if (CocoaWindow)
        {
            NSRect ContentRect = [CocoaWindow contentRectForFrameRect:CocoaWindow.frame];
            ContentRect = FMacApplication::ConvertCocoaRectToEngine(ContentRect.size.width, ContentRect.size.height, ContentRect.origin.x, ContentRect.origin.y);
            Size = ContentRect.size;
        }
    }, NSDefaultRunLoopMode, true);

    return uint32(Size.width);
}

uint32 FMacWindow::GetHeight() const
{
    __block NSSize Size = NSMakeSize(0, 0);
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        if (CocoaWindow)
        {
            NSRect ContentRect = [CocoaWindow contentRectForFrameRect:CocoaWindow.frame];
            ContentRect = FMacApplication::ConvertCocoaRectToEngine(ContentRect.size.width, ContentRect.size.height, ContentRect.origin.x, ContentRect.origin.y);
            Size = ContentRect.size;
        }
    }, NSDefaultRunLoopMode, true);

    return uint32(Size.height);
}

void FMacWindow::GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const
{
    __block NSRect Frame = NSMakeRect(0, 0, 0, 0);
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        if (CocoaWindow)
        {
            NSScreen* Screen = CocoaWindow ? CocoaWindow.screen : [NSScreen mainScreen];
            Frame = Screen.frame;
        }
    }, NSDefaultRunLoopMode, true);

    OutWidth  = Frame.size.width;
    OutHeight = Frame.size.height;
}

float FMacWindow::GetWindowDPIScale() const
{
    __block CGFloat Scale = 1.0f;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        if (CocoaWindow)
        {
            Scale = CocoaWindow.backingScaleFactor;
        }
    }, NSDefaultRunLoopMode, true);

    return static_cast<float>(Scale);
}

void FMacWindow::SetStyle(EWindowStyleFlags InStyle)
{
    const NSWindowStyleMask WindowStyle = GetCocoaWindowStyle(InStyle);

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        if (CocoaWindow)
        {
            const NSWindowLevel WindowLevel = (InStyle & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None ? NSFloatingWindowLevel : NSNormalWindowLevel;
            [CocoaWindow setLevel:WindowLevel];

            const bool bCustomTitleBar = (InStyle & EWindowStyleFlags::CustomTitleBar) != EWindowStyleFlags::None;
            [CocoaWindow setTitlebarAppearsTransparent:bCustomTitleBar];
            [CocoaWindow setTitleVisibility:bCustomTitleBar ? NSWindowTitleHidden : NSWindowTitleVisible];
            
            if ((InStyle & EWindowStyleFlags::Closable) != EWindowStyleFlags::None)
            {
                [[CocoaWindow standardWindowButton:NSWindowCloseButton] setEnabled:YES];
            }
            else
            {
                [[CocoaWindow standardWindowButton:NSWindowCloseButton] setEnabled:NO];
            }
            
            if ((InStyle & EWindowStyleFlags::Minimizable) != EWindowStyleFlags::None)
            {
                [[CocoaWindow standardWindowButton:NSWindowMiniaturizeButton] setEnabled:YES];
            }
            else
            {
                [[CocoaWindow standardWindowButton:NSWindowMiniaturizeButton] setEnabled:NO];
            }
            
            if ((InStyle & EWindowStyleFlags::Maximizable) != EWindowStyleFlags::None)
            {
                [[CocoaWindow standardWindowButton:NSWindowZoomButton] setEnabled:YES];
            }
            else
            {
                [[CocoaWindow standardWindowButton:NSWindowZoomButton] setEnabled:NO];
            }
            
            NSWindowCollectionBehavior Behavior = NSWindowCollectionBehaviorDefault | NSWindowCollectionBehaviorManaged | NSWindowCollectionBehaviorParticipatesInCycle;
            if ((InStyle & EWindowStyleFlags::Resizable) != EWindowStyleFlags::None)
            {
                Behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
            }
            else
            {
                Behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
            }
            
            if ((InStyle & EWindowStyleFlags::Opaque) == EWindowStyleFlags::None)
            {
                [CocoaWindow setOpaque:NO];
                [CocoaWindow setHasShadow:NO];
            }
            else
            {
                [CocoaWindow setHasShadow: YES];
            }
            
            [CocoaWindow setStyleMask:WindowStyle];
            [CocoaWindow setCollectionBehavior:Behavior];
            
            const bool bWasInWindowsMenu = (StyleParams & EWindowStyleFlags::NoTaskBarIcon) == EWindowStyleFlags::None;
            const bool bIsInWindowsMenu  = (InStyle & EWindowStyleFlags::NoTaskBarIcon) == EWindowStyleFlags::None;

            if (bWasInWindowsMenu != bIsInWindowsMenu)
            {
                if (bIsInWindowsMenu)
                {
                    [NSApp addWindowsItem:CocoaWindow title:[CocoaWindow title] filename:NO];
                }
                else
                {
                    [NSApp removeWindowsItem:CocoaWindow];
                }
            }
            
            StyleParams = InStyle;
        }

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

FWindowTitleBarMetrics FMacWindow::GetTitleBarMetrics() const
{
    if ((StyleParams & EWindowStyleFlags::CustomTitleBar) == EWindowStyleFlags::None)
    {
        return FWindowTitleBarMetrics();
    }

    __block FWindowTitleBarMetrics Metrics;
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        if (!CocoaWindow)
        {
            return;
        }

        if ((CocoaWindow.styleMask & NSWindowStyleMaskFullScreen) != 0)
        {
            return;
        }

        NSButton* CloseButton = [CocoaWindow standardWindowButton:NSWindowCloseButton];
        NSButton* ZoomButton  = [CocoaWindow standardWindowButton:NSWindowZoomButton];

        if (!CloseButton || !ZoomButton || CloseButton.isHidden)
        {
            return;
        }

        const NSRect ContentLayoutRect = [CocoaWindow contentLayoutRect];
        Metrics.Height       = static_cast<float>(CocoaWindow.frame.size.height - ContentLayoutRect.size.height);
        Metrics.LeadingInset = static_cast<float>(NSMaxX(ZoomButton.frame) + NSMinX(CloseButton.frame));

    }, NSDefaultRunLoopMode, true);

    return Metrics;
}

void FMacWindow::SetTitleBarRegions(const FWindowTitleBarRegions& InRegions)
{
    SCOPED_LOCK(TitleBarRegionsCS);
    TitleBarRegions = InRegions;
}

bool FMacWindow::HitTestTitleBar(NSPoint LocationInWindow) const
{
    if (!CocoaWindow || (StyleParams & EWindowStyleFlags::CustomTitleBar) == EWindowStyleFlags::None)
    {
        return false;
    }

    const NSWindowButton StandardButtons[] = { NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton };
    for (NSWindowButton ButtonKind : StandardButtons)
    {
        NSButton* Button = [CocoaWindow standardWindowButton:ButtonKind];
        if (Button && !Button.isHidden && NSPointInRect(LocationInWindow, [Button convertRect:Button.bounds toView:nil]))
        {
            return false;
        }
    }

    const NSRect     ContentRect = [CocoaWindow contentRectForFrameRect:CocoaWindow.frame];
    const IntVector2 ClientPoint = IntVector2(static_cast<int32>(LocationInWindow.x), static_cast<int32>(ContentRect.size.height - LocationInWindow.y));

    SCOPED_LOCK(TitleBarRegionsCS);

    if (!TitleBarRegions.CaptionRect.Contains(ClientPoint))
    {
        return false;
    }

    for (const FWindowRect& InteractiveRect : TitleBarRegions.InteractiveRects)
    {
        if (InteractiveRect.Contains(ClientPoint))
        {
            return false;
        }
    }

    return true;
}

void FMacWindow::SetWindowOpacity(float Alpha)
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        if (CocoaWindow)
        {
            CocoaWindow.alphaValue = Alpha;
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::SetAcceptsInput(bool bInAcceptsInput)
{
    if (bAcceptsInput == bInAcceptsInput)
    {
        return;
    }

    bAcceptsInput = bInAcceptsInput;

    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();

        if (CocoaWindow)
        {
            [CocoaWindow setIgnoresMouseEvents:!bInAcceptsInput];
        }

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::SetPlatformHandle(void* InPlatformHandle)
{
    if (InPlatformHandle)
    {
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            if (FCocoaWindow* NewWindow = NSClassCast<FCocoaWindow>(reinterpret_cast<NSObject*>(InPlatformHandle)))
            {
                if (FCocoaWindowView* NewWindowView = NSClassCast<FCocoaWindowView>(NewWindow.contentView))
                {
                    CocoaWindow     = NewWindow;
                    CocoaWindowView = NewWindowView;
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
