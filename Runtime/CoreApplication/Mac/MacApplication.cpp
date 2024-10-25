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

FString FMacApplication::GetMonitorNameFromNSScreen(NSScreen* Screen)
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

uint32 FMacApplication::GetDPIFromNSScreen(NSScreen* Screen)
{
    const NSRect Frame = [Screen frame];
    const float  BackingScaleFactor = [Screen backingScaleFactor];
    
    // Retrieve the pixel dimensions of the screen
    CGFloat PixelWidth  = CGRectGetWidth(Frame) * BackingScaleFactor;
    CGFloat PixelHeight = CGRectGetHeight(Frame) * BackingScaleFactor;

    // Retrieve the physical dimensions of the screen in millimeters
    const CGFloat InchToMillimeterFactor = 25.4;
    CGFloat PhysicalWidth  = CGRectGetWidth(Frame) * InchToMillimeterFactor / Frame.size.width;
    CGFloat PhysicalHeight = CGRectGetHeight(Frame) * InchToMillimeterFactor / Frame.size.height;

    // Calculate the DPI
    CGFloat DpiWidth  = PixelWidth / PhysicalWidth;
    CGFloat DpiHeight = PixelHeight / PhysicalHeight;

    // Use the average of width and height DPI values
    CGFloat Dpi = (DpiWidth + DpiHeight) / 2.0;

    // Round and convert to uint32_t
    uint32 RoundedDPI = static_cast<uint32>(FMath::Round(Dpi));
    return RoundedDPI;
}

NSPoint FMacApplication::GetCorrectedMouseLocation()
{
    NSScreen* Screen = [NSScreen mainScreen];
    NSPoint MouseLocation = [NSEvent mouseLocation];
    MouseLocation.y = Screen.frame.size.height - MouseLocation.y;
    return MouseLocation;
}

FMacApplication* MacApplication = nullptr;

TSharedPtr<FGenericApplication> FMacApplication::Create()
{
    return FMacApplication::CreateMacApplication();
}

TSharedPtr<FMacApplication> FMacApplication::CreateMacApplication()
{
    // Create a new instance and set the the global instance
    TSharedPtr<FMacApplication> NewMacApplication = MakeSharedPtr<FMacApplication>();
    MacApplication = NewMacApplication.Get();
    return NewMacApplication;
}

FMacApplication::FMacApplication()
    : FGenericApplication(MakeSharedPtr<FMacCursor>())
    , Observer(nullptr)
    , PreviousModifierFlags(0)
    , InputDevice(FGCInputDevice::CreateGCInputDevice())
    , LastPressedButton(EMouseButtonName::Unknown)
    , Windows()
    , WindowsCS()
    , ClosedWindows()
    , ClosedWindowsCS()
    , DeferredEvents()
    , DeferredEventsCS()
{
    ExecuteOnMainThread(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        // This should only be initialized from the main thread, but assert just to be sure.
        CHECK(FPlatformThreadMisc::IsMainThread());

        FPlatformInputMapper::Initialize();
        
        // Init the default macOS menu
        NSMenu*     MenuBar     = [NSMenu new];
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
        [[NSNotificationCenter defaultCenter] addObserver:Observer selector:@selector(onApplicationBecomeActive:) name:NSApplicationDidBecomeActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:Observer selector:@selector(onApplicationBecomeInactive:) name:NSApplicationDidResignActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:Observer selector:@selector(displaysDidChange:) name:NSApplicationDidChangeScreenParametersNotification object:nil];
        
        FPlatformApplicationMisc::PumpMessages(true);
    }, NSDefaultRunLoopMode, true);
}

FMacApplication::~FMacApplication()
{
    @autoreleasepool
    {
        [[NSNotificationCenter defaultCenter] removeObserver:Observer name:NSApplicationDidBecomeActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] removeObserver:Observer name:NSApplicationDidResignActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] removeObserver:Observer name:NSApplicationDidChangeScreenParametersNotification object:nil];

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
    TSharedRef<FMacWindow> NewWindow = FMacWindow::Create(this);
    
    TScopedLock Lock(WindowsCS);
    Windows.Emplace(NewWindow);
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

