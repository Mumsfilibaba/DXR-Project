#include "MacApplication.h"
#include "MacWindow.h"
#include "MacCursor.h"
#include "ScopedAutoreleasePool.h"
#include "CocoaAppDelegate.h"
#include "CocoaWindow.h"

#include "Core/Logging/Log.h"
#include "Core/Input/Platform/PlatformKeyMapping.h"
#include "Core/Threading/Mac/MacRunLoop.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"
#include "Core/Threading/ScopedLock.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacApplication

TSharedPtr<CMacApplication> CMacApplication::Make()
{
	return TSharedPtr<CMacApplication>(dbg_new CMacApplication());
}

CMacApplication::CMacApplication()
    : CGenericApplication(CMacCursor::Make())
	, AppDelegate(nullptr)
    , Windows()
    , WindowsMutex()
    , DeferredEvents()
    , DeferredEventsMutex()
	, bIsTerminating(false)
{ }

CMacApplication::~CMacApplication()
{
    SCOPED_AUTORELEASE_POOL();
    [AppDelegate release];
}

TSharedRef<CGenericWindow> CMacApplication::MakeWindow()
{
    TSharedRef<CMacWindow> NewWindow = CMacWindow::Make(this);
    
    {
        TScopedLock Lock(WindowsMutex);
        Windows.Emplace(NewWindow);
    }

    return NewWindow;
}

bool CMacApplication::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    Assert(PlatformThreadMisc::IsMainThread()); 

    /* Init application singleton */
    [NSApplication sharedApplication];
    Assert(NSApp != nullptr);
    
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    AppDelegate = [[CCocoaAppDelegate alloc] init:this];
    [NSApp setDelegate:AppDelegate];
    
	PlatformKeyMapping::Initialize();

    if (!InitializeAppMenu())
    {
        LOG_ERROR("[CMacApplication]: Failed to initialize the application menu");
        return false;
    }

    [NSApp finishLaunching];
    
    return true;
}

