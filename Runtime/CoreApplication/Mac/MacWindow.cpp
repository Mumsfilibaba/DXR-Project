#include "MacWindow.h"
#include "CocoaWindow.h"

#include "Core/Mac/Mac.h"
#include "Core/Logging/Log.h"
#include "Core/Threading/Mac/MacRunLoop.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacWindow

CMacWindow* CMacWindow::CreateMacWindow(FMacApplication* InApplication)
{
	return dbg_new CMacWindow(InApplication);
}

CMacWindow::CMacWindow(FMacApplication* InApplication)
    : FGenericWindow()
    , Application(InApplication)
    , WindowHandle(nullptr)
{ }

CMacWindow::~CMacWindow()
{
	MakeMainThreadCall(^
	{
		SCOPED_AUTORELEASE_POOL();
		NSSafeRelease(WindowHandle);
	}, true);
}

bool CMacWindow::Initialize(const FString& InTitle, uint32 InWidth, uint32 InHeight, int32 x, int32 y, FWindowStyle InStyle)
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
        WindowHandle = [[CCocoaWindow alloc] initWithContentRect: WindowRect styleMask: WindowStyle backing: NSBackingStoreBuffered defer: NO];
        if (!WindowHandle)
        {
            LOG_ERROR("[CMacWindow]: Failed to create NSWindow");
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
    }, true);

    return bResult;
}

void CMacWindow::Show(bool bMaximized)
{
	MakeMainThreadCall(^
	{
		[WindowHandle makeKeyAndOrderFront:WindowHandle];

		if (bMaximized)
		{
			[WindowHandle zoom:WindowHandle];
		}

		FPlatformApplicationMisc::PumpMessages(true);
	}, true);
}

void CMacWindow::Close()
{
	if (StyleParams.IsClosable())
	{
		MakeMainThreadCall(^
		{
			[WindowHandle performClose:WindowHandle];
			FPlatformApplicationMisc::PumpMessages(true);
		}, true);
	}
}

void CMacWindow::Minimize()
{
	if (StyleParams.IsMinimizable())
	{
		MakeMainThreadCall(^
		{
			[WindowHandle miniaturize:WindowHandle];
			FPlatformApplicationMisc::PumpMessages(true);
		}, true);
	}
}

void CMacWindow::Maximize()
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
		}, true);
	}
}

bool CMacWindow::IsActiveWindow() const
{
   NSWindow* KeyWindow = NSApp.keyWindow;
   return (KeyWindow == WindowHandle);
}

void CMacWindow::Restore()
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
    }, true);
}

void CMacWindow::ToggleFullscreen()
{
	if (StyleParams.IsResizeable())
	{
		MakeMainThreadCall(^
		{
			[WindowHandle toggleFullScreen:WindowHandle];
		}, true);
	}
}

void CMacWindow::SetTitle(const FString& InTitle)
{

    if (StyleParams.IsTitled())
    {
        SCOPED_AUTORELEASE_POOL();

        NSString* Title = InTitle.GetNSString();
		MakeMainThreadCall(^
		{
			WindowHandle.title = Title;
		}, true);
	}
}

void CMacWindow::GetTitle(FString& OutTitle)
{
    if (StyleParams.IsTitled())
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* Title = WindowHandle.title;
        OutTitle.Reset(Title.UTF8String, Title.length);
    }
}

void CMacWindow::SetWindowShape(const FWindowShape& Shape, bool bMove)
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
	}, true);
}

void CMacWindow::GetWindowShape(FWindowShape& OutWindowShape) const
{
	SCOPED_AUTORELEASE_POOL();

	__block NSRect Frame;
	__block NSRect ContentRect;
	MakeMainThreadCall(^
	{
		Frame       = WindowHandle.frame;
		ContentRect = [WindowHandle contentRectForFrameRect:WindowHandle.frame];
	}, true);

	OutWindowShape.Width      = ContentRect.size.width;
	OutWindowShape.Height     = ContentRect.size.height;
	OutWindowShape.Position.x = Frame.origin.x;
	OutWindowShape.Position.y = Frame.origin.y;
}

uint32 CMacWindow::GetWidth() const
{
	SCOPED_AUTORELEASE_POOL();

	__block NSRect ContentRect;
	MakeMainThreadCall(^
	{
		ContentRect = [WindowHandle contentRectForFrameRect:WindowHandle.frame];
	}, true);

	return uint32(ContentRect.size.width);
}

uint32 CMacWindow::GetHeight() const
{
	SCOPED_AUTORELEASE_POOL();

	__block NSRect ContentRect;
	MakeMainThreadCall(^
	{
		ContentRect = [WindowHandle contentRectForFrameRect:WindowHandle.frame];
	}, true);

	return uint32(ContentRect.size.height);
}

void CMacWindow::SetPlatformHandle(void* InPlatformHandle)
{
	if (InPlatformHandle)
	{
		NSObject* Object = reinterpret_cast<NSObject*>(InPlatformHandle);

		// Make sure that the handle sent in is of correct type
		CCocoaWindow* NewWindow = NSClassCast<CCocoaWindow>(Object);
		if (NewWindow)
		{
			WindowHandle = NewWindow;
		}
	}
}
