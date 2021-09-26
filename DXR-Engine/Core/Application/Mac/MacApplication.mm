#if defined(PLATFORM_MACOS)
#include "Core/Application/Platform/PlatformApplicationMisc.h"

#include "MacApplication.h"
#include "MacWindow.h"
#include "ScopedAutoreleasePool.h"
#include "CocoaAppDelegate.h"
#include "CocoaWindow.h"
#include "Notification.h"
#include "MacKeyMappings.h"

#include <AppKit/AppKit.h>

CMacApplication::CMacApplication()
    : AppDelegate(nullptr)
    , Windows()
{
}

CMacApplication::~CMacApplication()
{
    SCOPED_AUTORELEASE_POOL();
    [AppDelegate release];
}

TSharedRef<CGenericWindow> CMacApplication::MakeWindow()
{
    TSharedRef<CMacWindow> NewWindow = new CMacWindow( this );
    Windows.Emplace(NewWindow);
    return NewWindow;
}

bool CMacApplication::Init()
{
    SCOPED_AUTORELEASE_POOL();

    // TODO: Put this check in PlatformThreadMisc to avoid repetiton
    const bool IsMainThread = [NSThread isMainThread];
    Assert( IsMainThread == true ); 

    /* Init application singleton */
    [NSApplication sharedApplication];
    Assert(NSApp != nullptr);
    
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    AppDelegate = [[CCocoaAppDelegate alloc] init:this];
    [NSApp setDelegate:AppDelegate];
    
    // TOOD Init mac mainthread, the initialization should always happen on mainthread so it should be fine anywhere within this function
    
    CMacKeyMappings::Init();

    if (!InitAppMenu())
    {
        LOG_ERROR("[CMacApplication]: Failed to initialize the application menu");
        return false;
    }

    [NSApp finishLaunching];
    
    return true;
}

