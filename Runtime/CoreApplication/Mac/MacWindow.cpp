#include "MacWindow.h"
#include "CocoaWindow.h"

#include "Core/Mac/Mac.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Mac/MacRunLoop.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacWindow

FMacWindow::FMacWindow(FMacApplication* InApplication)
    : FGenericWindow()
    , Application(InApplication)
    , WindowHandle(nullptr)
{ }

FMacWindow::~FMacWindow()
{
	MakeMainThreadCall(^
	{
		SCOPED_AUTORELEASE_POOL();
		NSSafeRelease(WindowHandle);
	}, NSDefaultRunLoopMode, true);
}

bool FMacWindow::Initialize(const FString& InTitle, uint32 InWidth, uint32 InHeight, int32 x, int32 y, FWindowStyle InStyle)
{
    NSUInteger WindowStyle = 0;
    if (InStyle.Style)
    {
        WindowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskFullSizeContentView;
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
    
    __block bool bResult = false;
    MakeMainThreadCall(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        const NSRect WindowRect = NSMakeRect(CGFloat(x), CGFloat(y), CGFloat(InWidth), CGFloat(InHeight));
        WindowHandle = [[FCocoaWindow alloc] initWithContentRect: WindowRect styleMask: WindowStyle backing: NSBackingStoreBuffered defer: NO];
        if (!WindowHandle)
        {
            LOG_ERROR("[FMacWindow]: Failed to create NSWindow");
            return;
        }
        
        const int32 WindowLevel = NSNormalWindowLevel;
        WindowHandle.level = WindowLevel;
        
        if (InStyle.IsTitled())
        {
            WindowHandle.title = InTitle.GetNSString();
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
        
        if (!InStyle.IsMinimizable())
        {
            [[WindowHandle standardWindowButton:NSWindowMiniaturizeButton] setEnabled:NO];
        }
        if (!InStyle.IsMaximizable())
        {
            [[WindowHandle standardWindowButton:NSWindowZoomButton] setEnabled:NO];
        }
        
        NSWindowCollectionBehavior Behavior = NSWindowCollectionBehaviorDefault | NSWindowCollectionBehaviorManaged | NSWindowCollectionBehaviorParticipatesInCycle;
        if (InStyle.IsResizeable())
        {
            Behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
        }
        else
        {
            Behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
        }
        
        WindowHandle.collectionBehavior = Behavior;
        
        [NSApp addWindowsItem:WindowHandle title:InTitle.GetNSString() filename:NO];
        
        // Set styleflags
        StyleParams = InStyle;

        bResult = true;
    }, NSDefaultRunLoopMode, true);

    return bResult;
}

void FMacWindow::Show(bool bMaximized)
{
	MakeMainThreadCall(^
	{
		[WindowHandle makeKeyAndOrderFront:WindowHandle];

		if (bMaximized)
		{
			[WindowHandle zoom:WindowHandle];
		}

		FPlatformApplicationMisc::PumpMessages(true);
	}, NSDefaultRunLoopMode, true);
}

void FMacWindow::Close()
{
	if (StyleParams.IsClosable())
	{
		MakeMainThreadCall(^
		{
			[WindowHandle performClose:WindowHandle];
			FPlatformApplicationMisc::PumpMessages(true);
		}, NSDefaultRunLoopMode, true);
	}
}

void FMacWindow::Minimize()
{
	if (StyleParams.IsMinimizable())
	{
		MakeMainThreadCall(^
		{
			[WindowHandle miniaturize:WindowHandle];
			FPlatformApplicationMisc::PumpMessages(true);
		}, NSDefaultRunLoopMode, true);
	}
}

void FMacWindow::Maximize()
{
	if (StyleParams.IsMaximizable())
	{
		MakeMainThreadCall(^
		{
			if (WindowHandle.miniaturized)
			{
				[WindowHandle deminiaturize:WindowHandle];
			}

			[WindowHandle zoom:WindowHandle];

			FPlatformApplicationMisc::PumpMessages(true);
		}, NSDefaultRunLoopMode, true);
	}
}

bool FMacWindow::IsActiveWindow() const
{
   NSWindow* KeyWindow = NSApp.keyWindow;
   return (KeyWindow == WindowHandle);
}

void FMacWindow::Restore()
{
    MakeMainThreadCall(^
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
		MakeMainThreadCall(^
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
		MakeMainThreadCall(^
		{
			WindowHandle.title = Title;
		}, NSDefaultRunLoopMode, true);
	}
}

void FMacWindow::GetTitle(FString& OutTitle)
{
    if (StyleParams.IsTitled())
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* Title = WindowHandle.title;
        OutTitle.Reset(Title.UTF8String, Title.length);
    }
}

void FMacWindow::SetWindowShape(const FWindowShape& Shape, bool bMove)
{
	SCOPED_AUTORELEASE_POOL();
	
	MakeMainThreadCall(^
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
	MakeMainThreadCall(^
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
	MakeMainThreadCall(^
	{
		ContentRect = [WindowHandle contentRectForFrameRect:WindowHandle.frame];
	}, NSDefaultRunLoopMode, true);

	return uint32(ContentRect.size.width);
}

uint32 FMacWindow::GetHeight() const
{
	SCOPED_AUTORELEASE_POOL();

	__block NSRect ContentRect;
	MakeMainThreadCall(^
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
