#include "MacWindow.h"
#include "CocoaWindow.h"
#include "Core/Mac/Mac.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Mac/MacRunLoop.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

TSharedRef<FMacWindow> FMacWindow::Create(FMacApplication* InApplication)
{
    TSharedRef<FMacWindow> NewWindow = new FMacWindow(InApplication);
    return NewWindow;
}

FMacWindow::FMacWindow(FMacApplication* InApplication)
    : FGenericWindow()
    , Application(InApplication)
    , CocoaWindow(nullptr)
    , CocoaWindowView(nullptr)
{
}

FMacWindow::~FMacWindow()
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        [CocoaWindowView release];
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

        const NSRect WindowRect = FMacApplication::ConvertEngineRectToCocoa(Width, Height, PositionX, PositionY);
        CocoaWindow = [[FCocoaWindow alloc] initWithContentRect:WindowRect styleMask:WindowStyle backing:NSBackingStoreBuffered defer:NO];
        if (!CocoaWindow)
        {
            LOG_ERROR("[FMacWindow]: Failed to create NSWindow");
            return;
        }

        const NSWindowLevel WindowLevel = (InInitializer.Style & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None ? NSFloatingWindowLevel : NSNormalWindowLevel;
        [CocoaWindow setLevel:WindowLevel];

        if ((InInitializer.Style & EWindowStyleFlags::Titled) != EWindowStyleFlags::None)
        {
            CocoaWindow.title = InInitializer.Title.GetNSString();
        }

        if ((InInitializer.Style & EWindowStyleFlags::Closable) != EWindowStyleFlags::None)
        {
            [[CocoaWindow standardWindowButton:NSWindowCloseButton] setEnabled:YES];
        }
        else
        {
            [[CocoaWindow standardWindowButton:NSWindowCloseButton] setEnabled:NO];
        }
        
        if ((InInitializer.Style & EWindowStyleFlags::Minimizable) != EWindowStyleFlags::None)
        {
            [[CocoaWindow standardWindowButton:NSWindowMiniaturizeButton] setEnabled:YES];
        }
        else
        {
            [[CocoaWindow standardWindowButton:NSWindowMiniaturizeButton] setEnabled:NO];
        }
        
        if ((InInitializer.Style & EWindowStyleFlags::Maximizable) != EWindowStyleFlags::None)
        {
            [[CocoaWindow standardWindowButton:NSWindowZoomButton] setEnabled:YES];
        }
        else
        {
            [[CocoaWindow standardWindowButton:NSWindowZoomButton] setEnabled:NO];
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

        CocoaWindow.collectionBehavior = Behavior;

        if ((InInitializer.Style & EWindowStyleFlags::Opaque) == EWindowStyleFlags::None)
        {
            [CocoaWindow setOpaque:NO];
            [CocoaWindow setHasShadow:NO];
        }
        else
        {
            [CocoaWindow setHasShadow: YES];
        }
        
        // Create Window-View
        NSColor* BackGroundColor = [NSColor colorWithSRGBRed:0.15f green:0.15f blue:0.15f alpha:1.0f];
        CocoaWindowView = [[FCocoaWindowView alloc] initWithFrame:WindowRect];
        
        [CocoaWindow setReleasedWhenClosed:NO];
        [CocoaWindow setAcceptsMouseMovedEvents:YES];
        [CocoaWindow setRestorable:NO];
        [CocoaWindow setDelegate:CocoaWindow];
        [CocoaWindow setBackgroundColor:BackGroundColor];
        [CocoaWindow setContentView:CocoaWindowView];
        [CocoaWindow makeFirstResponder:CocoaWindowView];

        [NSApp addWindowsItem:CocoaWindow title:InInitializer.Title.GetNSString() filename:NO];

        if ([CocoaWindow respondsToSelector:@selector(setTabbingMode:)])
        {
            [CocoaWindow setTabbingMode:NSWindowTabbingModeDisallowed];
        }
        
        // Store the cached position and initialization params
        Position    = FIntVector2(static_cast<int32>(WindowRect.origin.x), static_cast<int32>(WindowRect.origin.y));
        StyleParams = InInitializer.Style;
        bResult     = true;
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);

    return bResult;
}

