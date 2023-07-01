#include "MacApplication.h"
#include "MacWindow.h"
#include "MacCursor.h"
#include "CocoaWindow.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Mac/MacRunLoop.h"
#include "Core/Platform/PlatformKeyMapping.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Threading/ScopedLock.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"

@interface FMacApplicationObserver : NSObject

- (void) onApplicationBecomeActive:(NSNotification*)InNotification;
- (void) onApplicationBecomeInactive:(NSNotification*)InNotification;
- (void) displaysDidChange:(NSNotification*)InNotification;

@end

@implementation FMacApplicationObserver

- (void)onApplicationBecomeActive:(NSNotification*)InNotification
{
    CHECK(MacApplication != nullptr);
    MacApplication->DeferEvent(InNotification);
}

- (void)onApplicationBecomeInactive:(NSNotification*)InNotification
{
    CHECK(MacApplication != nullptr);
    MacApplication->DeferEvent(InNotification);
}

- (void)displaysDidChange:(NSNotification*)InNotification
{
    CHECK(MacApplication != nullptr);
    MacApplication->DeferEvent(InNotification);
}

@end


FMacApplication* MacApplication = nullptr;

FMacApplication* FMacApplication::CreateMacApplication()
{
    // Set the global instance
    MacApplication = new FMacApplication();
    return MacApplication;
}

FMacApplication::FMacApplication()
    : FGenericApplication(MakeShared<FMacCursor>())
    , Windows()
    , WindowsCS()
    , ClosedWindows()
    , ClosedWindowsCS()
    , DeferredEvents()
    , DeferredEventsCS()
    , LastPressedButton(EMouseButtonName::Unknown)
    , Observer()
{
    SCOPED_AUTORELEASE_POOL();
    
    // This should only be initialized from the main thread, but assert just to be sure.
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

    Observer = [FMacApplicationObserver new];    
    [[NSNotificationCenter defaultCenter] addObserver:Observer
                                             selector:@selector(onApplicationBecomeActive:)
                                                 name:NSApplicationDidBecomeActiveNotification
                                               object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:Observer
                                             selector:@selector(onApplicationBecomeInactive:)
                                                 name:NSApplicationDidResignActiveNotification
                                               object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:Observer
                                             selector:@selector(displaysDidChange:)
                                                 name:NSApplicationDidChangeScreenParametersNotification
                                               object:nil];
}

FMacApplication::~FMacApplication()
{
    @autoreleasepool
    {
        [[NSNotificationCenter defaultCenter] removeObserver:Observer
                                                        name:NSApplicationDidBecomeActiveNotification
                                                      object:nil];

        [[NSNotificationCenter defaultCenter] removeObserver:Observer
                                                        name:NSApplicationDidResignActiveNotification
                                                      object:nil];

        [[NSNotificationCenter defaultCenter] removeObserver:Observer
                                                        name:NSApplicationDidChangeScreenParametersNotification
                                                      object:nil];

        NSSafeRelease(Observer);

        Windows.Clear();

        if (this == MacApplication)
        {
            MacApplication = nullptr;
        }
    }
}

TSharedRef<FGenericWindow> FMacApplication::CreateWindow()
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
        
        ::Swap(ProcessableEvents, DeferredEvents);
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
            MessageHandler->OnWindowClosed(Window);
        }
        
        ClosedWindows.Clear();
    }
}

