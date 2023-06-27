#include "MacWindow.h"
#include "CocoaWindow.h"
#include "Core/Mac/Mac.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Mac/MacRunLoop.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

FMacWindow::FMacWindow(FMacApplication* InApplication)
    : FGenericWindow()
    , Application(InApplication)
    , WindowHandle(nullptr)
{
}

FMacWindow::~FMacWindow()
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        NSSafeRelease(WindowHandle);
    }, NSDefaultRunLoopMode, true);
}

bool FMacWindow::Initialize(const FGenericWindowInitializer& InInitializer)
{
    NSUInteger WindowStyle = 0;
    if (InInitializer.Style != EWindowStyleFlag::None)
    {
        WindowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskFullSizeContentView;
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
        
        const NSRect WindowRect = NSMakeRect(CGFloat(InInitializer.Position.x), CGFloat(InInitializer.Position.y), CGFloat(InInitializer.Width), CGFloat(InInitializer.Height));
        WindowHandle = [[FCocoaWindow alloc] initWithContentRect: WindowRect styleMask: WindowStyle backing: NSBackingStoreBuffered defer: NO];
        if (!WindowHandle)
        {
            LOG_ERROR("[FMacWindow]: Failed to create NSWindow");
            return;
        }
        
        const int32 WindowLevel = NSNormalWindowLevel;
        WindowHandle.level = WindowLevel;
        
        if (InInitializer.Style.IsTitled())
        {
            WindowHandle.title = InInitializer.Title.GetNSString();
        }
        
        // Set a default background
        NSColor* BackGroundColor = [NSColor colorWithSRGBRed:0.15f green:0.15f blue:0.15f alpha:1.0f];
        
        // Setting this to no disables any notifications about the window closing. Not documented.
        [WindowHandle setReleasedWhenClosed:NO];
        [WindowHandle setAcceptsMouseMovedEvents:YES];
        [WindowHandle setRestorable:NO];
        [WindowHandle setHasShadow: YES];
        [WindowHandle setDelegate:WindowHandle];
        [WindowHandle setBackgroundColor:BackGroundColor];
        
        if (!InInitializer.Style.IsMinimizable())
        {
            [[WindowHandle standardWindowButton:NSWindowMiniaturizeButton] setEnabled:NO];
        }
        if (!InInitializer.Style.IsMaximizable())
        {
            [[WindowHandle standardWindowButton:NSWindowZoomButton] setEnabled:NO];
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
        
        WindowHandle.collectionBehavior = Behavior;
        
        [NSApp addWindowsItem:WindowHandle title:InInitializer.Title.GetNSString() filename:NO];
        
        // Set styleflags
        StyleParams = InInitializer.Style;

        bResult = true;
    }, NSDefaultRunLoopMode, true);

    return bResult;
}

void FMacWindow::Show(bool bMaximized)
{
    ExecuteOnMainThread(^
    {
        [WindowHandle makeKeyAndOrderFront:WindowHandle];

        if (bMaximized)
        {
            [WindowHandle zoom:WindowHandle];
        }

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Destroy()
{
    ExecuteOnMainThread(^
    {
        [WindowHandle performClose:WindowHandle];
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Minimize()
{
    ExecuteOnMainThread(^
    {
        [WindowHandle miniaturize:WindowHandle];
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::Maximize()
{
    ExecuteOnMainThread(^
    {
        if (WindowHandle.miniaturized)
        {
            [WindowHandle deminiaturize:WindowHandle];
        }

        [WindowHandle zoom:WindowHandle];

        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

bool FMacWindow::IsActiveWindow() const
{
   NSWindow* KeyWindow = NSApp.keyWindow;
   return (KeyWindow == WindowHandle);
}

void FMacWindow::Restore()
{
    ExecuteOnMainThread(^
    {
        if (WindowHandle.miniaturized)
        {
            [WindowHandle deminiaturize:WindowHandle];
        }
       
        if (WindowHandle.zoomed)
        {
            [WindowHandle zoom:WindowHandle];
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
            [WindowHandle toggleFullScreen:WindowHandle];
        }, NSDefaultRunLoopMode, true);
    }
}

void FMacWindow::SetTitle(const FString& InTitle)
{

    if (StyleParams.IsTitled())
    {
        SCOPED_AUTORELEASE_POOL();

        NSString* Title = InTitle.GetNSString();
        ExecuteOnMainThread(^
        {
            WindowHandle.title = Title;
        }, NSDefaultRunLoopMode, true);
    }
}

void FMacWindow::GetTitle(FString& OutTitle) const
{
    if (StyleParams.IsTitled())
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* Title = WindowHandle.title;
        OutTitle = FString(Title);
    }
}

void FMacWindow::SetWindowShape(const FWindowShape& Shape, bool bMove)
{
    SCOPED_AUTORELEASE_POOL();
    
    ExecuteOnMainThread(^
    {
        NSRect Frame = WindowHandle.frame;
        if (StyleParams.IsResizeable())
        {
            Frame.size.width  = Shape.Width;
            Frame.size.height = Shape.Height;
            [WindowHandle setFrame: Frame display: YES animate: YES];
        }
        
        if (bMove)
        {
            // TODO: Make sure this is correct
            [WindowHandle setFrameOrigin:NSMakePoint(Shape.Position.x, Shape.Position.y - Frame.size.height + 1)];
        }
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

void FMacWindow::GetWindowShape(FWindowShape& OutWindowShape) const
{
    SCOPED_AUTORELEASE_POOL();

    __block NSRect Frame;
    __block NSRect ContentRect;
    ExecuteOnMainThread(^
    {
        Frame       = WindowHandle.frame;
        ContentRect = [WindowHandle contentRectForFrameRect:WindowHandle.frame];
    }, NSDefaultRunLoopMode, true);

    OutWindowShape.Width      = ContentRect.size.width;
    OutWindowShape.Height     = ContentRect.size.height;
    OutWindowShape.Position.x = Frame.origin.x;
    OutWindowShape.Position.y = Frame.origin.y;
}

uint32 FMacWindow::GetWidth() const
{
    SCOPED_AUTORELEASE_POOL();

    __block NSRect ContentRect;
    ExecuteOnMainThread(^
    {
        ContentRect = [WindowHandle contentRectForFrameRect:WindowHandle.frame];
    }, NSDefaultRunLoopMode, true);

    return uint32(ContentRect.size.width);
}

uint32 FMacWindow::GetHeight() const
{
    SCOPED_AUTORELEASE_POOL();

    __block NSRect ContentRect;
    ExecuteOnMainThread(^
    {
        ContentRect = [WindowHandle contentRectForFrameRect:WindowHandle.frame];
    }, NSDefaultRunLoopMode, true);

    return uint32(ContentRect.size.height);
}

void FMacWindow::SetPlatformHandle(void* InPlatformHandle)
{
    if (InPlatformHandle)
    {
        NSObject* Object = reinterpret_cast<NSObject*>(InPlatformHandle);

        // Make sure that the handle sent in is of correct type
        FCocoaWindow* NewWindow = NSClassCast<FCocoaWindow>(Object);
        if (NewWindow)
        {
            WindowHandle = NewWindow;
        }
    }
}
