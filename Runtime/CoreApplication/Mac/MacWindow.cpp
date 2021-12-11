#if PLATFORM_MACOS
#include "MacWindow.h"
#include "ScopedAutoreleasePool.h"
#include "CocoaWindow.h"
#include "CocoaContentView.h"

#include "Core/Logging/Log.h"
#include "Core/Threading/Mac/MacRunLoop.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

TSharedRef<CMacWindow> CMacWindow::Make( CMacApplication* InApplication )
{
	return dbg_new CMacWindow( InApplication );
}

CMacWindow::CMacWindow( CMacApplication* InApplication )
    : CPlatformWindow()
    , Application( InApplication )
    , Window(nullptr)
    , View(nullptr)
{
}

CMacWindow::~CMacWindow()
{
	MakeMainThreadCall(^
	{
		SCOPED_AUTORELEASE_POOL();
			
		[Window release];
		[View release];
	}, true);
}

bool CMacWindow::Initialize( const CString& InTitle, uint32 InWidth, uint32 InHeight, int32 x, int32 y, SWindowStyle InStyle )
{
    __block bool bResult = false;
    MakeMainThreadCall(^
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
        
        const NSRect WindowRect = NSMakeRect( CGFloat(x), CGFloat(y), CGFloat(InWidth), CGFloat(InHeight) );
        Window = [[CCocoaWindow alloc] init:Application ContentRect:WindowRect StyleMask:WindowStyle Backing:NSBackingStoreBuffered Defer:NO];
        if (!Window)
        {
            LOG_ERROR("[CMacWindow]: Failed to create NSWindow");
            return;
        }
        
        View = [[CCocoaContentView alloc] init:Application];
        if (!View)
        {
            LOG_ERROR("[CMacWindow]: Failed to create CocoaContentView");
            return;
        }
        
        if (InStyle.IsTitled())
        {
            NSString* Title = [NSString stringWithUTF8String:InTitle.CStr()];
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
        StyleParams = InStyle;

        bResult = true;
    }, true);

    return bResult;
}

void CMacWindow::Show( bool bMaximized )
{
	MakeMainThreadCall(^
	{
		[Window makeKeyAndOrderFront:Window];

		if ( bMaximized )
		{
			[Window zoom:Window];
		}

		PlatformApplicationMisc::PumpMessages( true );
	}, true);
}

void CMacWindow::Close()
{
	if (StyleParams.IsClosable())
	{
		MakeMainThreadCall(^
		{
			[Window performClose:Window];
			PlatformApplicationMisc::PumpMessages( true );
		}, true);
	}
}

void CMacWindow::Minimize()
{
	if (StyleParams.IsMinimizable())
	{
		MakeMainThreadCall(^
		{
			[Window miniaturize:Window];
			PlatformApplicationMisc::PumpMessages( true );
		}, true);
	}
}

void CMacWindow::Maximize()
{
	if (StyleParams.IsMaximizable())
	{
		MakeMainThreadCall(^
		{
			if ([Window isMiniaturized])
			{
				[Window deminiaturize:Window];
			}

			[Window zoom:Window];

			PlatformApplicationMisc::PumpMessages( true );
		}, true);
	}
}

bool CMacWindow::IsActiveWindow() const
{
   NSWindow* KeyWindow = [NSApp keyWindow];
   return KeyWindow == Window;
}

void CMacWindow::Restore()
{
    MakeMainThreadCall(^
    {
        if ([Window isMiniaturized])
        {
            [Window deminiaturize:Window];
        }
       
        if ([Window isZoomed])
        {
            [Window zoom:Window];
        }
    
        PlatformApplicationMisc::PumpMessages( true );
    }, true);
}

void CMacWindow::ToggleFullscreen()
{
	if (StyleParams.IsResizeable())
	{
		MakeMainThreadCall(^
		{
			[Window toggleFullScreen:Window];
		}, true);
	}
}

void CMacWindow::SetTitle( const CString& InTitle )
{
	SCOPED_AUTORELEASE_POOL();

	if (StyleParams.IsTitled())
	{
		NSString* Title = [NSString stringWithUTF8String:InTitle.CStr()];

		MakeMainThreadCall(^
		{
			[Window setTitle:Title];
		}, true);
	}
}

void CMacWindow::GetTitle( CString& OutTitle )
{
    if (StyleParams.IsTitled())
    {
        NSString* Title  = [Window title];
        NSInteger Length = [Title length];
        OutTitle.Resize( static_cast<int32>(Length) );
        
        const char* UTF8Title = [Title UTF8String];
		CMemory::Memcpy(OutTitle.Data(), UTF8Title, sizeof(char) * Length);
    }
}

void CMacWindow::SetWindowShape( const SWindowShape& Shape, bool bMove )
{
	SCOPED_AUTORELEASE_POOL();
	
	MakeMainThreadCall(^
	{
		NSRect Frame = [Window frame];
		if ( StyleParams.IsResizeable() )
		{
			Frame.size.width  = Shape.Width;
			Frame.size.height = Shape.Height;
			[Window setFrame: Frame display: YES animate: YES];
		}
		
		if ( bMove )
		{
			// TODO: Make sure this is correct
			[Window setFrameOrigin:NSMakePoint(Shape.Position.x, Shape.Position.y - Frame.size.height + 1)];
		}
		
		PlatformApplicationMisc::PumpMessages( true );
	}, true);
}

void CMacWindow::GetWindowShape( SWindowShape& OutWindowShape ) const
{
	SCOPED_AUTORELEASE_POOL();

	__block NSRect Frame;
	__block NSRect ContentRect;
	MakeMainThreadCall(^
	{
		Frame       = [Window frame];
		ContentRect = [Window contentRectForFrameRect:[Window frame]];
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
		ContentRect = [Window contentRectForFrameRect:[Window frame]];
	}, true);

	return uint32(ContentRect.size.width);
}

uint32 CMacWindow::GetHeight() const
{
	SCOPED_AUTORELEASE_POOL();

	__block NSRect ContentRect;
	MakeMainThreadCall(^
	{
		ContentRect = [Window contentRectForFrameRect:[Window frame]];
	}, true);

	return uint32(ContentRect.size.height);
}

#endif