void FMacWindow::Show(bool bFocus)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            [CocoaWindow setIsVisible:YES];

            if (bFocus)
            {
                [CocoaWindow orderFront:nil];
            }
            else
            {
                [CocoaWindow makeKeyAndOrderFront:nil];
            }
        }

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Minimize()
{
    ExecuteOnMainThread(^
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
    ExecuteOnMainThread(^
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

void FMacWindow::Destroy()
{
    if (CocoaWindow)
    {
        SCOPED_AUTORELEASE_POOL();
        
        TSharedRef<FMacWindow> ThisWindow = MakeSharedRef<FMacWindow>(this);
        Application->OnWindowDestroyed(ThisWindow);
        CocoaWindow = nullptr;
    }
}

void FMacWindow::Restore()
{
    ExecuteOnMainThread(^
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
    if ((StyleParams & EWindowStyleFlags::Resizeable) != EWindowStyleFlags::None)
    {
        ExecuteOnMainThread(^
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

bool FMacWindow::IsActiveWindow() const
{
    __block bool bIsKeyWindow = false;
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            bIsKeyWindow = CocoaWindow.isKeyWindow;
        }
        
    }, NSDefaultRunLoopMode, true);

    return bIsKeyWindow;
}

bool FMacWindow::IsValid() const
{
   return CocoaWindow != nullptr;
}

bool FMacWindow::IsMinimized() const
{
    __block bool bIsMinimized = false;
    ExecuteOnMainThread(^
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
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        if (CocoaWindow)
        {
            bIsMaximized = CocoaWindow.zoomed;
        }
        
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

        if (CocoaWindow)
        {
            for (NSWindow* ChildWindow in CocoaWindow.childWindows)
            {
                FCocoaWindow* CocoaWindow = NSClassCast<FCocoaWindow>(ChildWindow);
                if (CocoaWindow && CocoaWindow == MacChildWindow->GetCocoaWindow())
                {
                    bIsChildWindow = true;
                    break;
                }
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
        
        if (CocoaWindow)
        {
            [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::SetTitle(const FString& InTitle)
{
    SCOPED_AUTORELEASE_POOL();

    __block NSString* Title = InTitle.GetNSString();
    ExecuteOnMainThread(^
    {
        if (CocoaWindow)
        {
            [CocoaWindow setTitle:Title];
            [CocoaWindow setMiniwindowTitle:Title];
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::GetTitle(FString& OutTitle) const
{
    SCOPED_AUTORELEASE_POOL();
    
    __block NSString* Title;
    ExecuteOnMainThread(^
    {
        if (CocoaWindow)
        {
            Title = CocoaWindow.title;
        }
    }, NSDefaultRunLoopMode, true);

    OutTitle = FString(Title);
}

void FMacWindow::SetWindowPos(int32 x, int32 y)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        if (CocoaWindow)
        {
            const NSRect ContentRect = [CocoaWindow contentRectForFrameRect:CocoaWindow.frame];
            NSRect NewContentRect = NSMakeRect(x, y, ContentRect.size.width, ContentRect.size.height);
            NewContentRect = FMacApplication::ConvertEngineRectToCocoa(NewContentRect.size.width, NewContentRect.size.height, NewContentRect.origin.x, NewContentRect.origin.y);
            
            const NSRect WindowFrame = [CocoaWindow frameRectForContentRect:NewContentRect];
            [CocoaWindow setFrameOrigin:WindowFrame.origin];
            
            // Cache the position
            Position = FIntVector2(static_cast<int32>(WindowFrame.origin.x), static_cast<int32>(WindowFrame.origin.y));
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::SetWindowOpacity(float Alpha)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();

        if (CocoaWindow)
        {
            CocoaWindow.alphaValue = Alpha;
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::SetWindowShape(const FWindowShape& Shape, bool bMove)
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
       
        if (CocoaWindow)
        {
            NSRect NewContentRect;
            if (bMove)
            {
                NewContentRect = NSMakeRect(Shape.Position.x, Shape.Position.y, Shape.Width, Shape.Height);
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
            
            // Cache the position
            Position = FIntVector2(static_cast<int32>(NewFrame.origin.x), static_cast<int32>(NewFrame.origin.y));
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::GetWindowShape(FWindowShape& OutWindowShape) const
{
    __block NSRect ContentRect = NSMakeRect(0, 0, 0, 0);
    ExecuteOnMainThread(^
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
    OutWindowShape.Position.x = ContentRect.origin.x;
    OutWindowShape.Position.y = ContentRect.origin.y;
}

uint32 FMacWindow::GetWidth() const
{
    __block NSSize Size = NSMakeSize(0, 0);
    ExecuteOnMainThread(^
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
    ExecuteOnMainThread(^
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
    ExecuteOnMainThread(^
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
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        if (CocoaWindow)
        {
            Scale = CocoaWindow.backingScaleFactor;
        }
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

        if (CocoaWindow)
        {
            const NSWindowLevel WindowLevel = (InStyle & EWindowStyleFlags::TopMost) != EWindowStyleFlags::None ? NSFloatingWindowLevel : NSNormalWindowLevel;
            [CocoaWindow setLevel:WindowLevel];
            
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
            if ((InStyle & EWindowStyleFlags::Resizeable) != EWindowStyleFlags::None)
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
            
            // Set styleflags
            StyleParams = InStyle;
        }

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}