void FMacApplication::SetActiveWindow(const TSharedRef<FGenericWindow>& Window)
{
    __block TSharedRef<FMacWindow> MacWindow = StaticCastSharedRef<FMacWindow>(Window);
    ExecuteOnMainThread(^
    {
        FCocoaWindow* CocoaWindow = MacWindow->GetWindow();
        [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
    }, NSDefaultRunLoopMode, false);
}

TSharedRef<FGenericWindow> FMacApplication::GetActiveWindow() const
{
    @autoreleasepool
    {
        NSWindow* KeyWindow = NSApp.keyWindow;
        return GetWindowFromNSWindow(KeyWindow);
    }
}

TSharedRef<FGenericWindow> FMacApplication::GetWindowUnderCursor() const
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
            MessageHandler->OnWindowMoved(Window, int16(Event.Position.x), int16(Event.Position.y));
        }
        else if (NotificationName == NSWindowDidResizeNotification)
        {
            MessageHandler->OnWindowResized(Window, uint16(Event.Size.width), uint16(Event.Size.height));
        }
        else if (NotificationName == NSWindowDidMiniaturizeNotification)
        {
            MessageHandler->OnWindowResized(Window, uint16(Event.Size.width), uint16(Event.Size.height));
        }
        else if (NotificationName == NSWindowDidDeminiaturizeNotification)
        {
            MessageHandler->OnWindowResized(Window, uint16(Event.Size.width), uint16(Event.Size.height));
        }
        else if (NotificationName == NSWindowDidBecomeMainNotification)
        {
            MessageHandler->OnWindowFocusGained(Window);
        }
        else if (NotificationName == NSWindowDidResignMainNotification)
        {
            MessageHandler->OnWindowFocusLost(Window);
        }
        else if (NotificationName == NSApplicationDidChangeScreenParametersNotification)
        {
            MessageHandler->OnMonitorChange();
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
                MessageHandler->OnKeyUp(FPlatformKeyMapping::GetKeyCodeFromScanCode(CurrentEvent.keyCode), FPlatformApplicationMisc::GetModifierKeyState());
                break;
            }
               
            case NSEventTypeKeyDown:
            {
                MessageHandler->OnKeyDown(FPlatformKeyMapping::GetKeyCodeFromScanCode(CurrentEvent.keyCode), CurrentEvent.ARepeat, FPlatformApplicationMisc::GetModifierKeyState());
                break;
            }

            case NSEventTypeLeftMouseUp:
            case NSEventTypeRightMouseUp:
            case NSEventTypeOtherMouseUp:
            {
                const NSPoint CursorPosition = [NSEvent mouseLocation];
                MessageHandler->OnMouseButtonUp(FPlatformKeyMapping::GetButtonFromIndex(static_cast<int32>(CurrentEvent.buttonNumber)), FPlatformApplicationMisc::GetModifierKeyState(), static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
                break;
            }

            case NSEventTypeLeftMouseDown:
            case NSEventTypeRightMouseDown:
            case NSEventTypeOtherMouseDown:
            {
                const EMouseButtonName::Type CurrentMouseButton = FPlatformKeyMapping::GetButtonFromIndex(static_cast<int32>(CurrentEvent.buttonNumber));
                
                const NSPoint CursorPosition = [NSEvent mouseLocation];
                if (LastPressedButton == CurrentMouseButton && CurrentEvent.clickCount % 2 == 0)
                {
                    MessageHandler->OnMouseButtonDoubleClick(Window, CurrentMouseButton, FPlatformApplicationMisc::GetModifierKeyState(), static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
                }
                else
                {
                    MessageHandler->OnMouseButtonDown(Window, CurrentMouseButton, FPlatformApplicationMisc::GetModifierKeyState(), static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
                }
                
                // Save the mousebutton to handle double-click events
                LastPressedButton = CurrentMouseButton;
                break;
            }

            case NSEventTypeLeftMouseDragged:
            case NSEventTypeOtherMouseDragged:
            case NSEventTypeRightMouseDragged:
            case NSEventTypeMouseMoved:
            {
                const NSPoint CursorPosition = [NSEvent mouseLocation];
                MessageHandler->OnMouseMove(static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
                break;
            }
               
            case NSEventTypeScrollWheel:
            {
                if (CurrentEvent.phase != NSEventPhaseCancelled)
                {
                    CGFloat ScrollDeltaX = CurrentEvent.scrollingDeltaX;
                    CGFloat ScrollDeltaY = CurrentEvent.scrollingDeltaY;
                    
                    if (CurrentEvent.hasPreciseScrollingDeltas)
                    {
                        ScrollDeltaX *= 0.1;
                        ScrollDeltaY *= 0.1;
                    }
                    
                    const NSPoint CursorPosition = [NSEvent mouseLocation];
                    MessageHandler->OnMouseScrolled(int32(ScrollDeltaX), int32(ScrollDeltaY), static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
                }

                break;
            }
                
            case NSEventTypeMouseEntered:
            {
                TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(CurrentEvent.window);
                if (Window)
                {
                    MessageHandler->OnWindowMouseEntered(Window);
                }

                break;
            }
                
            case NSEventTypeMouseExited:
            {
                TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(CurrentEvent.window);
                if (Window)
                {
                    MessageHandler->OnWindowMouseLeft(Window);
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
        MessageHandler->OnKeyChar(Event.Character);
    }
}
