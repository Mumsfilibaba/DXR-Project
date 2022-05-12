#include "MacApplication.h"
#include "MacWindow.h"
#include "MacCursor.h"
#include "CocoaWindow.h"

#include "Core/Logging/Log.h"
#include "Core/Input/Platform/PlatformKeyMapping.h"
#include "Core/Threading/Mac/MacRunLoop.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"
#include "Core/Threading/ScopedLock.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacApplication

CMacApplication* MacApplication = nullptr;

CMacApplication* CMacApplication::CreateMacApplication()
{
    MacApplication = dbg_new CMacApplication();
	return MacApplication;
}

CMacApplication::CMacApplication()
    : CGenericApplication(TSharedPtr<ICursor>(CMacCursor::CreateMacCursor()))
    , Windows()
    , WindowsCS()
    , DeferredEvents()
    , DeferredEventsCS()
{
    SCOPED_AUTORELEASE_POOL();
    
    // This should only be init from the main thread, but assert just to be sure.
    Check(PlatformThreadMisc::IsMainThread());

    PlatformKeyMapping::Initialize();
    
    // Init the default macOS menu
    NSMenu* MenuBar = [[NSMenu alloc] init];
    NSMenuItem* AppMenuItem = [MenuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];
    
    NSMenu* AppMenu = [[NSMenu alloc] init];
    AppMenuItem.submenu = AppMenu;
    
    [AppMenu addItemWithTitle:@"DXR-Engine" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
    [AppMenu addItem: [NSMenuItem separatorItem]];
    
    // Engine menu item
    NSMenu* ServiceMenu = [[NSMenu alloc] init];
    [AppMenu addItemWithTitle:@"Services" action:nil keyEquivalent:@""].submenu = ServiceMenu;
    [AppMenu addItem:[NSMenuItem separatorItem]];
    [AppMenu addItemWithTitle:@"Hide DXR-Engine" action:@selector(hide:) keyEquivalent:@"h"];
    [AppMenu addItemWithTitle:@"Hide Other" action:@selector(hideOtherApplications:) keyEquivalent:@""].keyEquivalentModifierMask = NSEventModifierFlagOption | NSEventModifierFlagCommand;
    [AppMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
    [AppMenu addItem:[NSMenuItem separatorItem]];
    [AppMenu addItemWithTitle:@"Quit DXR-Engine" action:@selector(terminate:) keyEquivalent:@"q"];
    
    // Window menu
    NSMenuItem* WindowMenuItem = [MenuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];
    
    NSMenu* WindowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    WindowMenuItem.submenu = WindowMenu;
    
    [WindowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [WindowMenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
    [WindowMenu addItem:[NSMenuItem separatorItem]];
    
    [WindowMenu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];
    [WindowMenu addItem:[NSMenuItem separatorItem]];
    
    [WindowMenu addItemWithTitle:@"Enter Full Screen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"].keyEquivalentModifierMask = NSEventModifierFlagControl | NSEventModifierFlagCommand;
    
    SEL SetAppleMenuSelector = NSSelectorFromString(@"setAppleMenu:");
    [NSApp performSelector:SetAppleMenuSelector withObject:AppMenu];
    
    NSApp.mainMenu     = MenuBar;
    NSApp.windowsMenu  = WindowMenu;
    NSApp.servicesMenu = ServiceMenu;
}

CMacApplication::~CMacApplication()
{
    @autoreleasepool
    {
        Windows.Clear();

        if (this == MacApplication)
        {
            MacApplication = nullptr;
        }
    }
}

TSharedRef<CGenericWindow> CMacApplication::CreateWindow()
{
    TSharedRef<CMacWindow> NewWindow = CMacWindow::CreateMacWindow(this);
    
    {
        TScopedLock Lock(WindowsCS);
        Windows.Emplace(NewWindow);
    }

    return NewWindow;
}

void CMacApplication::Tick(float)
{
    PlatformApplicationMisc::PumpMessages(true);
	
	TArray<SDeferredMacEvent> ProcessableEvents;
    if (!DeferredEvents.IsEmpty())
	{
		TScopedLock Lock(DeferredEventsCS);
        
        ProcessableEvents.Swap(DeferredEvents);
		DeferredEvents.Clear();
	}
	
	for (const SDeferredMacEvent& CurrentEvent : ProcessableEvents)
	{
		ProcessDeferredEvent(CurrentEvent);
	}
    
    if (!ClosedWindows.IsEmpty())
    {
        TScopedLock Lock(ClosedWindowsCS);
        
        for (const TSharedRef<CMacWindow>& Window : ClosedWindows)
        {
            MessageListener->HandleWindowClosed(Window);
        }
        
        ClosedWindows.Clear();
    }
}

void CMacApplication::SetActiveWindow(const TSharedRef<CGenericWindow>& Window)
{
    MakeMainThreadCall(^
    {
		CCocoaWindow* CocoaWindow = static_cast<CMacWindow*>(Window.Get())->GetWindowHandle();
        [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
    }, true);
}

TSharedRef<CGenericWindow> CMacApplication::GetActiveWindow() const
{
    @autoreleasepool
    {
        NSWindow* KeyWindow = NSApp.keyWindow;
        return GetWindowFromNSWindow(KeyWindow);
    }
}

TSharedRef<CGenericWindow> CMacApplication::GetWindowUnderCursor() const
{
    @autoreleasepool
    {
        const NSInteger WindowNumber = [NSWindow windowNumberAtPoint:[NSEvent mouseLocation] belowWindowWithWindowNumber:0];
        return GetWindowFromNSWindow([NSApp windowWithWindowNumber:WindowNumber]);
    }
}

TSharedRef<CMacWindow> CMacApplication::GetWindowFromNSWindow(NSWindow* Window) const
{
    if (Window && [Window isKindOfClass:[CCocoaWindow class]])
    {
        TScopedLock Lock(WindowsCS);

        CCocoaWindow* CocoaWindow = reinterpret_cast<CCocoaWindow*>(Window);
        for (const TSharedRef<CMacWindow>& MacWindow : Windows)
        {
			if (CocoaWindow == reinterpret_cast<CCocoaWindow*>(MacWindow->GetPlatformHandle()))
            {
                return MacWindow;
            }
        }
    }
    
    return nullptr;
}

void CMacApplication::CloseWindow(const TSharedRef<CMacWindow>& Window)
{
    {
        TScopedLock Lock(ClosedWindowsCS);
        ClosedWindows.Emplace(Window);
    }
    
    {
        TScopedLock Lock(WindowsCS);
        Windows.Remove(Window);
    }
}

void CMacApplication::DeferEvent(NSObject* EventOrNotificationObject)
{
	if (EventOrNotificationObject)
	{
		SDeferredMacEvent NewDeferredEvent;
		
		if ([EventOrNotificationObject isKindOfClass:[NSEvent class]])
		{
			NSEvent* Event = reinterpret_cast<NSEvent*>(EventOrNotificationObject);
			NewDeferredEvent.Event  = [Event retain];
			
			NSWindow* Window = Event.window;
			if ([Window isKindOfClass: [CCocoaWindow class]])
			{
				CCocoaWindow* EventWindow = reinterpret_cast<CCocoaWindow*>(Window);
				NewDeferredEvent.Window   = [EventWindow retain];
			}
		}
		else if ([EventOrNotificationObject isKindOfClass:[NSNotification class]])
		{
			NSNotification* Notification = reinterpret_cast<NSNotification*>(EventOrNotificationObject);
			NewDeferredEvent.NotificationName = [Notification.name retain];
			
			NSObject* NotificationObject = Notification.object;
			if ([NotificationObject isKindOfClass: [CCocoaWindow class]])
			{
				CCocoaWindow* EventWindow = reinterpret_cast<CCocoaWindow*>(NotificationObject);
				NewDeferredEvent.Window   = [EventWindow retain];
				
				const NSRect ContentRect  = EventWindow.contentView.frame;
				NewDeferredEvent.Size     = ContentRect.size;
				NewDeferredEvent.Position = ContentRect.origin;
			}
		}
		else if ([EventOrNotificationObject isKindOfClass:[NSString class]])
		{
			NSString* Characters = reinterpret_cast<NSString*>(EventOrNotificationObject);
			
			NSUInteger Count = Characters.length;
			for (NSUInteger Index = 0; Index < Count; Index++)
			{
				const unichar Codepoint = [Characters characterAtIndex:Index];
				if ((Codepoint & 0xff00) != 0xf700)
				{
                    NewDeferredEvent.Character = uint32(Codepoint);
				}
			}
		}
		
        {
            TScopedLock Lock(DeferredEventsCS);
            DeferredEvents.Emplace(NewDeferredEvent);
        }
	}
}

void CMacApplication::ProcessDeferredEvent(const SDeferredMacEvent& Event)
{
    TSharedRef<CMacWindow> Window = GetWindowFromNSWindow(Event.Window);
	
	if (Event.NotificationName)
	{
		NSNotificationName NotificationName = Event.NotificationName;
		
		if (NotificationName == NSWindowDidMoveNotification)
		{
			MessageListener->HandleWindowMoved(Window, int16(Event.Position.x), int16(Event.Position.y));
		}
		else if (NotificationName == NSWindowDidResizeNotification)
		{
			MessageListener->HandleWindowResized(Window, uint16(Event.Size.width), uint16(Event.Size.height));
		}
		else if (NotificationName == NSWindowDidMiniaturizeNotification)
		{
			MessageListener->HandleWindowResized(Window, uint16(Event.Size.width), uint16(Event.Size.height));
		}
		else if (NotificationName == NSWindowDidDeminiaturizeNotification)
		{
			MessageListener->HandleWindowResized(Window, uint16(Event.Size.width), uint16(Event.Size.height));
		}
        else if (NotificationName == NSWindowDidBecomeMainNotification)
        {
            MessageListener->HandleWindowFocusChanged(Window, true);
        }
        else if (NotificationName == NSWindowDidResignMainNotification)
        {
            MessageListener->HandleWindowFocusChanged(Window, false);
        }
	}
	else if (Event.Event)
	{
		NSEvent* CurrentEvent = Event.Event;
		
		NSEventType EventType = CurrentEvent.type;
		switch(EventType)
		{
			case NSEventTypeKeyUp:
			{
				MessageListener->HandleKeyReleased(PlatformKeyMapping::GetKeyCodeFromScanCode(CurrentEvent.keyCode), PlatformApplicationMisc::GetModifierKeyState());
				break;
			}
			   
			case NSEventTypeKeyDown:
			{
				MessageListener->HandleKeyPressed(PlatformKeyMapping::GetKeyCodeFromScanCode(CurrentEvent.keyCode), CurrentEvent.ARepeat, PlatformApplicationMisc::GetModifierKeyState());
				break;
			}

			case NSEventTypeLeftMouseUp:
			case NSEventTypeRightMouseUp:
			case NSEventTypeOtherMouseUp:
			{
				MessageListener->HandleMouseReleased(PlatformKeyMapping::GetButtonFromIndex(static_cast<int32>(CurrentEvent.buttonNumber)), PlatformApplicationMisc::GetModifierKeyState());
				break;
			}

			case NSEventTypeLeftMouseDown:
			case NSEventTypeRightMouseDown:
			case NSEventTypeOtherMouseDown:
			{
				MessageListener->HandleMousePressed(PlatformKeyMapping::GetButtonFromIndex(static_cast<int32>(CurrentEvent.buttonNumber)), PlatformApplicationMisc::GetModifierKeyState());
				break;
			}

			case NSEventTypeLeftMouseDragged:
			case NSEventTypeOtherMouseDragged:
			case NSEventTypeRightMouseDragged:
			case NSEventTypeMouseMoved:
			{
				NSWindow* EventWindow = CurrentEvent.window;
				if (EventWindow)
				{
					const NSPoint MousePosition	= CurrentEvent.locationInWindow;
					const NSRect  ContentRect 	= EventWindow.contentView.frame;
					
					const int32 x = int32(MousePosition.x);
					const int32 y = int32(ContentRect.size.height - MousePosition.y);
					
					MessageListener->HandleMouseMove(x, y);
					break;
				}
			}
			   
			case NSEventTypeScrollWheel:
			{
				CGFloat ScrollDeltaX = CurrentEvent.scrollingDeltaX;
				CGFloat ScrollDeltaY = CurrentEvent.scrollingDeltaY;
                
				if (CurrentEvent.hasPreciseScrollingDeltas)
				{
					ScrollDeltaX *= 0.1;
					ScrollDeltaY *= 0.1;
				}
					
				MessageListener->HandleMouseScrolled(int32(ScrollDeltaX), int32(ScrollDeltaY));
				break;
			}
				
			case NSEventTypeMouseEntered:
			{
				TSharedRef<CMacWindow> Window = GetWindowFromNSWindow(CurrentEvent.window);
				if (Window)
				{
					MessageListener->HandleWindowMouseEntered(Window);
				}

				break;
			}
				
			case NSEventTypeMouseExited:
			{
				TSharedRef<CMacWindow> Window = GetWindowFromNSWindow(CurrentEvent.window);
				if (Window)
				{
					MessageListener->HandleWindowMouseLeft(Window);
				}

				break;
			}
				
			default:
			{
				break;
			}
		}
	}
	else if (Event.Character != uint32(-1))
	{
		MessageListener->HandleKeyChar(Event.Character);
	}
}
