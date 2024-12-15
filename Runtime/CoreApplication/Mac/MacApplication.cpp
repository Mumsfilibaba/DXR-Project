#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Mac/MacThreadManager.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Threading/ScopedLock.h"
#include "CoreApplication/Mac/MacApplication.h"
#include "CoreApplication/Mac/MacWindow.h"
#include "CoreApplication/Mac/CocoaWindow.h"
#include "CoreApplication/Mac/MacCursor.h"
#include "CoreApplication/Platform/PlatformInputMapper.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"
#include <AppKit/AppKit.h>
#include <IOKit/graphics/IOGraphicsLib.h>

@interface FMacApplicationObserver : NSObject

- (void)onApplicationBecomeActive:(NSNotification*)InNotification;
- (void)onApplicationBecomeInactive:(NSNotification*)InNotification;
- (void)displaysDidChange:(NSNotification*)InNotification;

@end

@implementation FMacApplicationObserver

- (void)onApplicationBecomeActive:(NSNotification*)InNotification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(InNotification);
    }
}

- (void)onApplicationBecomeInactive:(NSNotification*)InNotification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(InNotification);
    }
}

- (void)displaysDidChange:(NSNotification*)InNotification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(InNotification);
    }
}

@end

FMacApplication* GMacApplication = nullptr;

TSharedPtr<FGenericApplication> FMacApplication::Create()
{
    // Create the cursor interface
    TSharedPtr<FMacCursor> Cursor = MakeSharedPtr<FMacCursor>();

    // Create a new MacApplication instance. The global MacApplication pointer is initialized inside of the
    // FMacApplication constructor, and later on destroyed in the destructor.
    TSharedPtr<FMacApplication> NewMacApplication = MakeSharedPtr<FMacApplication>(Cursor);
    return NewMacApplication;
}

FMacApplication::FMacApplication(const TSharedPtr<FMacCursor>& InCursor)
    : FGenericApplication(InCursor)
    , LocalEventMonitor(nullptr)
    , GlobalMouseMovedEventMonitor(nullptr)
    , Observer(nullptr)
    , WindowUnderCursor(nullptr)
    , CurrentModifierFlags(0)
    , LastPressedButton(EMouseButtonName::Unknown)
    , InputDevice(FGCInputDevice::CreateGCInputDevice())
    , MacCursor(InCursor)
    , Windows()
    , WindowsCS()
    , ClosedWindows()
    , ClosedWindowsCS()
    , DeferredEvents()
    , DeferredEventsCS()
{
    if (!GMacApplication)
    {
        GMacApplication = this;
    }
    
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        CHECK(FPlatformThreadMisc::IsMainThread());

        // We need to map input from the macOS specific key-codes etc. which needs to be initialized somewhere
        // and this seems like the best place to do this, however we might need to move this if the input-mapping
        // is necessary somewhere else at an earlier point than at the MacApplication initalization time.
        FPlatformInputMapper::Initialize();
        
        // Initialize the default macOS menu programmatically.
        //
        // Since this application does not use a NIB (Interface Builder) file, which typically contains
        // information about the application's menu structure, we need to create the menu manually.
        //
        // This code sets up the main menu bar (`NSMenu`) for the application, including the application
        // menu and the window menu. The application menu contains standard items such as "About",
        // "Services", "Hide", "Quit", etc. The window menu includes items like "Minimize", "Zoom",
        // "Bring All to Front", and "Enter Full Screen".
        //
        // The application menu is associated with the first item of the main menu bar and is configured
        // with standard selectors to provide expected macOS behaviors. For example:
        //   - "About DXR-Engine" opens the standard About panel.
        //   - "Hide DXR-Engine" hides the application.
        //   - "Quit DXR-Engine" terminates the application.
        //
        // TODO: Probably not call this DXR-Engine but read from some file what the application is actually called
        //
        // The window menu is associated with the second item of the main menu bar and provides window
        // management actions, such as minimizing and zooming windows, and entering full-screen mode.
        //
        // By manually creating and configuring the menu bar we ensure that the application integrates
        // properly with macOS conventions and provides a familiar user experience, even without using
        // a NIB file.
        
        // Initialize the default macOS menu
        NSMenu*     MenuBar     = [NSMenu new];
        NSMenuItem* AppMenuItem = [MenuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];

        // Create the application menu
        NSMenu* AppMenu = [NSMenu new];
        AppMenuItem.submenu = AppMenu;

        // Add standard application menu items
        [AppMenu addItemWithTitle:@"DXR-Engine" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
        [AppMenu addItem:[NSMenuItem separatorItem]];

        // Services menu item
        NSMenu* ServiceMenu = [NSMenu new];
        [AppMenu addItemWithTitle:@"Services" action:nil keyEquivalent:@""].submenu = ServiceMenu;
        [AppMenu addItem:[NSMenuItem separatorItem]];
        [AppMenu addItemWithTitle:@"Hide DXR-Engine" action:@selector(hide:) keyEquivalent:@"h"];
        [AppMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@""].keyEquivalentModifierMask = NSEventModifierFlagOption | NSEventModifierFlagCommand;
        [AppMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
        [AppMenu addItem:[NSMenuItem separatorItem]];
        [AppMenu addItemWithTitle:@"Quit DXR-Engine" action:@selector(terminate:) keyEquivalent:@"q"];

        // Create the window menu
        NSMenuItem* WindowMenuItem = [MenuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];

        NSMenu* WindowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
        WindowMenuItem.submenu = WindowMenu;

        // Add window menu items
        [WindowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
        [WindowMenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
        [WindowMenu addItem:[NSMenuItem separatorItem]];
        [WindowMenu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];
        [WindowMenu addItem:[NSMenuItem separatorItem]];
        [WindowMenu addItemWithTitle:@"Enter Full Screen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"].keyEquivalentModifierMask = NSEventModifierFlagControl | NSEventModifierFlagCommand;

        // Set the application menu
        SEL SetAppleMenuSelector = NSSelectorFromString(@"setAppleMenu:");
        [NSApp performSelector:SetAppleMenuSelector withObject:AppMenu];

        // Assign the menu bar and menus to the application
        NSApp.mainMenu     = MenuBar;
        NSApp.windowsMenu  = WindowMenu;
        NSApp.servicesMenu = ServiceMenu;

        Observer = [FMacApplicationObserver new];
        [[NSNotificationCenter defaultCenter] addObserver:Observer selector:@selector(onApplicationBecomeActive:) name:NSApplicationDidBecomeActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:Observer selector:@selector(onApplicationBecomeInactive:) name:NSApplicationDidResignActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:Observer selector:@selector(displaysDidChange:) name:NSApplicationDidChangeScreenParametersNotification object:nil];
        
        LocalEventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskAny handler:^(NSEvent* Event)
        {
            return OnNSEvent(Event);
        }];

        GlobalMouseMovedEventMonitor = [NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskMouseMoved handler:^(NSEvent* Event)
        {
            DeferEvent(Event);
        }];
        
        FCocoaWindow* NewWindowUnderCursor = FindNSWindowUnderCursor();
        WindowUnderCursor = [NewWindowUnderCursor retain];
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

FMacApplication::~FMacApplication()
{
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        [[NSNotificationCenter defaultCenter] removeObserver:Observer name:NSApplicationDidBecomeActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] removeObserver:Observer name:NSApplicationDidResignActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] removeObserver:Observer name:NSApplicationDidChangeScreenParametersNotification object:nil];
        [Observer release];
        
        if (GlobalMouseMovedEventMonitor)
        {
            [NSEvent removeMonitor:GlobalMouseMovedEventMonitor];
        }
        if (LocalEventMonitor)
        {
            [NSEvent removeMonitor:LocalEventMonitor];
        }
        
        [WindowUnderCursor release];
        WindowUnderCursor = nil;
    }, NSDefaultRunLoopMode, true);

    Windows.Clear();

    if (this == GMacApplication)
    {
        GMacApplication = nullptr;
    }
}

