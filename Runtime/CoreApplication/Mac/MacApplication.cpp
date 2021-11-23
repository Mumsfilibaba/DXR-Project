#if PLATFORM_MACOS
#include "MacApplication.h"
#include "MacWindow.h"
#include "MacCursor.h"
#include "ScopedAutoreleasePool.h"
#include "CocoaAppDelegate.h"
#include "CocoaWindow.h"
#include "Notification.h"

#include "Core/Logging/Log.h"
#include "Core/Input/Platform/PlatformKeyMapping.h"
#include "Core/Threading/Mac/MacRunLoop.h"
#include "Core/Threading/Platform/PlatformThreadMisc.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include <AppKit/AppKit.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

TSharedPtr<CMacApplication> CMacApplication::Make()
{
	return TSharedPtr<CMacApplication>( dbg_new CMacApplication() );
}

CMacApplication::CMacApplication()
    : CPlatformApplication( CMacCursor::Make() )
	, AppDelegate( nullptr )
    , Windows()
    , WindowsMutex()
    , DeferredEvents()
    , DeferredEventsMutex()
	, IsTerminating( false )
{
}

CMacApplication::~CMacApplication()
{
    SCOPED_AUTORELEASE_POOL();
    [AppDelegate release];
}

TSharedRef<CPlatformWindow> CMacApplication::MakeWindow()
{
    TSharedRef<CMacWindow> NewWindow = CMacWindow::Make( this );
    
    {
        TScopedLock Lock( WindowsLock );
        Windows.Emplace(NewWindow);
    }

    return NewWindow;
}

bool CMacApplication::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    Assert( PlatformThreadMisc::IsMainThread() ); 

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
    const bool IsMainThread = PlatformThreadMisc::IsMainThread();
    Assert( IsMainThread ); 

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

void CMacApplication::Tick( float )
{
    PlatformApplicationMisc::PumpMessages(true);
	
	if ( PlatformThreadMisc::IsMainThread() )
	{
		CMacMainThread::Tick();
	}
}