bool CMacApplication::InitializeAppMenu()
{
    SCOPED_AUTORELEASE_POOL();

    // This should only be init from the main thread, but assert just to be sure.
    Assert(PlatformThreadMisc::IsMainThread());

    // Init the default macOS menu
    NSMenu*     MenuBar     = [[NSMenu alloc] init];
    NSMenuItem* AppMenuItem = [MenuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu*     AppMenu     = [[NSMenu alloc] init];
    [AppMenuItem setSubmenu:AppMenu];
    
    [AppMenu addItemWithTitle:@"DXR-Engine" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
    [AppMenu addItem: [NSMenuItem separatorItem]];
    
    // Engine menu item
    NSMenu* ServiceMenu = [[NSMenu alloc] init];
    [[AppMenu addItemWithTitle:@"Services" action:nil keyEquivalent:@""] setSubmenu:ServiceMenu];
    [AppMenu addItem:[NSMenuItem separatorItem]];
    [AppMenu addItemWithTitle:@"Hide DXR-Engine" action:@selector(hide:) keyEquivalent:@"h"];
    [[AppMenu addItemWithTitle:@"Hide Other" action:@selector(hideOtherApplications:) keyEquivalent:@""] setKeyEquivalentModifierMask:NSEventModifierFlagOption | NSEventModifierFlagCommand];
    [AppMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
    [AppMenu addItem:[NSMenuItem separatorItem]];
    [AppMenu addItemWithTitle:@"Quit DXR-Engine" action:@selector(terminate:) keyEquivalent:@"q"];
    
    // Window menu
    NSMenuItem* WindowMenuItem = [MenuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu*     WindowMenu     = [[NSMenu alloc] initWithTitle:@"Window"];
    [WindowMenuItem setSubmenu:WindowMenu];
    
    [WindowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [WindowMenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
    [WindowMenu addItem:[NSMenuItem separatorItem]];
    
    [WindowMenu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];
    [WindowMenu addItem:[NSMenuItem separatorItem]];
    
    [[WindowMenu addItemWithTitle:@"Enter Full Screen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"] setKeyEquivalentModifierMask:NSEventModifierFlagControl | NSEventModifierFlagCommand];
    
    SEL SetAppleMenuSelector = NSSelectorFromString(@"setAppleMenu:");
    [NSApp performSelector:SetAppleMenuSelector withObject:AppMenu];
    
    [NSApp setMainMenu:MenuBar];
    [NSApp setWindowsMenu:WindowMenu];
    [NSApp setServicesMenu:ServiceMenu];

    return true;
}

void CMacApplication::Tick(float)
{
    PlatformApplicationMisc::PumpMessages(true);
	
	TArray<SMacApplicationEvent> ProcessableEvents;
	{
		TScopedLock Lock(DeferredEventsMutex);
		ProcessableEvents.Swap(DeferredEvents);
		DeferredEvents.Empty();
	}
	
	for (const SMacApplicationEvent& CurrentEvent : ProcessableEvents)
	{
		HandleEvent(CurrentEvent);
	}
}

void CMacApplication::SetActiveWindow(const TSharedRef<CGenericWindow>& Window)
{
    MakeMainThreadCall(^
    {
		CCocoaWindow* CocoaWindow = reinterpret_cast<CCocoaWindow*>(Window->GetPlatformHandle());
        [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
    }, true);
}

TSharedRef<CGenericWindow> CMacApplication::GetActiveWindow() const
{
	SCOPED_AUTORELEASE_POOL();

    NSWindow* KeyWindow = [NSApp keyWindow];
    return GetWindowFromNSWindow(KeyWindow);
}

TSharedRef<CGenericWindow> CMacApplication::GetWindowUnderCursor() const
{
	SCOPED_AUTORELEASE_POOL();
	
	NSPoint   MousePosition = [NSEvent mouseLocation];
	NSInteger WindowNumber  = [NSWindow windowNumberAtPoint:MousePosition belowWindowWithWindowNumber:0];
	
	NSWindow* Window = [NSApp windowWithWindowNumber:WindowNumber];
	return GetWindowFromNSWindow(Window);
}

TSharedRef<CMacWindow> CMacApplication::GetWindowFromNSWindow(NSWindow* Window) const
{
    if (Window && [Window isKindOfClass:[CCocoaWindow class]])
    {
        TScopedLock Lock(WindowsMutex);

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

void CMacApplication::DeferEvent(NSObject* EventOrNotificationObject)
{
	if (EventOrNotificationObject)
	{
		SMacApplicationEvent NewDeferredEvent;
		
		if ([EventOrNotificationObject isKindOfClass:[NSEvent class]])
		{
			NSEvent* Event = reinterpret_cast<NSEvent*>(EventOrNotificationObject);
			NewDeferredEvent.Event  = [Event retain];
			
			NSWindow* Window = [Event window];
			if ([Window isKindOfClass: [CCocoaWindow class]])
			{
				CCocoaWindow* EventWindow = reinterpret_cast<CCocoaWindow*>(Window);
				NewDeferredEvent.Window   = [EventWindow retain];
			}
		}
		else if ([EventOrNotificationObject isKindOfClass:[NSNotification class]])
		{
			NSNotification* Notification = reinterpret_cast<NSNotification*>(EventOrNotificationObject);
			NewDeferredEvent.NotificationName = [[Notification name] retain];
			
			NSObject* NotificationObject = [Notification object];
			if ([NotificationObject isKindOfClass: [CCocoaWindow class]])
			{
				CCocoaWindow* EventWindow = reinterpret_cast<CCocoaWindow*>(NotificationObject);
				NewDeferredEvent.Window   = [EventWindow retain];
				
				const NSRect ContentRect  = [[EventWindow contentView] frame];
				NewDeferredEvent.Size     = ContentRect.size;
				NewDeferredEvent.Position = ContentRect.origin;
			}
		}
		else if ([EventOrNotificationObject isKindOfClass:[NSString class]])
		{
			NSString* Characters = reinterpret_cast<NSString*>(EventOrNotificationObject);
			
			NSUInteger Count = [Characters length];
			for (NSUInteger Index = 0; Index < Count; Index++)
			{
				const unichar Codepoint = [Characters characterAtIndex:Index];
				if ((Codepoint & 0xff00) != 0xf700)
				{
					MessageListener->HandleKeyTyped(uint32(Codepoint));
				}
			}
		}
		
		TScopedLock Lock(DeferredEventsMutex);
		DeferredEvents.Emplace(NewDeferredEvent);
	}
}

void CMacApplication::HandleEvent(const SMacApplicationEvent& Event)
{
    TSharedRef<CMacWindow> Window = GetWindowFromNSWindow(Event.Window);
	
	if (Event.NotificationName)
	{
		NSNotificationName NotificationName = Event.NotificationName;
		
		if (NotificationName == NSWindowWillCloseNotification)
		{
			MessageListener->HandleWindowClosed(Window);
		}
		else if (NotificationName == NSWindowDidMoveNotification)
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
		else if (NotificationName == NSWindowDidBecomeKeyNotification)
		{
			MessageListener->HandleWindowFocusChanged(Window, true);
		}
		else if (NotificationName == NSWindowDidResignKeyNotification)
		{
			MessageListener->HandleWindowFocusChanged(Window, false);
		}
		else if (NotificationName == NSApplicationWillTerminateNotification)
		{
			bIsTerminating = true;
		}
	}
	else if (Event.Event)
	{
		NSEvent* CurrentEvent = Event.Event;
		
		NSEventType EventType = [CurrentEvent type];
		switch(EventType)
		{
			case NSEventTypeKeyUp:
			{
				const SModifierKeyState ModiferKeyState = PlatformApplicationMisc::GetModifierKeyState();
				const EKey Key = CMacKeyMapping::GetKeyCodeFromScanCode([CurrentEvent keyCode]);
				MessageListener->HandleKeyReleased(Key, ModiferKeyState);
				break;
			}
			   
			case NSEventTypeKeyDown:
			{
				const SModifierKeyState ModiferKeyState = PlatformApplicationMisc::GetModifierKeyState();
				const EKey Key = CMacKeyMapping::GetKeyCodeFromScanCode([CurrentEvent keyCode]);
				MessageListener->HandleKeyPressed(Key, [CurrentEvent isARepeat], ModiferKeyState);
				break;
			}

			case NSEventTypeLeftMouseUp:
			case NSEventTypeRightMouseUp:
			case NSEventTypeOtherMouseUp:
			{
				const EMouseButton      Button   = CMacKeyMapping::GetButtonFromIndex(static_cast<int32>([CurrentEvent buttonNumber]));
				const SModifierKeyState KeyState = PlatformApplicationMisc::GetModifierKeyState();
				MessageListener->HandleMouseReleased(Button, KeyState);
				break;
			}

			case NSEventTypeLeftMouseDown:
			case NSEventTypeRightMouseDown:
			case NSEventTypeOtherMouseDown:
			{
				const EMouseButton      Button   = CMacKeyMapping::GetButtonFromIndex(static_cast<int32>([CurrentEvent buttonNumber]));
				const SModifierKeyState KeyState = PlatformApplicationMisc::GetModifierKeyState();
				MessageListener->HandleMousePressed(Button, KeyState);
				break;
			}

			case NSEventTypeLeftMouseDragged:
			case NSEventTypeOtherMouseDragged:
			case NSEventTypeRightMouseDragged:
			case NSEventTypeMouseMoved:
			{
				NSWindow* EventWindow = [CurrentEvent window];
				if (EventWindow)
				{
					const NSPoint MousePosition	= [CurrentEvent locationInWindow];
					const NSRect  ContentRect 	= [[EventWindow contentView] frame];
					
					const int32 x = int32(MousePosition.x);
					const int32 y = int32(ContentRect.size.height - MousePosition.y);
					
					MessageListener->HandleMouseMove(x, y);
					break;
				}
			}
			   
			case NSEventTypeScrollWheel:
			{
				CGFloat ScrollDeltaX = [CurrentEvent scrollingDeltaX];
				CGFloat ScrollDeltaY = [CurrentEvent scrollingDeltaY];
				if ([CurrentEvent hasPreciseScrollingDeltas])
				{
					ScrollDeltaX *= 0.1;
					ScrollDeltaY *= 0.1;
				}
					
				MessageListener->HandleMouseScrolled(int32(ScrollDeltaX), int32(ScrollDeltaY));
				break;
			}
				
			case NSEventTypeMouseEntered:
			{
				NSWindow* EventWindow = [CurrentEvent window];
				
				TSharedRef<CMacWindow> Window = GetWindowFromNSWindow(EventWindow);
				if (Window)
				{
					MessageListener->HandleWindowMouseEntered(Window);
				}

				break;
			}
				
			case NSEventTypeMouseExited:
			{
				NSWindow* EventWindow = [CurrentEvent window];
				
				TSharedRef<CMacWindow> Window = GetWindowFromNSWindow(EventWindow);
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
		MessageListener->HandleKeyTyped(Event.Character);
	}
}
