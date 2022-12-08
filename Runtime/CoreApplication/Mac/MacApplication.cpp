#include "MacApplication.h"
#include "MacWindow.h"
#include "MacCursor.h"
#include "CocoaWindow.h"

#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Input/Platform/PlatformKeyMapping.h"
#include "Core/Mac/MacRunLoop.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Threading/ScopedLock.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

FMacApplication* MacApplication = nullptr;

FMacApplication* FMacApplication::CreateMacApplication()
{
    // Set the global instance
    MacApplication = new FMacApplication();
    return MacApplication;
}

FMacApplication::FMacApplication()
    : FGenericApplication(TSharedPtr<ICursor>(new FMacCursor()))
    , Windows()
    , WindowsCS()
    , DeferredEvents()
    , DeferredEventsCS()
{
    SCOPED_AUTORELEASE_POOL();
    
    // This should only be init from the main thread, but assert just to be sure.
    CHECK(FPlatformThreadMisc::IsMainThread());

    FPlatformKeyMapping::Initialize();
    
    // Init the default macOS menu
    NSMenu* MenuBar = [NSMenu new];
    NSMenuItem* AppMenuItem = [MenuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];
    
    NSMenu* AppMenu = [NSMenu new];
    AppMenuItem.submenu = AppMenu;
    
    [AppMenu addItemWithTitle:@"DXR-Engine" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
    [AppMenu addItem: [NSMenuItem separatorItem]];
    
    // Engine menu item
    NSMenu* ServiceMenu = [NSMenu new];
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

FMacApplication::~FMacApplication()
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

FGenericWindowRef FMacApplication::CreateWindow()
{
    TSharedRef<FMacWindow> NewWindow = new FMacWindow(this);
    
    {
        TScopedLock Lock(WindowsCS);
        Windows.Emplace(NewWindow);
    }

    return NewWindow;
}

void FMacApplication::Tick(float)
{
    SCOPED_AUTORELEASE_POOL();
    
    FPlatformApplicationMisc::PumpMessages(true);
    
    TArray<FDeferredMacEvent> ProcessableEvents;
    if (!DeferredEvents.IsEmpty())
    {
        TScopedLock Lock(DeferredEventsCS);
        
        ProcessableEvents.Swap(DeferredEvents);
        DeferredEvents.Clear();
    }
    
    for (const FDeferredMacEvent& CurrentEvent : ProcessableEvents)
    {
        ProcessDeferredEvent(CurrentEvent);
    }
    
    if (!ClosedWindows.IsEmpty())
    {
        TScopedLock Lock(ClosedWindowsCS);
        
        for (const TSharedRef<FMacWindow>& Window : ClosedWindows)
        {
            MessageHandler->HandleWindowClosed(Window);
        }
        
        ClosedWindows.Clear();
    }
}

void FMacApplication::SetActiveWindow(const FGenericWindowRef& Window)
{
    __block TSharedRef<FMacWindow> MacWindow = StaticCastSharedRef<FMacWindow>(Window);
    ExecuteOnMainThread(^
    {
        FCocoaWindow* CocoaWindow = MacWindow->GetWindowHandle();
        [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
    }, NSDefaultRunLoopMode, false);
}

FGenericWindowRef FMacApplication::GetActiveWindow() const
{
    @autoreleasepool
    {
        NSWindow* KeyWindow = NSApp.keyWindow;
        return GetWindowFromNSWindow(KeyWindow);
    }
}

FGenericWindowRef FMacApplication::GetWindowUnderCursor() const
{
    @autoreleasepool
    {
        const NSInteger WindowNumber = [NSWindow windowNumberAtPoint:[NSEvent mouseLocation] belowWindowWithWindowNumber:0];
        return GetWindowFromNSWindow([NSApp windowWithWindowNumber:WindowNumber]);
    }
}

TSharedRef<FMacWindow> FMacApplication::GetWindowFromNSWindow(NSWindow* Window) const
{
    if (Window && [Window isKindOfClass:[FCocoaWindow class]])
    {
        TScopedLock Lock(WindowsCS);

        FCocoaWindow* CocoaWindow = reinterpret_cast<FCocoaWindow*>(Window);
        for (const TSharedRef<FMacWindow>& MacWindow : Windows)
        {
            if (CocoaWindow == reinterpret_cast<FCocoaWindow*>(MacWindow->GetPlatformHandle()))
            {
                return MacWindow;
            }
        }
    }
    
    return nullptr;
}

void FMacApplication::CloseWindow(const TSharedRef<FMacWindow>& Window)
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

void FMacApplication::DeferEvent(NSObject* EventOrNotificationObject)
{
    SCOPED_AUTORELEASE_POOL();
    
    if (EventOrNotificationObject)
    {
        FDeferredMacEvent NewDeferredEvent;
        
        if ([EventOrNotificationObject isKindOfClass:[NSEvent class]])
        {
            NSEvent* Event = reinterpret_cast<NSEvent*>(EventOrNotificationObject);
            NewDeferredEvent.Event  = [Event retain];
            
            NSWindow* Window = Event.window;
            if ([Window isKindOfClass: [FCocoaWindow class]])
            {
                FCocoaWindow* EventWindow = reinterpret_cast<FCocoaWindow*>(Window);
                NewDeferredEvent.Window   = [EventWindow retain];
            }
        }
        else if ([EventOrNotificationObject isKindOfClass:[NSNotification class]])
        {
            NSNotification* Notification = reinterpret_cast<NSNotification*>(EventOrNotificationObject);
            NewDeferredEvent.NotificationName = [Notification.name retain];
            
            NSObject* NotificationObject = Notification.object;
            if ([NotificationObject isKindOfClass: [FCocoaWindow class]])
            {
                FCocoaWindow* EventWindow = reinterpret_cast<FCocoaWindow*>(NotificationObject);
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

void FMacApplication::ProcessDeferredEvent(const FDeferredMacEvent& Event)
{
    SCOPED_AUTORELEASE_POOL();
    
    TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(Event.Window);
    
    if (Event.NotificationName)
    {
        NSNotificationName NotificationName = Event.NotificationName;
        
        if (NotificationName == NSWindowDidMoveNotification)
        {
            MessageHandler->HandleWindowMoved(Window, int16(Event.Position.x), int16(Event.Position.y));
        }
        else if (NotificationName == NSWindowDidResizeNotification)
        {
            MessageHandler->HandleWindowResized(Window, uint16(Event.Size.width), uint16(Event.Size.height));
        }
        else if (NotificationName == NSWindowDidMiniaturizeNotification)
        {
            MessageHandler->HandleWindowResized(Window, uint16(Event.Size.width), uint16(Event.Size.height));
        }
        else if (NotificationName == NSWindowDidDeminiaturizeNotification)
        {
            MessageHandler->HandleWindowResized(Window, uint16(Event.Size.width), uint16(Event.Size.height));
        }
        else if (NotificationName == NSWindowDidBecomeMainNotification)
        {
            MessageHandler->HandleWindowFocusChanged(Window, true);
        }
        else if (NotificationName == NSWindowDidResignMainNotification)
        {
            MessageHandler->HandleWindowFocusChanged(Window, false);
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
                MessageHandler->HandleKeyReleased(FPlatformKeyMapping::GetKeyCodeFromScanCode(CurrentEvent.keyCode), FPlatformApplicationMisc::GetModifierKeyState());
                break;
            }
               
            case NSEventTypeKeyDown:
            {
                MessageHandler->HandleKeyPressed(FPlatformKeyMapping::GetKeyCodeFromScanCode(CurrentEvent.keyCode), CurrentEvent.ARepeat, FPlatformApplicationMisc::GetModifierKeyState());
                break;
            }

            case NSEventTypeLeftMouseUp:
            case NSEventTypeRightMouseUp:
            case NSEventTypeOtherMouseUp:
            {
                MessageHandler->HandleMouseReleased(FPlatformKeyMapping::GetButtonFromIndex(static_cast<int32>(CurrentEvent.buttonNumber)), FPlatformApplicationMisc::GetModifierKeyState());
                break;
            }

            case NSEventTypeLeftMouseDown:
            case NSEventTypeRightMouseDown:
            case NSEventTypeOtherMouseDown:
            {
                MessageHandler->HandleMousePressed(FPlatformKeyMapping::GetButtonFromIndex(static_cast<int32>(CurrentEvent.buttonNumber)), FPlatformApplicationMisc::GetModifierKeyState());
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
                    const NSPoint MousePosition    = CurrentEvent.locationInWindow;
                    const NSRect  ContentRect     = EventWindow.contentView.frame;
                    
                    const int32 x = int32(MousePosition.x);
                    const int32 y = int32(ContentRect.size.height - MousePosition.y);
                    
                    MessageHandler->HandleMouseMove(x, y);
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
                    
                MessageHandler->HandleMouseScrolled(int32(ScrollDeltaX), int32(ScrollDeltaY));
                break;
            }
                
            case NSEventTypeMouseEntered:
            {
                TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(CurrentEvent.window);
                if (Window)
                {
                    MessageHandler->HandleWindowMouseEntered(Window);
                }

                break;
            }
                
            case NSEventTypeMouseExited:
            {
                TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(CurrentEvent.window);
                if (Window)
                {
                    MessageHandler->HandleWindowMouseLeft(Window);
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
        MessageHandler->HandleKeyChar(Event.Character);
    }
}