TSharedRef<FGenericWindow> FMacApplication::CreateWindow()
{
    TSharedRef<FMacWindow> NewWindow = FMacWindow::Create(this);
    
    TScopedLock Lock(WindowsCS);
    Windows.Emplace(NewWindow);
    return NewWindow;
}

void FMacApplication::Tick(float)
{
    SCOPED_AUTORELEASE_POOL();
    
    TArray<TSharedRef<FMacWindow>> LocalClosedWindows;
    if (!ClosedWindows.IsEmpty())
    {
        TScopedLock Lock(ClosedWindowsCS);
        LocalClosedWindows = Move(ClosedWindows);
        ClosedWindows.Clear();
    }
    
    for (const TSharedRef<FMacWindow>& Window : LocalClosedWindows)
    {
        MessageHandler->OnWindowClosed(Window);
    }

    TArray<FCocoaWindow*> LocalClosedCocoaWindows;
    if (!ClosedCocoaWindows.IsEmpty())
    {
        TScopedLock Lock(ClosedCocoaWindowsCS);
        LocalClosedCocoaWindows = Move(ClosedCocoaWindows);
        ClosedCocoaWindows.Clear();
    }
    
    if (!LocalClosedCocoaWindows.IsEmpty())
    {
        FMacThreadManager::Get().MainThreadDispatch(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            for (FCocoaWindow* CocoaWindow : LocalClosedCocoaWindows)
            {
                [CocoaWindow close];
                [CocoaWindow release];
            }
        }, NSDefaultRunLoopMode, true);
    }
}

void FMacApplication::ProcessEvents()
{
    FPlatformApplicationMisc::PumpMessages(true);
}

void FMacApplication::ProcessDeferredEvents()
{
    TArray<FDeferredMacEvent> LocalDeferredEvents;
    if (!DeferredEvents.IsEmpty())
    {
        TScopedLock Lock(DeferredEventsCS);
        LocalDeferredEvents = Move(DeferredEvents);
        DeferredEvents.Clear();
    }

    for (const FDeferredMacEvent& CurrentEvent : LocalDeferredEvents)
    {
        ProcessDeferredEvent(CurrentEvent);
    }
}

void FMacApplication::UpdateInputDevices()
{
    if (InputDevice)
    {
        InputDevice->UpdateDeviceState();
    }
}

FInputDevice* FMacApplication::GetInputDevice()
{
    return InputDevice.Get();
}

bool FMacApplication::SupportsHighPrecisionMouse() const
{
    // TODO: Implement high precision mouse
    return false;
}

bool FMacApplication::EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>&)
{
    // TODO: Implement high precision mouse
    return false;
}