void FMacApplication::UpdateInputDevices()
{
    if (InputDevice)
    {
        InputDevice->UpdateDeviceState();
    }
}

FInputDevice* FMacApplication::GetInputDeviceInterface()
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
    
    return GetWindowFromNSWindow(KeyWindow);
}

TSharedRef<FGenericWindow> FMacApplication::GetWindowUnderCursor() const
{
    NSWindow* WindowUnderCursor = ExecuteOnMainThreadAndReturn(^
    {
        SCOPED_AUTORELEASE_POOL();
        const NSInteger WindowNumber = [NSWindow windowNumberAtPoint:[NSEvent mouseLocation] belowWindowWithWindowNumber:0];
        return [NSApp windowWithWindowNumber:WindowNumber];
    }, NSDefaultRunLoopMode);
    
    return GetWindowFromNSWindow(WindowUnderCursor);
}

void FMacApplication::QueryDisplayInfo(FDisplayInfo& OutDisplayInfo) const
{   
    for (NSScreen* Screen in NSScreen.screens)
    {
        const NSRect ScreenFrame        = Screen.frame;
        const NSRect ScreenVisibleFrame = Screen.visibleFrame;
        
        FMonitorInfo NewMonitorInfo;
        NewMonitorInfo.DeviceName     = GetMonitorNameFromNSScreen(Screen);
        NewMonitorInfo.MainPosition   = FIntVector2(ScreenFrame.origin.x, ScreenFrame.origin.y);
        NewMonitorInfo.MainSize       = FIntVector2(ScreenFrame.size.width, ScreenFrame.size.height);
        NewMonitorInfo.WorkPosition   = FIntVector2(ScreenVisibleFrame.origin.x, ScreenVisibleFrame.origin.y);
        NewMonitorInfo.WorkSize       = FIntVector2(ScreenVisibleFrame.size.width, ScreenVisibleFrame.size.height);
        NewMonitorInfo.bIsPrimary     = [NSScreen mainScreen] == Screen;
        NewMonitorInfo.DisplayDPI     = GetDPIFromNSScreen(Screen);
        NewMonitorInfo.DisplayScaling = Screen.backingScaleFactor;
        
        // Convert the backingScaleFactor to a value similar to the one in windows that can be retreived via the 'GetScaleFactorForMonitor' function
        NewMonitorInfo.DisplayScaleFactor = static_cast<uint32>(FMath::Round(NewMonitorInfo.DisplayScaling * 100.0f));

        if (NewMonitorInfo.bIsPrimary)
        {
            OutDisplayInfo.PrimaryDisplayWidth  = NewMonitorInfo.MainSize.x;
            OutDisplayInfo.PrimaryDisplayHeight = NewMonitorInfo.MainSize.y;
        }

        OutDisplayInfo.MonitorInfos.Add(NewMonitorInfo);
    }
    
    // Ensure that we don't waste any space
    OutDisplayInfo.MonitorInfos.Shrink();
}