void CMacApplication::SetActiveWindow( const TSharedRef<CPlatformWindow>& Window )
{
    MakeMainThreadCall(^
    {
        CCocoaWindow* CocoaWindow = reinterpret_cast<CCocoaWindow*>(Window->GetNativeHandle());
        [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
    }, true);
}

TSharedRef<CPlatformWindow> CMacApplication::GetActiveWindow() const
{
	SCOPED_AUTORELEASE_POOL();

    NSWindow* KeyWindow = [NSApp keyWindow];
    return GetWindowFromNSWindow( KeyWindow );
}

TSharedRef<CPlatformWindow> CMacApplication::GetWindowUnderCursor() const
{
	SCOPED_AUTORELEASE_POOL();
	
	NSPoint   MousePosition = [NSEvent mouseLocation];
	NSInteger WindowNumber  = [NSWindow windowNumberAtPoint:MousePosition belowWindowWithWindowNumber:0];
	
	NSWindow* Window = [NSApp windowWithWindowNumber:WindowNumber];
	return GetWindowFromNSWindow( Window );
}

TSharedRef<CMacWindow> CMacApplication::GetWindowFromNSWindow( NSWindow* Window ) const
{
    if (Window && [Window isKindOfClass:[CCocoaWindow class]])
    {
        TScopedLock Lock( WindowsMutex );

        CCocoaWindow* CocoaWindow = reinterpret_cast<CCocoaWindow*>(Window);
        for ( const TSharedRef<CMacWindow>& MacWindow : Windows )
        {
            if ( CocoaWindow == reinterpret_cast<CCocoaWindow*>(MacWindow->GetNativeHandle()) )
            {
                return MacWindow;
            }
        }
    }
    
    return nullptr;
}

void CMacApplication::StoreEvent( NSObject* EventOrNotificationObject )
{
    if ( [EventOrNotificationObject isKindOfClass:[NSEvent class]] )
    {
        NSEvent* Event = reinterpret_cast<NSEvent*>(EventOrNotificationObject);

        SMacApplicationEvent DeferredEvent;
        DeferredEvent.Event  = [Event retain];
        DeferredEvent.Window = [[Event window] retain];

        TScopedLock Lock(DeferredEventLock);
        DeferredEvent.Emplace( DeferredEvent ); 
    }
    else if ( [EventOrNotificationObject isKindOfClass:[NSNotification class]] )
    {
        NSNotification* Notification = reinterpret_cast<NSNotification*>(EventOrNotificationObject);
        NSString*   NotificationName = [Notification name]; 

        SMacApplicationEvent DeferredEvent;
        DeferredEvent.NotificationName = [NotificationName retain];

        TScopedLock Lock(DeferredEventLock);
        DeferredEvent.Emplace( DeferredEvent ); 
    }
}

void CMacApplication::HandleNotification( const SMacApplicationEvent& Notification )
{
    TSharedRef<CMacWindow> Window = GetWindowFromNSWindow(Notification.Window);

    NSNotificationName NotificationName = [Notification.Notification name];
    if (NotificationName == NSWindowWillCloseNotification)
    {
        MessageListener->HandleWindowClosed(Window);
    }
    else if (NotificationName == NSWindowDidMoveNotification)
    {
        MessageListener->HandleWindowMoved(Window, int16(Notification.Position.x), int16(Notification.Position.y));
    }
    else if (NotificationName == NSWindowDidResizeNotification)
    {
        MessageListener->HandleWindowResized(Window, uint16(Notification.Size.width), uint16(Notification.Size.height) );
    }
    else if (NotificationName == NSWindowDidMiniaturizeNotification)
    {
        MessageListener->HandleWindowResized(Window, uint16(Notification.Size.width), uint16(Notification.Size.height) );
    }
    else if (NotificationName == NSWindowDidDeminiaturizeNotification)
    {
        MessageListener->HandleWindowResized(Window, uint16(Notification.Size.width), uint16(Notification.Size.height) );
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
        IsTerminating = true;
    }
}

void CMacApplication::HandleEvent( NSEvent* Event )
{
    NSEventType EventType = [Event type];
    switch(EventType)
    {
        case NSEventTypeKeyUp:
        {
            const SModifierKeyState ModiferKeyState = PlatformApplicationMisc::GetModifierKeyState();
            const EKey Key = CMacKeyMapping::GetKeyCodeFromScanCode( [Event keyCode] );
            MessageListener->HandleKeyReleased( Key, ModiferKeyState );
            break;
        }
           
        case NSEventTypeKeyDown:
        {
            const SModifierKeyState ModiferKeyState = PlatformApplicationMisc::GetModifierKeyState();
            const EKey Key = CMacKeyMapping::GetKeyCodeFromScanCode( [Event keyCode] );
            MessageListener->HandleKeyPressed( Key, [Event isARepeat], ModiferKeyState );
            break;
        }

        case NSEventTypeLeftMouseUp:
        case NSEventTypeRightMouseUp:
        case NSEventTypeOtherMouseUp:
        {
            const EMouseButton      Button   = CMacKeyMapping::GetButtonFromIndex( static_cast<int32>([Event buttonNumber]) );
            const SModifierKeyState KeyState = PlatformApplicationMisc::GetModifierKeyState();
            MessageListener->HandleMouseReleased( Button, KeyState );
            break;
        }

        case NSEventTypeLeftMouseDown:
        case NSEventTypeRightMouseDown:
        case NSEventTypeOtherMouseDown:
        {
            const EMouseButton      Button   = CMacKeyMapping::GetButtonFromIndex( static_cast<int32>([Event buttonNumber]) );
            const SModifierKeyState KeyState = PlatformApplicationMisc::GetModifierKeyState();
            MessageListener->HandleMousePressed( Button, KeyState );
            break;
        }

        case NSEventTypeLeftMouseDragged:
        case NSEventTypeOtherMouseDragged:
        case NSEventTypeRightMouseDragged:
        case NSEventTypeMouseMoved:
        {
            NSWindow* EventWindow = [Event window];
            if (EventWindow)
            {
                const NSPoint MousePosition	= [Event locationInWindow];
                const NSRect  ContentRect 	= [[EventWindow contentView] frame];
                
                const int32 x = int32(MousePosition.x);
                const int32 y = int32(ContentRect.size.height - MousePosition.y);
                
                MessageListener->HandleMouseMove( x, y );
                break;
            }
        }
           
        case NSEventTypeScrollWheel:
        {
            CGFloat ScrollDeltaX = [Event scrollingDeltaX];
            CGFloat ScrollDeltaY = [Event scrollingDeltaY];
            if ([Event hasPreciseScrollingDeltas])
            {
                ScrollDeltaX *= 0.1;
                ScrollDeltaY *= 0.1;
            }
                
            MessageListener->HandleMouseScrolled(int32(ScrollDeltaX), int32(ScrollDeltaY));
            break;
        }
            
        case NSEventTypeMouseEntered:
        {
            NSWindow* EventWindow = [Event window];
            TSharedRef<CMacWindow> Window = GetWindowFromNSWindow(EventWindow);
            if (Window)
            {
                MessageListener->HandleWindowMouseEntered(Window);
            }

            break;
        }
            
        case NSEventTypeMouseExited:
        {
            NSWindow* EventWindow = [Event window];
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

void CMacApplication::HandleKeyTypedEvent( NSString* Text )
{
    NSUInteger Count = [Text length];
    for (NSUInteger Index = 0; Index < Count; Index++)
    {
        const unichar Codepoint = [Text characterAtIndex:Index];
        if ((Codepoint & 0xff00) != 0xf700)
        {
            MessageListener->HandleKeyTyped( uint32(Codepoint) );
        }
    }
}

#endif