bool CMacApplication::InitAppMenu()
{
    SCOPED_AUTORELEASE_POOL();

    /* Init the default macOS menu */
    NSMenu*     MenuBar     = [[NSMenu alloc] init];
    NSMenuItem* AppMenuItem = [MenuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu*     AppMenu     = [[NSMenu alloc] init];
    [AppMenuItem setSubmenu:AppMenu];
    
    [AppMenu addItemWithTitle:@"DXR-Engine" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
    [AppMenu addItem: [NSMenuItem separatorItem]];
    
    //Lambda Engine menu item
    NSMenu* ServiceMenu = [[NSMenu alloc] init];
    [[AppMenu addItemWithTitle:@"Services" action:nil keyEquivalent:@""] setSubmenu:ServiceMenu];
    [AppMenu addItem:[NSMenuItem separatorItem]];
    [AppMenu addItemWithTitle:@"Hide DXR-Engine" action:@selector(hide:) keyEquivalent:@"h"];
    [[AppMenu addItemWithTitle:@"Hide Other" action:@selector(hideOtherApplications:) keyEquivalent:@""] setKeyEquivalentModifierMask:NSEventModifierFlagOption | NSEventModifierFlagCommand];
    [AppMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
    [AppMenu addItem:[NSMenuItem separatorItem]];
    [AppMenu addItemWithTitle:@"Quit DXR-Engine" action:@selector(terminate:) keyEquivalent:@"q"];
    
    //Window menu
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
}

void CMacApplication::SetActiveWindow( const TSharedRef<CGenericWindow>& Window )
{
    CCocoaWindow* CocoaWindow = reinterpret_cast<CCocoaWindow*>(Window->GetNativeHandle());
    [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
}

TSharedRef<CGenericWindow> CMacApplication::GetActiveWindow() const
{
    NSWindow* KeyWindow = [NSApp keyWindow];
    return GetWindowFromNSWindow( KeyWindow );
}

TSharedRef<CMacWindow> CMacApplication::GetWindowFromNSWindow( NSWindow* Window ) const
{
    if (Window && [Window isKindOfClass:[CCocoaWindow class]])
    {
        CCocoaWindow* CocoaWindow = reinterpret_cast<CCocoaWindow*>(Window);
        for ( const TSharedRef<CMacWindow>& MacWindow : Windows )
        {
            Assert( MacWindow != nullptr );
            if ( CocoaWindow == reinterpret_cast<CCocoaWindow*>(MacWindow->GetNativeHandle()) )
            {
				return MacWindow;
            }
        }
    }
    
    return nullptr;
}

void CMacApplication::HandleNotification( const SNotification& Notification )
{
    if (Notification.IsValid())
    {
        NSNotificationName     NotificationName = [Notification.Notification name];
        TSharedRef<CMacWindow> Window           = GetWindowFromNSWindow(Notification.Window);

        if (NotificationName == NSWindowWillCloseNotification)
        {
            MessageListener->OnWindowClosed(Window);
        }
        else if (NotificationName == NSWindowDidMoveNotification)
        {
            MessageListener->OnWindowMoved(Window, int16(Notification.Position.x), int16(Notification.Position.y));
        }
        else if (NotificationName == NSWindowDidResizeNotification)
        {
            MessageListener->OnWindowResized(Window, uint16(Notification.Size.width), uint16(Notification.Size.height) );
        }
        else if (NotificationName == NSWindowDidMiniaturizeNotification)
        {
            MessageListener->OnWindowResized(Window, uint16(Notification.Size.width), uint16(Notification.Size.height) );
        }
        else if (NotificationName == NSWindowDidDeminiaturizeNotification)
        {
            MessageListener->OnWindowResized(Window, uint16(Notification.Size.width), uint16(Notification.Size.height) );
        }
        else if (NotificationName == NSWindowDidBecomeKeyNotification)
        {
            MessageListener->OnWindowFocusChanged(Window, true);
        }
        else if (NotificationName == NSWindowDidResignKeyNotification)
        {
            MessageListener->OnWindowFocusChanged(Window, false);
        }
        else if (NotificationName == NSApplicationWillTerminateNotification)
        {
            IsTerminating = true;
        }
    }
}

void CMacApplication::HandleEvent( NSEvent* Event )
{
    NSEventType EventType = [Event type];
    switch(EventType)
    {
        case NSEventTypeKeyUp:
        {
            const uint16 MacKey = [Event keyCode];
            const SModifierKeyState ModiferKeyState = PlatformApplicationMisc::GetModifierKeyState();
			const EKey Key = CMacKeyMappings::GetKeyCodeFromScanCode( MacKey );
            MessageListener->OnKeyReleased( Key, ModiferKeyState );
            
            Keyboard.RegisterKeyState( Key, false );
            break;
        }
           
        case NSEventTypeKeyDown:
        {
            const uint16 MacKey = [Event keyCode];
            const SModifierKeyState ModiferKeyState = PlatformApplicationMisc::GetModifierKeyState();
			const EKey Key = CMacKeyMappings::GetKeyCodeFromScanCode( MacKey );
            MessageListener->OnKeyPressed( Key, [Event isARepeat], ModiferKeyState );
            
            Keyboard.RegisterKeyState( Key, true );
            break;
        }

        case NSEventTypeLeftMouseUp:
        case NSEventTypeRightMouseUp:
        case NSEventTypeOtherMouseUp:
        {
            const NSInteger	   MacButton = [Event buttonNumber];
			const EMouseButton Button	 = CMacKeyMappings::GetButtonFromIndex( MacButton );
            const SModifierKeyState ModiferKeyState = PlatformApplicationMisc::GetModifierKeyState();
            MessageListener->OnMouseReleased( Button, ModiferKeyState );
            
            Cursor.RegisterButtonState( Button, false );
            break;
        }

        case NSEventTypeLeftMouseDown:
        case NSEventTypeRightMouseDown:
        case NSEventTypeOtherMouseDown:
        {
            const NSInteger	   MacButton = [Event buttonNumber];
			const EMouseButton Button	 = CMacKeyMappings::GetButtonFromIndex( MacButton );
            const SModifierKeyState ModiferKeyState = PlatformApplicationMisc::GetModifierKeyState();
            MessageListener->OnMousePressed( Button, ModiferKeyState );
            
            Cursor.RegisterButtonState( Button, true );
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
                
                MessageListener->OnMouseMove( x, y );
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
                
            MessageListener->OnMouseScrolled(int32(ScrollDeltaX), int32(ScrollDeltaY));
            break;
        }
            
        case NSEventTypeMouseEntered:
        {
            NSWindow* EventWindow = [Event window];
            TSharedRef<CMacWindow> Window = GetWindowFromNSWindow(EventWindow);
            if (Window)
            {
                MessageListener->OnWindowMouseEntered(Window);
            }

            break;
        }
            
        case NSEventTypeMouseExited:
        {
            NSWindow* EventWindow = [Event window];
            TSharedRef<CMacWindow> Window = GetWindowFromNSWindow(EventWindow);
            if (Window)
            {
                MessageListener->OnWindowMouseLeft(Window);
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
            MessageListener->OnKeyTyped(uint32(Codepoint));
        }
    }
}

#endif
