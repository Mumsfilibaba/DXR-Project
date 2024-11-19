#include "MacApplication.h"
#include "MacWindow.h"
#include "MacCursor.h"
#include "CocoaWindow.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Mac/MacRunLoop.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Threading/ScopedLock.h"
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
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

- (void)onApplicationBecomeInactive:(NSNotification*)InNotification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

- (void)displaysDidChange:(NSNotification*)InNotification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(InNotification);
    }
}

@end

FMacApplication* MacApplication = nullptr;

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
    , PreviousModifierFlags(0)
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
    if (!MacApplication)
    {
        MacApplication = this;
    }
    
    ExecuteOnMainThread(^
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
    ExecuteOnMainThread(^
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

    if (this == MacApplication)
    {
        MacApplication = nullptr;
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

    FPlatformApplicationMisc::PumpMessages(true);

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
        ExecuteOnMainThread(^
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

void FMacApplication::SetActiveWindow(const TSharedRef<FGenericWindow>& Window)
{
    __block TSharedRef<FMacWindow> MacWindow = StaticCastSharedRef<FMacWindow>(Window);
    ExecuteOnMainThread(^
    {
        FCocoaWindow* CocoaWindow = MacWindow->GetCocoaWindow();
        [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
    }, NSDefaultRunLoopMode, false);
}

TSharedRef<FGenericWindow> FMacApplication::GetActiveWindow() const
{
    NSWindow* KeyWindow = ExecuteOnMainThreadAndReturn(^
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
        if ([EventObject isKindOfClass:[NSEvent class]])
        {
            NSEvent* Event = reinterpret_cast<NSEvent*>(EventObject);
            NewDeferredEvent.Event = [Event retain];
            
            NSWindow* Window = Event.window;
            if ([Window isKindOfClass: [FCocoaWindow class]])
            {
                FCocoaWindow* EventWindow = reinterpret_cast<FCocoaWindow*>(Window);
                NewDeferredEvent.Window   = [EventWindow retain];
            }
        }
        else if ([EventObject isKindOfClass:[NSNotification class]])
        {
            NSNotification* Notification = reinterpret_cast<NSNotification*>(EventObject);
            NewDeferredEvent.NotificationName = [Notification.name retain];
            
            NSObject* NotificationObject = Notification.object;
            if ([NotificationObject isKindOfClass: [FCocoaWindow class]])
            {
                FCocoaWindow* EventWindow = reinterpret_cast<FCocoaWindow*>(NotificationObject);
                NewDeferredEvent.Window   = [EventWindow retain];
            }
        }
        else if ([EventObject isKindOfClass:[NSString class]])
        {
            NSString* Characters = reinterpret_cast<NSString*>(EventObject);
            
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
        
        TScopedLock Lock(DeferredEventsCS);
        DeferredEvents.Emplace(NewDeferredEvent);
    }
}

void FMacApplication::ProcessDeferredEvent(const FDeferredMacEvent& DeferredEvent)
{
    SCOPED_AUTORELEASE_POOL();
    
    TSharedRef<FMacWindow> Window = FindWindowFromNSWindow(DeferredEvent.Window);
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
            MessageHandler->OnWindowFocusGained(Window);
        }
        else if (NotificationName == NSWindowDidResignMainNotification)
        {
            MessageHandler->OnWindowFocusLost(Window);
        }
        else if (NotificationName == NSApplicationDidChangeScreenParametersNotification)
        {
            MessageHandler->OnMonitorConfigurationChange();
        }
    }
    else if (DeferredEvent.Event)
    {
        switch(DeferredEvent.Event.type)
        {
            case NSEventTypeFlagsChanged:
            case NSEventTypeKeyUp:
            case NSEventTypeKeyDown:
            {
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
                ProcessMouseButtonEvent(DeferredEvent);
                break;
            }

            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged:
            case NSEventTypeOtherMouseDragged:
            case NSEventTypeRightMouseDragged:
            {
                ProcessMouseMoveEvent(DeferredEvent);
                break;
            }
               
            case NSEventTypeScrollWheel:
            {
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
    else if (DeferredEvent.Character != uint32(-1))
    {
        MessageHandler->OnKeyChar(DeferredEvent.Character);
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
            ReturnEvent = nil;
            break;
            
        default:
            break;
    }
    
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
    const EMouseButtonName::Type CurrentMouseButton = FPlatformInputMapper::GetButtonFromIndex(static_cast<int32>(DeferredEvent.Event.buttonNumber));
    if (DeferredEvent.Event.type == NSEventTypeLeftMouseDown ||
        DeferredEvent.Event.type == NSEventTypeRightMouseDown ||
        DeferredEvent.Event.type == NSEventTypeOtherMouseDown)
    {
        if (LastPressedButton == CurrentMouseButton && DeferredEvent.Event.clickCount % 2 == 0)
        {
            MessageHandler->OnMouseButtonDoubleClick(CurrentMouseButton, FPlatformApplicationMisc::GetModifierKeyState());
        }
        else
        {
            TSharedRef<FMacWindow> Window = FindWindowFromNSWindow(DeferredEvent.Window);
            MessageHandler->OnMouseButtonDown(Window, CurrentMouseButton, FPlatformApplicationMisc::GetModifierKeyState());
        }
        
        // Save the mousebutton to handle double-click events
        LastPressedButton = CurrentMouseButton;
    }
    else
    {
        MessageHandler->OnMouseButtonUp(CurrentMouseButton, FPlatformApplicationMisc::GetModifierKeyState());
    }
}

void FMacApplication::ProcessMouseScrollEvent(const FDeferredMacEvent& DeferredEvent)
{
    if (DeferredEvent.Event.phase != NSEventPhaseCancelled)
    {
        CGFloat ScrollDeltaX = DeferredEvent.Event.scrollingDeltaX;
        CGFloat ScrollDeltaY = DeferredEvent.Event.scrollingDeltaY;
        if (DeferredEvent.Event.hasPreciseScrollingDeltas)
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
    if (TSharedRef<FMacWindow> Window = FindWindowFromNSWindow(DeferredEvent.Event.window))
    {
        if (DeferredEvent.Event.type == NSEventTypeMouseEntered)
        {
            MessageHandler->OnMouseEntered();
        }
        else if (DeferredEvent.Event.type == NSEventTypeMouseExited)
        {
            MessageHandler->OnMouseLeft();
        }
    }
}

void FMacApplication::ProcessKeyEvent(const FDeferredMacEvent& DeferredEvent)
{
    const EKeyboardKeyName::Type KeyName = FPlatformInputMapper::GetKeyCodeFromScanCode(DeferredEvent.Event.keyCode);
    
    bool bIsRepeat  = false;
    bool bIsKeyDown = false;
    
    if (DeferredEvent.Event.type == NSEventTypeKeyDown)
    {
        bIsRepeat  = DeferredEvent.Event.ARepeat;
        bIsKeyDown = true;
    }
    else if (DeferredEvent.Event.type == NSEventTypeFlagsChanged)
    {
        const NSUInteger KeyFlag       = FMacInputMapper::TranslateKeyToModifierFlag(KeyName);
        const NSUInteger ModifierFlags = [DeferredEvent.Event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
        
        if (KeyFlag & ModifierFlags)
        {
            if (KeyFlag & PreviousModifierFlags)
            {
                bIsKeyDown = false;
            }
            else
            {
                bIsKeyDown = true;
            }
        }
        else
        {
            bIsKeyDown = false;
        }
        
        // Save the modifier flag so that we can change what is changed
        PreviousModifierFlags = ModifierFlags;
    }

    if (bIsKeyDown)
    {
        MessageHandler->OnKeyDown(KeyName, bIsRepeat, FPlatformApplicationMisc::GetModifierKeyState());
    }
    else
    {
        MessageHandler->OnKeyUp(KeyName, FPlatformApplicationMisc::GetModifierKeyState());
    }
}

void FMacApplication::ProcessWindowResized(const FDeferredMacEvent& DeferredEvent)
{
    TSharedRef<FMacWindow> Window = FindWindowFromNSWindow(DeferredEvent.Window);
    
    // Start by giving other systems a chance to prepare for a window-resize
    MessageHandler->OnWindowResizing(Window);
 
    // When entering fullscreen the window size is the full frame
    NSRect ContentFrame;
    if (DeferredEvent.NotificationName == NSWindowDidEnterFullScreenNotification)
    {
        ContentFrame = [DeferredEvent.Window frame];
    }
    else
    {
        ContentFrame = [DeferredEvent.Window contentRectForFrameRect:DeferredEvent.Window.frame];
    }

    // Convert the coordinates to the generic ones that are expected
    ContentFrame = FMacApplication::ConvertEngineRectToCocoa(ContentFrame.size.width, ContentFrame.size.height, ContentFrame.origin.x, ContentFrame.origin.y);

    // Window can move sometimes when resized so send and event about it
    const int32 PositionX = static_cast<int32>(ContentFrame.origin.x);
    const int32 PositionY = static_cast<int32>(ContentFrame.origin.y);
    
    const FIntVector2 CachedPosition = Window->GetCachedPosition();
    if (CachedPosition.x != PositionX || CachedPosition.y != PositionY)
    {
        MessageHandler->OnWindowMoved(Window, PositionX, PositionY);
        Window->SetCachedPosition(FIntVector2(PositionX, PositionY));
    }
    
    MessageHandler->OnWindowResized(Window, uint32(ContentFrame.size.width), uint32(ContentFrame.size.height));
}

void FMacApplication::ProcessWindowMoved(const FDeferredMacEvent& DeferredEvent)
{
    TSharedRef<FMacWindow> Window = FindWindowFromNSWindow(DeferredEvent.Window);
    
    // We always retrieve the contentRect in order to find out where the position are
    NSRect ContentFrame = [DeferredEvent.Window contentRectForFrameRect:DeferredEvent.Window.frame];
    ContentFrame = FMacApplication::ConvertEngineRectToCocoa(ContentFrame.size.width, ContentFrame.size.height, ContentFrame.origin.x, ContentFrame.origin.y);
    
    const int32 PositionX = static_cast<int32>(ContentFrame.origin.x);
    const int32 PositionY = static_cast<int32>(ContentFrame.origin.y);
    
    const FIntVector2 CachedPosition = Window->GetCachedPosition();
    if (CachedPosition.x != PositionX || CachedPosition.y != PositionY)
    {
        MessageHandler->OnWindowMoved(Window, PositionX, PositionY);
        Window->SetCachedPosition(FIntVector2(PositionX, PositionY));
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
    const uint32 RoundedDPI = static_cast<uint32>(FMath::Round(ScreenDPI));
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