FModifierKeyState FMacApplication::GetModifierKeyState() const
{
    EModifierFlag ModifierFlags = EModifierFlag::None;
    if (CurrentModifierFlags & NSEventModifierFlagControl)
    {
        ModifierFlags |= EModifierFlag::Ctrl;
    }
    if (CurrentModifierFlags & NSEventModifierFlagShift)
    {
        ModifierFlags |= EModifierFlag::Shift;
    }
    if (CurrentModifierFlags & NSEventModifierFlagOption)
    {
        ModifierFlags |= EModifierFlag::Alt;
    }
    if (CurrentModifierFlags & NSEventModifierFlagCommand)
    {
        ModifierFlags |= EModifierFlag::Super;
    }
    if (CurrentModifierFlags & NSEventModifierFlagCapsLock)
    {
        ModifierFlags |= EModifierFlag::CapsLock;
    }

    return FModifierKeyState(ModifierFlags);
}

void FMacApplication::SetActiveWindow(const TSharedRef<FGenericWindow>& Window)
{
    __block TSharedRef<FMacWindow> MacWindow = StaticCastSharedRef<FMacWindow>(Window);
    FMacThreadManager::Get().MainThreadDispatch(^
    {
        FCocoaWindow* CocoaWindow = MacWindow->GetCocoaWindow();
        [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
    }, NSDefaultRunLoopMode, false);
}

TSharedRef<FGenericWindow> FMacApplication::GetActiveWindow() const
{
    NSWindow* KeyWindow = FMacThreadManager::Get().MainThreadDispatchAndReturn(^
    {
        SCOPED_AUTORELEASE_POOL();
        return [NSApp keyWindow];
    }, NSDefaultRunLoopMode);
    
    return FindWindowFromNSWindow(KeyWindow);
}

TSharedRef<FGenericWindow> FMacApplication::GetWindowUnderCursor() const
{
    return FindWindowFromNSWindow(WindowUnderCursor);
}

void FMacApplication::QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const
{
    NSScreen* MainScreen = [NSScreen mainScreen];

    const int32 NumMonitors = static_cast<int32>(NSScreen.screens.count);
    OutMonitorInfo.Resize(NumMonitors);

    int32 Index = 0;
    for (NSScreen* Screen in NSScreen.screens)
    {
        // This is the full resolution frame of the monitor
        const NSRect ScreenFrame = Screen.frame;
        
        // This is the frame of the monitor that is usable. I.e the full resolution frame excluding the top
        // menu-bar, and the dock-space.
        const NSRect ScreenVisibleFrame = Screen.visibleFrame;
        
        // Here we try and gather as much monitor information as possible and be consitent with the similar
        // information we can retrieve from the Win32 API in order to be consisitent across platforms.
        FMonitorInfo& MonitorInfo = OutMonitorInfo[Index++];
        MonitorInfo.DeviceName     = FindMonitorName(Screen);
        MonitorInfo.MainPosition   = FIntVector2(ScreenFrame.origin.x, ScreenFrame.origin.y);
        MonitorInfo.MainSize       = FIntVector2(ScreenFrame.size.width, ScreenFrame.size.height);
        MonitorInfo.WorkPosition   = FIntVector2(ScreenVisibleFrame.origin.x, ScreenVisibleFrame.origin.y);
        MonitorInfo.WorkSize       = FIntVector2(ScreenVisibleFrame.size.width, ScreenVisibleFrame.size.height);
        MonitorInfo.bIsPrimary     = MainScreen == Screen;
        MonitorInfo.DisplayDPI     = MonitorDPIFromScreen(Screen);
        MonitorInfo.DisplayScaling = Screen.backingScaleFactor;
    }
}

void FMacApplication::SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
{
    FGenericApplication::SetMessageHandler(InMessageHandler);
    
    if (InputDevice)
    {
        InputDevice->SetMessageHandler(InMessageHandler);
    }
}

FCocoaWindow* FMacApplication::FindNSWindowUnderCursor() const
{
    SCOPED_AUTORELEASE_POOL();
    
    const NSInteger WindowNumber = [NSWindow windowNumberAtPoint:[NSEvent mouseLocation] belowWindowWithWindowNumber:0];
    
    const NSWindow* Window = [NSApp windowWithWindowNumber:WindowNumber];
    if (!Window)
    {
        return nullptr;
    }
    
    // Only return the Window if it is a CocoaWindow
    return [Window isKindOfClass:[FCocoaWindow class]] ? reinterpret_cast<FCocoaWindow*>(Window) : nullptr;
}

TSharedRef<FMacWindow> FMacApplication::FindWindowFromNSWindow(NSWindow* Window) const
{
    if (!Window)
    {
        return nullptr;
    }
    
    if ([Window isKindOfClass:[FCocoaWindow class]])
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
    TScopedLock Lock(ClosedWindowsCS);
    
    if (!ClosedWindows.Contains(Window))
    {
        ClosedWindows.Emplace(Window);
    }
}

void FMacApplication::DeferEvent(NSObject* EventObject)
{
    SCOPED_AUTORELEASE_POOL();
    
    FCocoaWindow* NewWindowUnderCursor = FindNSWindowUnderCursor();
    if (WindowUnderCursor != NewWindowUnderCursor)
    {
        [WindowUnderCursor release];
        WindowUnderCursor = [NewWindowUnderCursor retain];
    }
    
    if (EventObject)
    {
        FDeferredMacEvent NewDeferredEvent;
        if ([EventObject isKindOfClass:[NSNotification class]])
        {
            NSNotification* Notification = reinterpret_cast<NSNotification*>(EventObject);
            NewDeferredEvent.NotificationName = [Notification.name retain];
            
            NSObject* NotificationObject = Notification.object;
            if ([NotificationObject isKindOfClass: [FCocoaWindow class]])
            {
                FCocoaWindow* EventWindow = reinterpret_cast<FCocoaWindow*>(NotificationObject);
                NewDeferredEvent.CocoaWindow = [EventWindow retain];
                NewDeferredEvent.Window      = FindWindowFromNSWindow(NewDeferredEvent.CocoaWindow);
            }
        }
        else if ([EventObject isKindOfClass:[NSEvent class]])
        {
            NSEvent* CurrentEvent = reinterpret_cast<NSEvent*>(EventObject);
            NewDeferredEvent.Event         = [CurrentEvent retain];
            NewDeferredEvent.EventType     = [CurrentEvent type];
            NewDeferredEvent.ModifierFlags = [CurrentEvent modifierFlags];
            
            NSWindow* Window = CurrentEvent.window;
            if ([Window isKindOfClass: [FCocoaWindow class]])
            {
                FCocoaWindow* EventWindow = reinterpret_cast<FCocoaWindow*>(Window);
                NewDeferredEvent.CocoaWindow = [EventWindow retain];
                NewDeferredEvent.Window      = FindWindowFromNSWindow(NewDeferredEvent.CocoaWindow);
            }
            
            // We have to be careful what events call certain functions, since invalid calls raises an exception
            // causing the deferred events to not be put into the deferred-events array, which means that the event
            // will not be processed properly and the events "disappear".
            switch(NewDeferredEvent.EventType)
            {
                case NSEventTypeKeyUp:
                case NSEventTypeKeyDown:
                {
                    NewDeferredEvent.KeyCode   = [CurrentEvent keyCode];
                    NewDeferredEvent.bIsRepeat = [CurrentEvent isARepeat];

                    const NSUInteger NumCharacters = [[CurrentEvent characters] length];
                    if (NumCharacters > 0)
                    {
                        const unichar FirstCharacter = [[CurrentEvent characters] characterAtIndex:0];
                        if (FirstCharacter != NSDeleteCharacter)
                        {
                            NewDeferredEvent.Character = static_cast<uint32>(FirstCharacter);
                        }
                    }
                
                    break;
                }

                case NSEventTypeLeftMouseUp:
                case NSEventTypeRightMouseUp:
                case NSEventTypeOtherMouseUp:
                case NSEventTypeLeftMouseDown:
                case NSEventTypeRightMouseDown:
                case NSEventTypeOtherMouseDown:
                {
                    NewDeferredEvent.ClickCount        = [CurrentEvent clickCount];
                    NewDeferredEvent.MouseButtonNumber = static_cast<int32>([CurrentEvent buttonNumber]);
                    break;
                }
                
                case NSEventTypeScrollWheel:
                {
                    NewDeferredEvent.ScrollPhase                = [CurrentEvent phase];
                    NewDeferredEvent.ScrollDelta                = FVector2([CurrentEvent scrollingDeltaX], [CurrentEvent scrollingDeltaY]);
                    NewDeferredEvent.bHasPreciseScrollingDeltas = [CurrentEvent hasPreciseScrollingDeltas];
                    break;
                }

                default:
                {
                    break;
                }
            }
        }
        
        TScopedLock Lock(DeferredEventsCS);
        DeferredEvents.Emplace(NewDeferredEvent);
    }
}

void FMacApplication::ProcessDeferredEvent(const FDeferredMacEvent& DeferredEvent)
{
    SCOPED_AUTORELEASE_POOL();
    
    if (DeferredEvent.NotificationName)
    {
        NSNotificationName NotificationName = DeferredEvent.NotificationName;
        if (NotificationName == NSWindowDidMoveNotification)
        {
            ProcessWindowMoved(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidResizeNotification)
        {
            ProcessWindowResized(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidMiniaturizeNotification)
        {
            ProcessWindowResized(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidMiniaturizeNotification)
        {
            ProcessWindowResized(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidEnterFullScreenNotification)
        {
            ProcessWindowResized(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidExitFullScreenNotification)
        {
            ProcessWindowResized(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidBecomeMainNotification)
        {
            MessageHandler->OnWindowFocusGained(DeferredEvent.Window);
        }
        else if (NotificationName == NSWindowDidResignMainNotification)
        {
            MessageHandler->OnWindowFocusLost(DeferredEvent.Window);
        }
        else if (NotificationName == NSApplicationDidChangeScreenParametersNotification)
        {
            MessageHandler->OnMonitorConfigurationChange();
        }
    }
    else if (DeferredEvent.Event)
    {
        switch(DeferredEvent.EventType)
        {
            case NSEventTypeFlagsChanged:
            {
                ProcessUpdatedModfierFlags(DeferredEvent);
                break;
            }
                
            case NSEventTypeKeyUp:
            case NSEventTypeKeyDown:
            {
                ProcessUpdatedModfierFlags(DeferredEvent);
                ProcessKeyEvent(DeferredEvent);
                break;
            }

            case NSEventTypeLeftMouseUp:
            case NSEventTypeRightMouseUp:
            case NSEventTypeOtherMouseUp:
            case NSEventTypeLeftMouseDown:
            case NSEventTypeRightMouseDown:
            case NSEventTypeOtherMouseDown:
            {
                ProcessUpdatedModfierFlags(DeferredEvent);
                ProcessMouseButtonEvent(DeferredEvent);
                break;
            }

            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged:
            case NSEventTypeOtherMouseDragged:
            case NSEventTypeRightMouseDragged:
            {
                ProcessUpdatedModfierFlags(DeferredEvent);
                ProcessMouseMoveEvent(DeferredEvent);
                break;
            }
               
            case NSEventTypeScrollWheel:
            {
                ProcessUpdatedModfierFlags(DeferredEvent);
                ProcessMouseScrollEvent(DeferredEvent);
                break;
            }

            case NSEventTypeMouseEntered:
            case NSEventTypeMouseExited:
            {
                ProcessMouseHoverEvent(DeferredEvent);
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

NSEvent* FMacApplication::OnNSEvent(NSEvent* Event)
{
    NSEvent* ReturnEvent = Event;
    DeferEvent(Event);
    
    switch(Event.type)
    {
        case NSEventTypeKeyDown:
        case NSEventTypeKeyUp:
            ReturnEvent = nullptr;
            break;
            
        default:
            break;
    }
    
    // If the event is returned it is continued to be sent down the responder change, and for events
    // that we want to stop sending we are returning nullptr.
    return ReturnEvent;
}

void FMacApplication::ProcessMouseMoveEvent(const FDeferredMacEvent&)
{
    const NSPoint MouseLocation  = [NSEvent mouseLocation];
    const NSPoint CursorPosition = ConvertCocoaPointToEngine(MouseLocation.x, MouseLocation.y);
    MacCursor->UpdateCursorPosition(FIntVector2(static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y)));

    MessageHandler->OnMouseMove(static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
}

void FMacApplication::ProcessMouseButtonEvent(const FDeferredMacEvent& DeferredEvent)
{
    // Convert the MouseButton into engine enum
    const EMouseButtonName::Type CurrentMouseButton = FPlatformInputMapper::GetButtonFromIndex(DeferredEvent.MouseButtonNumber);

    // MouseDown otherwise it is a MouseUp event
    if (DeferredEvent.EventType == NSEventTypeLeftMouseDown || DeferredEvent.EventType == NSEventTypeRightMouseDown || DeferredEvent.EventType == NSEventTypeOtherMouseDown)
    {
        constexpr uint32 DoubleClickEvenCount = 2;
        if (LastPressedButton == CurrentMouseButton && (DeferredEvent.ClickCount % DoubleClickEvenCount) == 0)
        {
            MessageHandler->OnMouseButtonDoubleClick(CurrentMouseButton, GetModifierKeyState());
        }
        else
        {
            MessageHandler->OnMouseButtonDown(DeferredEvent.Window, CurrentMouseButton, GetModifierKeyState());
        }

        // Save the mousebutton to handle double-click events
        LastPressedButton = CurrentMouseButton;
    }
    else
    {
        MessageHandler->OnMouseButtonUp(CurrentMouseButton, GetModifierKeyState());
    }
}

void FMacApplication::ProcessMouseScrollEvent(const FDeferredMacEvent& DeferredEvent)
{
    if (DeferredEvent.ScrollPhase != NSEventPhaseCancelled)
    {
        CGFloat ScrollDeltaX = DeferredEvent.ScrollDelta.X;
        CGFloat ScrollDeltaY = DeferredEvent.ScrollDelta.Y;
        if (DeferredEvent.bHasPreciseScrollingDeltas)
        {
            ScrollDeltaX *= 0.1;
            ScrollDeltaY *= 0.1;
        }
        
        if (FMath::Abs(ScrollDeltaX) > 0.0f)
        {
            MessageHandler->OnMouseScrolled(ScrollDeltaX, false);
        }
        if (FMath::Abs(ScrollDeltaY) > 0.0f)
        {
            MessageHandler->OnMouseScrolled(ScrollDeltaY, true);
        }
    }
}

void FMacApplication::ProcessMouseHoverEvent(const FDeferredMacEvent& DeferredEvent)
{
    if (DeferredEvent.Window)
    {
        if (DeferredEvent.EventType == NSEventTypeMouseEntered)
        {
            MessageHandler->OnMouseEntered();
        }
        else if (DeferredEvent.EventType == NSEventTypeMouseExited)
        {
            MessageHandler->OnMouseLeft();
        }
    }
}

void FMacApplication::ProcessKeyEvent(const FDeferredMacEvent& DeferredEvent)
{
    const EKeyboardKeyName::Type KeyName = FPlatformInputMapper::GetKeyCodeFromScanCode(DeferredEvent.KeyCode);
    if (DeferredEvent.EventType == NSEventTypeKeyDown)
    {
        // First notify about a key being down...
        MessageHandler->OnKeyDown(KeyName, DeferredEvent.bIsRepeat, GetModifierKeyState());
    
        // ... then send the character
        if (DeferredEvent.Character != uint32(-1))
        {
            MessageHandler->OnKeyChar(DeferredEvent.Character);
        }
    }
    else if (DeferredEvent.EventType == NSEventTypeKeyUp)
    {
        MessageHandler->OnKeyUp(KeyName, GetModifierKeyState());
    }
}

void FMacApplication::ProcessUpdatedModfierFlags(const FDeferredMacEvent& DeferredEvent)
{
    // NSUinteger seems to be defined as a unsigned long, which would be equal to a uint64 on macOS
    const uint64 ModifierFlags = DeferredEvent.ModifierFlags;
    if (ModifierFlags != CurrentModifierFlags)
    {
        ProcessModfierKey(MacModifierKey_LeftControl, ModifierFlags);
        ProcessModfierKey(MacModifierKey_RightControl, ModifierFlags);
        ProcessModfierKey(MacModifierKey_LeftShift, ModifierFlags);
        ProcessModfierKey(MacModifierKey_RightShift, ModifierFlags);
        ProcessModfierKey(MacModifierKey_LeftCommand, ModifierFlags);
        ProcessModfierKey(MacModifierKey_RightCommand, ModifierFlags);
        ProcessModfierKey(MacModifierKey_LeftAlt, ModifierFlags);
        ProcessModfierKey(MacModifierKey_RightAlt, ModifierFlags);
        ProcessModfierKey(MacModifierKey_CapsLock, ModifierFlags);
        ProcessModfierKey(MacModifierKey_NumLock, ModifierFlags);
        
        // Save the modifier flag so that we can change what is changed
        CurrentModifierFlags = ModifierFlags;
    }
}

#include <IOKit/hidsystem/ev_keymap.h>

void FMacApplication::ProcessModfierKey(EMacModifierKey MacModifierKey, uint64 ModifierKeyFlags)
{
    // Quick access to the modifer key masks. The values for these can be found inside the IOKit/hidsystem/ev_keymap.h
    // header but we have redefined them here to avoid including IOKit.
    static constexpr uint64 ModifierKeyMask[] =
    {
        0x00000001, // LeftCtrl
        0x00002000, // RightCtrl

        0x00000002, // LeftShift
        0x00000004, // RightShift

        0x00000008, // LeftCmd
        0x00000010, // RightCmd

        0x00000020, // LeftAlt
        0x00000040, // RightAlt
        
        0x00010000, // CapsLock
        // TODO: NumLock
    };

    // Quick access to the keyboard names for the modifier keys
    static constexpr EKeyboardKeyName::Type KeyBoardNames[] =
    {
        EKeyboardKeyName::LeftControl,
        EKeyboardKeyName::RightControl,

        EKeyboardKeyName::LeftShift,
        EKeyboardKeyName::RightShift,

        EKeyboardKeyName::LeftSuper,
        EKeyboardKeyName::RightSuper,

        EKeyboardKeyName::LeftAlt,
        EKeyboardKeyName::RightAlt,
        
        EKeyboardKeyName::CapsLock,
        // TODO: NumLock
    };

    // Ensure that the modifier key is within the allowed range
    CHECK(MacModifierKey >= MacModifierKey_LeftControl && MacModifierKey <= MacModifierKey_NumLock);
    
    // Retrieve the key-name
    const EKeyboardKeyName::Type KeyName = KeyBoardNames[MacModifierKey];

    // Retrieve the key-mask
    const uint64 KeyFlag = ModifierKeyMask[MacModifierKey];

    const bool bIsPressed     = (KeyFlag & ModifierKeyFlags)     != 0;
    const bool bIsPrevPressed = (KeyFlag & CurrentModifierFlags) != 0;

    if (bIsPressed)
    {
        bool bIsRepeat = false;
        if (bIsPrevPressed)
        {
            bIsRepeat = true;
        }
        
        MessageHandler->OnKeyDown(KeyName, bIsRepeat, GetModifierKeyState());
    }
    else
    {
        if (bIsPrevPressed)
        {
            // Modifier is currently NOT down, if the key was down previously, we send a key up event
            MessageHandler->OnKeyUp(KeyName, GetModifierKeyState());
        }
    }
}

void FMacApplication::ProcessWindowResized(const FDeferredMacEvent& DeferredEvent)
{
    // Start by giving other systems a chance to prepare for a window-resize
    MessageHandler->OnWindowResizing(DeferredEvent.Window);
 
    // When entering fullscreen the window size is the full frame
    NSRect ContentFrame;
    if (DeferredEvent.NotificationName == NSWindowDidEnterFullScreenNotification)
    {
        ContentFrame = [DeferredEvent.CocoaWindow frame];
    }
    else
    {
        ContentFrame = [DeferredEvent.CocoaWindow contentRectForFrameRect:DeferredEvent.CocoaWindow.frame];
    }

    // Convert the coordinates to the generic ones that are expected
    ContentFrame = FMacApplication::ConvertEngineRectToCocoa(ContentFrame.size.width, ContentFrame.size.height, ContentFrame.origin.x, ContentFrame.origin.y);

    // Window can move sometimes when resized so send and event about it
    const int32 PositionX = static_cast<int32>(ContentFrame.origin.x);
    const int32 PositionY = static_cast<int32>(ContentFrame.origin.y);
    
    const FIntVector2 CachedPosition = DeferredEvent.Window->GetCachedPosition();
    if (CachedPosition.X != PositionX || CachedPosition.Y != PositionY)
    {
        MessageHandler->OnWindowMoved(DeferredEvent.Window, PositionX, PositionY);
        DeferredEvent.Window->SetCachedPosition(FIntVector2(PositionX, PositionY));
    }
    
    MessageHandler->OnWindowResized(DeferredEvent.Window, uint32(ContentFrame.size.width), uint32(ContentFrame.size.height));
}

void FMacApplication::ProcessWindowMoved(const FDeferredMacEvent& DeferredEvent)
{
    // We always retrieve the contentRect in order to find out where the position are
    NSRect ContentFrame = [DeferredEvent.CocoaWindow contentRectForFrameRect:DeferredEvent.CocoaWindow.frame];
    ContentFrame = FMacApplication::ConvertEngineRectToCocoa(ContentFrame.size.width, ContentFrame.size.height, ContentFrame.origin.x, ContentFrame.origin.y);
    
    const int32 PositionX = static_cast<int32>(ContentFrame.origin.x);
    const int32 PositionY = static_cast<int32>(ContentFrame.origin.y);
    
    const FIntVector2 CachedPosition = DeferredEvent.Window->GetCachedPosition();
    if (CachedPosition.X != PositionX || CachedPosition.Y != PositionY)
    {
        MessageHandler->OnWindowMoved(DeferredEvent.Window, PositionX, PositionY);
        DeferredEvent.Window->SetCachedPosition(FIntVector2(PositionX, PositionY));
    }
}

void FMacApplication::OnWindowDestroyed(const TSharedRef<FMacWindow>& Window)
{
    // Schedule the removal and deletion of the CocoaWindow, the window will later actually be released
    // during the next call to FMacApplication::Tick.
    FCocoaWindow* CocoaWindow = Window->GetCocoaWindow();
    if (CocoaWindow && !ClosedCocoaWindows.Contains(CocoaWindow))
    {
        TScopedLock Lock(ClosedCocoaWindowsCS);
        ClosedCocoaWindows.Add(CocoaWindow);
    }
    
    // Remove the MacWindow
    TScopedLock Lock(ClosedCocoaWindowsCS);
    Windows.Remove(Window);
}

void FMacApplication::OnWindowWillResize(const TSharedRef<FMacWindow>& Window)
{
    // This callback allows other engine systems (Mainly the FApplicationInterface) to be notifies when a
    // window is about to be resized. This can for example be when we want to wait for the GPU to finish
    // rendering before we resize the window.
    MessageHandler->OnWindowResizing(Window);
}

FString FMacApplication::FindMonitorName(NSScreen* Screen)
{
    if (!Screen)
    {
        return "Unknown Display";
    }
    
    // If the localizedName is available (macOS 10.15 and above) then call that
    if ([Screen respondsToSelector:@selector(localizedName)])
    {
        NSString* MonitorName = [Screen valueForKey:@"localizedName"];
        if (MonitorName)
        {
            return MonitorName;
        }
    }

    // Retrieve the displayID from the NSScreen
    CGDirectDisplayID DisplayID = static_cast<CGDirectDisplayID>([[[Screen deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue]);
    
    io_iterator_t Iterator;
    if (IOServiceGetMatchingServices(MACH_PORT_NULL, IOServiceMatching("IODisplayConnect"), &Iterator) != 0)
    {
        return "Unknown Display";
    }
    
    io_service_t    Service;
    CFDictionaryRef DisplayInfo;
    while ((Service = IOIteratorNext(Iterator)) != 0)
    {
        DisplayInfo = IODisplayCreateInfoDictionary(Service, kIODisplayOnlyPreferredName);
        
        CFNumberRef VendorIDRef  = (CFNumberRef)CFDictionaryGetValue(DisplayInfo, CFSTR(kDisplayVendorID));
        CFNumberRef ProductIDRef = (CFNumberRef)CFDictionaryGetValue(DisplayInfo, CFSTR(kDisplayProductID));
        if (!VendorIDRef || !ProductIDRef)
        {
            CFRelease(DisplayInfo);
            continue;
        }
        
        uint32 VendorID;
        uint32 ProductID;
        CFNumberGetValue(VendorIDRef, kCFNumberIntType, &VendorID);
        CFNumberGetValue(ProductIDRef, kCFNumberIntType, &ProductID);
        
        if (CGDisplayVendorNumber(DisplayID) == VendorID && CGDisplayModelNumber(DisplayID) == ProductID)
        {
            break;
        }
        
        CFRelease(DisplayInfo);
    }
    
    IOObjectRelease(Iterator);
    if (!Service)
    {
        return "Unknown Display";
    }

    CFDictionaryRef Names = (CFDictionaryRef)CFDictionaryGetValue(DisplayInfo, CFSTR(kDisplayProductName));
    
    CFStringRef NameRef;
    if (!Names || !CFDictionaryGetValueIfPresent(Names, CFSTR("en_US"), reinterpret_cast<const void**>(&NameRef)))
    {
        CFRelease(DisplayInfo);
        return "Unknown Display";
    }
    
    NSString* MonitorName = (__bridge NSString*)NameRef;
    
    // Store the string name, since we need to release the original string (via the DisplayInfo) before we return
    FString Result(MonitorName);
    
    // Release DisplayInfo
    CFRelease(DisplayInfo);

    // Finally return the result
    return Result;
}

uint32 FMacApplication::MonitorDPIFromScreen(NSScreen* Screen)
{
    const float BackingScaleFactor = [Screen backingScaleFactor];
    
    // Retrieve the pixel dimensions of the screen
    const NSRect Frame = [Screen frame];
    
    CGFloat PixelWidth  = CGRectGetWidth(Frame) * BackingScaleFactor;
    CGFloat PixelHeight = CGRectGetHeight(Frame) * BackingScaleFactor;

    // Retrieve the physical dimensions of the screen in millimeters
    const CGFloat InchToMillimeterFactor = 25.4;
    CGFloat PhysicalWidth  = CGRectGetWidth(Frame) * InchToMillimeterFactor / Frame.size.width;
    CGFloat PhysicalHeight = CGRectGetHeight(Frame) * InchToMillimeterFactor / Frame.size.height;

    // Calculate the DPI
    const CGFloat ScreenWidthDPI  = PixelWidth / PhysicalWidth;
    const CGFloat ScreenHeightDPI = PixelHeight / PhysicalHeight;

    // Use the average of width and height DPI values
    CGFloat ScreenDPI = (ScreenWidthDPI + ScreenHeightDPI) / 2.0;

    // Round and convert to uint32_t
    const uint32 RoundedDPI = static_cast<uint32>(FMath::RoundToInt(ScreenDPI));
    return RoundedDPI;
}

NSScreen* FMacApplication::FindScreenFromCocoaPoint(CGFloat PositionX, CGFloat PositionY)
{
    NSPoint Position = NSMakePoint(PositionX, PositionY);
    NSArray* ScreensArray = [NSScreen screens];
    
    // Find the screen that contains the point
    NSScreen* Screen = nil;
    for (NSScreen* CurrentScreen in ScreensArray)
    {
        NSRect ScreenFrame = [CurrentScreen frame];
        if (NSPointInRect(Position, ScreenFrame))
        {
            Screen = CurrentScreen;
            break;
        }
    }

    // If no screen contains the point, default to the main screen
    if (!Screen)
    {
        Screen = [NSScreen mainScreen];
    }
    
    return Screen;
}

NSScreen* FMacApplication::FindScreenFromEnginePoint(CGFloat PositionX, CGFloat PositionY)
{
    NSArray* ScreensArray = [NSScreen screens];
    
    // Since EngineX and EngineY are relative to the screen's top-left corner, we need to find the
    // screen that matches these coordinates.
    NSScreen* Screen = nil;
    for (NSScreen* CurrentScreen in ScreensArray)
    {
        NSRect ScreenFrame = [CurrentScreen frame];
        
        // Screen's size
        CGFloat ScreenWidth  = ScreenFrame.size.width;
        CGFloat ScreenHeight = ScreenFrame.size.height;
        
        // Check if the engine point falls within this screen's bounds
        if (PositionX >= 0 && PositionX <= ScreenWidth && PositionY >= 0 && PositionY <= ScreenHeight)
        {
            Screen = CurrentScreen;
            break;
        }
    }
    
    // If no screen is found, default to the main screen
    if (!Screen)
    {
        Screen = [NSScreen mainScreen];
    }
    
    return Screen;
}

NSPoint FMacApplication::ConvertCocoaPointToEngine(CGFloat PositionX, CGFloat PositionY)
{
    NSScreen* Screen = FindScreenFromCocoaPoint(PositionX, PositionY);
        
    // Adjust the point's coordinates relative to the screen (in points)
    const NSRect ScreenFrame = [Screen frame];
    CGFloat RelativeX = PositionX - ScreenFrame.origin.x;
    CGFloat RelativeY = PositionY - ScreenFrame.origin.y;
    
    // Convert the Y-coordinate from Cocoa (bottom-left origin) to engine (top-left origin)
    CGFloat ConvertedY = ScreenFrame.size.height - RelativeY;
    
    // Create the converted point in pixels
    NSPoint ConvertedPoint = NSMakePoint(RelativeX, ConvertedY);
    return ConvertedPoint;
}

NSPoint FMacApplication::ConvertEnginePointToCocoa(CGFloat PositionX, CGFloat PositionY)
{
    NSScreen* Screen = FindScreenFromEnginePoint(PositionX, PositionY);
    
    // Convert the engine point to Cocoa's coordinate system
    const NSRect ScreenFrame = [Screen frame];
    CGFloat RelativeX = ScreenFrame.origin.x + PositionX;
    CGFloat RelativeY = ScreenFrame.origin.y + (ScreenFrame.size.height - PositionY);
    
    // Create the converted point
    NSPoint CocoaPoint = NSMakePoint(RelativeX, RelativeY);
    return CocoaPoint;
}

NSRect FMacApplication::ConvertEngineRectToCocoa(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY)
{
    const NSPoint Position = ConvertEnginePointToCocoa(PositionX, PositionY);
    return NSMakeRect(Position.x, Position.y - Height + 1.0f, Width, Height);
}

NSRect FMacApplication::ConvertCocoaRectToEngine(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY)
{
    const NSPoint Position = ConvertCocoaPointToEngine(PositionX, PositionY);
    return NSMakeRect(Position.x, Position.y - Height + 1.0f, Width, Height);
}
