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

static FString GetMonitorNameFromNSScreen(NSScreen* Screen)
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

static uint32 GetDPIFromNSScreen(NSScreen* Screen)
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


FMacApplication* MacApplication = nullptr;

TSharedPtr<FMacApplication> FMacApplication::CreateMacApplication()
{
    // Create a new instance and set the the global instance
    TSharedPtr<FMacApplication> NewMacApplication = MakeShared<FMacApplication>();
    MacApplication = NewMacApplication.Get();
    return NewMacApplication;
}

FMacApplication::FMacApplication()
    : FGenericApplication(MakeShared<FMacCursor>())
    , DisplayInfo()
    , Observer(nullptr)
    , InputDevice(FGCInputDevice::CreateGCInputDevice())
    , LastPressedButton(EMouseButtonName::Unknown)
    , bHasDisplayInfoChanged(true)
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

        // Release the observer
        NSSafeRelease(Observer);

        // Release all windows
        Windows.Clear();

        // Reset the global instance
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

void FMacApplication::UpdateGamepadDevices()
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
        FCocoaWindow* CocoaWindow = MacWindow->GetWindow();
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

void FMacApplication::GetDisplayInfo(FDisplayInfo& OutDisplayInfo) const
{
    if (!bHasDisplayInfoChanged)
    {
        OutDisplayInfo = DisplayInfo;
        return;
    }
    
    for (NSScreen* Screen in NSScreen.screens)
    {
        const NSRect ScreenFrame        = Screen.frame;
        const NSRect ScreenVisibleFrame = Screen.visibleFrame;
        
        FMonitorInfo NewMonitorInfo;
        NewMonitorInfo.DeviceName         = GetMonitorNameFromNSScreen(Screen);
        NewMonitorInfo.MainPosition       = FIntVector2(ScreenFrame.origin.x, ScreenFrame.origin.y);
        NewMonitorInfo.MainSize           = FIntVector2(ScreenFrame.size.width, ScreenFrame.size.height);
        NewMonitorInfo.WorkPosition       = FIntVector2(ScreenVisibleFrame.origin.x, ScreenVisibleFrame.origin.y);
        NewMonitorInfo.WorkSize           = FIntVector2(ScreenVisibleFrame.size.width, ScreenVisibleFrame.size.height);
        NewMonitorInfo.bIsPrimary         = [NSScreen mainScreen] == Screen;
        NewMonitorInfo.DisplayDPI         = GetDPIFromNSScreen(Screen);
        NewMonitorInfo.DisplayScaling     = Screen.backingScaleFactor;
        // Convert the backingScaleFactor to a value similar to the one in windows that can be retreived via the 'GetScaleFactorForMonitor' function
        NewMonitorInfo.DisplayScaleFactor = static_cast<uint32>(FMath::Round(NewMonitorInfo.DisplayScaling * 100.0f));

        if (NewMonitorInfo.bIsPrimary)
        {
            DisplayInfo.PrimaryDisplayWidth  = NewMonitorInfo.MainSize.x;
            DisplayInfo.PrimaryDisplayHeight = NewMonitorInfo.MainSize.y;
        }

        DisplayInfo.MonitorInfos.Add(NewMonitorInfo);
    }
    
    // Ensure that we don't waste any space
    DisplayInfo.MonitorInfos.Shrink();
    
    // Ensure that we don't do the same calculations when calling GetMonitorInfo multiple times
    bHasDisplayInfoChanged = false;
    
    //Return the DisplayInfo
    OutDisplayInfo = DisplayInfo;
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
            bHasDisplayInfoChanged = true;
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
                MessageHandler->OnKeyUp(FPlatformInputMapper::GetKeyCodeFromScanCode(CurrentEvent.keyCode), FPlatformApplicationMisc::GetModifierKeyState());
                break;
            }
               
            case NSEventTypeKeyDown:
            {
                MessageHandler->OnKeyDown(FPlatformInputMapper::GetKeyCodeFromScanCode(CurrentEvent.keyCode), CurrentEvent.ARepeat, FPlatformApplicationMisc::GetModifierKeyState());
                break;
            }

            case NSEventTypeLeftMouseUp:
            case NSEventTypeRightMouseUp:
            case NSEventTypeOtherMouseUp:
            {
                const NSPoint CursorPosition = [NSEvent mouseLocation];
                MessageHandler->OnMouseButtonUp(FPlatformInputMapper::GetButtonFromIndex(static_cast<int32>(CurrentEvent.buttonNumber)), FPlatformApplicationMisc::GetModifierKeyState(), static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
                break;
            }

            case NSEventTypeLeftMouseDown:
            case NSEventTypeRightMouseDown:
            case NSEventTypeOtherMouseDown:
            {
                const EMouseButtonName::Type CurrentMouseButton = FPlatformInputMapper::GetButtonFromIndex(static_cast<int32>(CurrentEvent.buttonNumber));
                
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