void FMacApplication::SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
{
    FGenericApplication::SetMessageHandler(InMessageHandler);
    
    if (InputDevice)
    {
        InputDevice->SetMessageHandler(InMessageHandler);
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

void FMacApplication::DeferEvent(NSObject* EventObject)
{
    SCOPED_AUTORELEASE_POOL();
    
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
    
    TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(DeferredEvent.Window);
    if (DeferredEvent.NotificationName)
    {
        NSNotificationName NotificationName = DeferredEvent.NotificationName;
        if (NotificationName == NSWindowDidMoveNotification)
        {
            OnWindowMoved(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidResizeNotification)
        {
            OnWindowResized(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidMiniaturizeNotification)
        {
            OnWindowResized(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidMiniaturizeNotification)
        {
            OnWindowResized(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidEnterFullScreenNotification)
        {
            OnWindowResized(DeferredEvent);
        }
        else if (NotificationName == NSWindowDidExitFullScreenNotification)
        {
            OnWindowResized(DeferredEvent);
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
                OnKeyEvent(DeferredEvent);
                break;
            }
                
            case NSEventTypeLeftMouseUp:
            case NSEventTypeRightMouseUp:
            case NSEventTypeOtherMouseUp:
            case NSEventTypeLeftMouseDown:
            case NSEventTypeRightMouseDown:
            case NSEventTypeOtherMouseDown:
            {
                OnMouseButtonEvent(DeferredEvent);
                break;
            }

            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged:
            case NSEventTypeOtherMouseDragged:
            case NSEventTypeRightMouseDragged:
            {
                OnMouseMoveEvent();
                break;
            }
               
            case NSEventTypeScrollWheel:
            {
                OnMouseScrollEvent(DeferredEvent);
                break;
            }

            case NSEventTypeMouseEntered:
            {
                TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(DeferredEvent.Event.window);
                if (Window)
                {
                    MessageHandler->OnMouseEntered(Window);
                }
                break;
            }

            case NSEventTypeMouseExited:
            {
                TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(DeferredEvent.Event.window);
                if (Window)
                {
                    MessageHandler->OnMouseLeft(Window);
                }

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

void FMacApplication::OnMouseMoveEvent()
{
    const NSPoint CursorPosition = GetCorrectedMouseLocation();
    MessageHandler->OnMouseMove(static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
}

void FMacApplication::OnMouseButtonEvent(const FDeferredMacEvent& DeferredEvent)
{
    const NSPoint CursorPosition = GetCorrectedMouseLocation();
    const EMouseButtonName::Type CurrentMouseButton = FPlatformInputMapper::GetButtonFromIndex(static_cast<int32>(DeferredEvent.Event.buttonNumber));

    if (DeferredEvent.Event.type == NSEventTypeLeftMouseDown ||
        DeferredEvent.Event.type == NSEventTypeRightMouseDown ||
        DeferredEvent.Event.type == NSEventTypeOtherMouseDown)
    {
        TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(DeferredEvent.Window);
        if (LastPressedButton == CurrentMouseButton && DeferredEvent.Event.clickCount % 2 == 0)
        {
            MessageHandler->OnMouseButtonDoubleClick(Window, CurrentMouseButton, FPlatformApplicationMisc::GetModifierKeyState(), static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
        }
        else
        {
            MessageHandler->OnMouseButtonDown(Window, CurrentMouseButton, FPlatformApplicationMisc::GetModifierKeyState(), static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
        }
        
        // Save the mousebutton to handle double-click events
        LastPressedButton = CurrentMouseButton;
    }
    else
    {
        MessageHandler->OnMouseButtonUp(CurrentMouseButton, FPlatformApplicationMisc::GetModifierKeyState(), static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
    }
}

void FMacApplication::OnMouseScrollEvent(const FDeferredMacEvent& DeferredEvent)
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
        
        const NSPoint CursorPosition = GetCorrectedMouseLocation();
        if (FMath::Abs(ScrollDeltaX) > 0.0f)
        {
            MessageHandler->OnMouseScrolled(ScrollDeltaX, false, static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
        }
        if (FMath::Abs(ScrollDeltaY) > 0.0f)
        {
            MessageHandler->OnMouseScrolled(ScrollDeltaY, true, static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
        }
    }
}

void FMacApplication::OnKeyEvent(const FDeferredMacEvent& DeferredEvent)
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

void FMacApplication::OnWindowResized(const FDeferredMacEvent& DeferredEvent)
{
    TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(DeferredEvent.Window);
    
    const NSRect ContentRect = DeferredEvent.Window.contentView.frame;
    MessageHandler->OnWindowResized(Window, uint16(ContentRect.size.width), uint16(ContentRect.size.height));
}

void FMacApplication::OnWindowMoved(const FDeferredMacEvent& DeferredEvent)
{
    TSharedRef<FMacWindow> Window = GetWindowFromNSWindow(DeferredEvent.Window);
    
    const NSRect ContentRect = DeferredEvent.Window.contentView.frame;
    MessageHandler->OnWindowMoved(Window, int16(ContentRect.origin.x), int16(ContentRect.origin.y));
}
