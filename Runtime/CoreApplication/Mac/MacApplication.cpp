#include "Core/Math/Math.h"
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
#include <IOKit/hidsystem/ev_keymap.h>

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
        GMacApplication->RefreshScreenCache();
        GMacApplication->DeferEvent(InNotification);
    }
}

@end

static CGFloat NormalizeWheelDetent(CGFloat Delta)
{
    return (Delta > 0.0) ? 1.0 : ((Delta < 0.0) ? -1.0 : 0.0);
}

FMacApplication* GMacApplication = nullptr;

TSharedPtr<FGenericApplication> FMacApplication::Create()
{
    // Create the cursor interface
    TSharedPtr<FMacCursor> Cursor = MakeSharedPtr<FMacCursor>();

    // Create a new MacApplication instance. The global MacApplication pointer is initialized inside of the FMacApplication constructor
    TSharedPtr<FMacApplication> NewMacApplication = MakeSharedPtr<FMacApplication>(Cursor);
    return NewMacApplication;
}

String FMacApplication::FindMonitorName(NSScreen* Screen)
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
    String Result(MonitorName);
    
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
    const CGDirectDisplayID DisplayID = static_cast<CGDirectDisplayID>([[[Screen deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue]);
    const CGSize PhysicalSize = CGDisplayScreenSize(DisplayID);

    // Zero for a display that reports no EDID, fall back to the default rather than divide by it
    if (PhysicalSize.width <= 0.0 || PhysicalSize.height <= 0.0)
    {
        return 72;
    }

    // Calculate the DPI
    const CGFloat InchToMillimeterFactor = 25.4;
    const CGFloat ScreenWidthDPI         = PixelWidth  / (PhysicalSize.width  / InchToMillimeterFactor);
    const CGFloat ScreenHeightDPI        = PixelHeight / (PhysicalSize.height / InchToMillimeterFactor);

    // Use the average of width and height DPI values
    CGFloat ScreenDPI = (ScreenWidthDPI + ScreenHeightDPI) / 2.0;

    // Round and convert to uint32
    const uint32 RoundedDPI = static_cast<uint32>(Math::RoundToInt(ScreenDPI));
    return RoundedDPI;
}

NSPoint FMacApplication::ConvertCocoaPointToEngine(CGFloat PositionX, CGFloat PositionY)
{
    if (!GMacApplication)
    {
        return NSMakePoint(PositionX, PositionY);
    }

    TScopedLock Lock(GMacApplication->ScreenCacheCS);

    const FMacScreenInfo* Screen = FindScreenFromCocoaPoint(PositionX, PositionY);
    if (!Screen)
    {
        return NSMakePoint(PositionX, PositionY);
    }

    // Adjust the point's coordinates relative to the screen (in points)
    CGFloat RelativeX = PositionX - Screen->Frame.origin.x;
    CGFloat RelativeY = PositionY - Screen->Frame.origin.y;

    // Convert the Y-coordinate from Cocoa (bottom-left origin) to engine (top-left origin)
    CGFloat ConvertedY = Screen->Frame.size.height - RelativeY;

    // Create the converted point in pixels
    NSPoint ConvertedPoint = NSMakePoint(RelativeX, ConvertedY);
    return ConvertedPoint;
}

NSPoint FMacApplication::ConvertEnginePointToCocoa(CGFloat PositionX, CGFloat PositionY)
{
    if (!GMacApplication)
    {
        return NSMakePoint(PositionX, PositionY);
    }

    TScopedLock Lock(GMacApplication->ScreenCacheCS);

    const FMacScreenInfo* Screen = FindScreenFromEnginePoint(PositionX, PositionY);
    if (!Screen)
    {
        return NSMakePoint(PositionX, PositionY);
    }

    // Convert the engine point to Cocoa's coordinate system
    CGFloat RelativeX = Screen->Frame.origin.x + PositionX;
    CGFloat RelativeY = Screen->Frame.origin.y + (Screen->Frame.size.height - PositionY);

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

FMacApplication::FMacApplication(const TSharedPtr<FMacCursor>& InCursor)
    : FGenericApplication(InCursor)
    , LocalEventMonitor(nullptr)
    , GlobalMouseMovedEventMonitor(nullptr)
    , Observer(nullptr)
    , WindowUnderCursor(nullptr)
    , CapturedWindow(nullptr)
    , CurrentModifierFlags(0)
    , LastPressedButton(EMouseButtonName::Unknown)
    , HighPrecisionMouseRemainder()
    , CursorConfinementPosition()
    , CursorConfinementSize()
    , bHighPrecisionMouseEnabled(false)
    , bCursorConfined(false)
    , MacCursor(InCursor)
    , InputDevice(FGCInputDevice::CreateGCInputDevice())
    , ScreenCache()
    , ScreenCacheCS()
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
        
        CHECK_COCOA_MAIN_THREAD();

        RefreshScreenCache();

        FPlatformInputMapper::Initialize();

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

        // Create the edit menu
        NSMenuItem* EditMenuItem = [MenuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];

        NSMenu* EditMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
        EditMenuItem.submenu = EditMenu;

        [EditMenu addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
        [EditMenu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
        [EditMenu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
        [EditMenu addItem:[NSMenuItem separatorItem]];
        [EditMenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];

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
        CHECK_COCOA_MAIN_THREAD();

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

        [CapturedWindow release];
        CapturedWindow = nil;
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
            CHECK_COCOA_MAIN_THREAD();
            SCOPED_AUTORELEASE_POOL();
            
            for (FCocoaWindow* CocoaWindow : LocalClosedCocoaWindows)
            {
                [CocoaWindow close];
                [CocoaWindow release];
            }
        }, NSDefaultRunLoopMode, true);
    }

    ClampCursorToConfinement();
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
    return true;
}

bool FMacApplication::SetHighPrecisionMouseMode(const TSharedRef<FGenericWindow>&, EHighPrecisionMouseMode Mode)
{
    const bool bEnable = (Mode == EHighPrecisionMouseMode::Enabled);
    if (bEnable == bHighPrecisionMouseEnabled)
    {
        return true;
    }

    if (bEnable)
    {
        CGAssociateMouseAndMouseCursorPosition(false);

        if (CGEventSourceRef EventSource = CGEventSourceCreate(kCGEventSourceStateHIDSystemState))
        {
            CGEventSourceSetLocalEventsSuppressionInterval(EventSource, 0.0);
            CFRelease(EventSource);
        }

        HighPrecisionMouseRemainder = Vector2();
    }
    else
    {
        CGAssociateMouseAndMouseCursorPosition(true);
    }

    bHighPrecisionMouseEnabled = bEnable;
    return true;
}

bool FMacApplication::ConfineCursorToRect(const TSharedRef<FGenericWindow>&, const IntVector2& Position, const IntVector2& Size)
{
    if (!MacCursor || Size.X <= 0 || Size.Y <= 0)
    {
        return false;
    }

    CursorConfinementPosition = Position;
    CursorConfinementSize     = Size;
    bCursorConfined           = true;

    ClampCursorToConfinement();
    return true;
}

void FMacApplication::ReleaseCursorConfinement()
{
    bCursorConfined           = false;
    CursorConfinementPosition = IntVector2();
    CursorConfinementSize     = IntVector2();
}

void FMacApplication::ClampCursorToConfinement()
{
    if (!bCursorConfined || !MacCursor)
    {
        return;
    }

    const IntVector2 CursorPosition = MacCursor->GetPosition();
    const int32      ClampedX       = Math::Clamp(CursorPosition.X, CursorConfinementPosition.X, CursorConfinementPosition.X + CursorConfinementSize.X - 1);
    const int32      ClampedY       = Math::Clamp(CursorPosition.Y, CursorConfinementPosition.Y, CursorConfinementPosition.Y + CursorConfinementSize.Y - 1);

    if (ClampedX != CursorPosition.X || ClampedY != CursorPosition.Y)
    {
        MacCursor->SetPosition(ClampedX, ClampedY);
    }
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
        CHECK_COCOA_MAIN_THREAD();

        FCocoaWindow* CocoaWindow = MacWindow->GetCocoaWindow();
        [CocoaWindow makeKeyAndOrderFront:CocoaWindow];
    }, NSDefaultRunLoopMode, false);
}

void FMacApplication::SetCapture(const TSharedRef<FGenericWindow>& Window)
{
    FCocoaWindow* NewCapturedWindow = nullptr;
    if (TSharedRef<FMacWindow> MacWindow = StaticCastSharedRef<FMacWindow>(Window))
    {
        NewCapturedWindow = MacWindow->GetCocoaWindow();
    }

    TScopedLock Lock(CapturedWindowCS);
    if (CapturedWindow != NewCapturedWindow)
    {
        [CapturedWindow release];
        CapturedWindow = [NewCapturedWindow retain];
    }
}

TSharedRef<FGenericWindow> FMacApplication::GetWindowUnderCursor() const
{
    FCocoaWindow* CurrentWindowUnderCursor = nullptr;
    {
        TScopedLock Lock(WindowUnderCursorCS);
        CurrentWindowUnderCursor = [WindowUnderCursor retain];
    }

    TSharedRef<FGenericWindow> Result = FindWindowFromNSWindow(CurrentWindowUnderCursor);
    [CurrentWindowUnderCursor release];
    return Result;
}

TSharedRef<FGenericWindow> FMacApplication::GetActiveWindow() const
{
    NSWindow* KeyWindow = FMacThreadManager::Get().MainThreadDispatchAndReturn(^
    {
        CHECK_COCOA_MAIN_THREAD();
        SCOPED_AUTORELEASE_POOL();
        return [NSApp keyWindow];
    }, NSDefaultRunLoopMode);
    
    return FindWindowFromNSWindow(KeyWindow);
}

TSharedRef<FGenericWindow> FMacApplication::GetCapture() const
{
    // Retained across the lookup, or the main thread can drop the last reference during it
    FCocoaWindow* CurrentCapturedWindow = nullptr;
    {
        TScopedLock Lock(CapturedWindowCS);
        CurrentCapturedWindow = [CapturedWindow retain];
    }

    TSharedRef<FGenericWindow> Result = FindWindowFromNSWindow(CurrentCapturedWindow);
    [CurrentCapturedWindow release];
    return Result;
}

void FMacApplication::QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const
{
    TScopedLock Lock(ScreenCacheCS);

    OutMonitorInfo.Resize(ScreenCache.Size());

    int32 Index = 0;
    for (const FMacScreenInfo& Screen : ScreenCache)
    {
        FMonitorInfo& MonitorInfo = OutMonitorInfo[Index++];
        MonitorInfo.DeviceName     = Screen.DeviceName;
        MonitorInfo.MainPosition   = IntVector2(Screen.Frame.origin.x, Screen.Frame.origin.y);
        MonitorInfo.MainSize       = IntVector2(Screen.Frame.size.width, Screen.Frame.size.height);
        MonitorInfo.WorkPosition   = IntVector2(Screen.VisibleFrame.origin.x, Screen.VisibleFrame.origin.y);
        MonitorInfo.WorkSize       = IntVector2(Screen.VisibleFrame.size.width, Screen.VisibleFrame.size.height);
        MonitorInfo.bIsPrimary     = Screen.bIsPrimary;
        MonitorInfo.DisplayDPI     = Screen.DisplayDPI;
        MonitorInfo.DisplayScaling = Screen.BackingScaleFactor;
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

void FMacApplication::DeferEvent(NSObject* EventObject)
{
    SCOPED_AUTORELEASE_POOL();

    CHECK_COCOA_MAIN_THREAD();

    UpdateWindowUnderCursor();
    
    if (EventObject)
    {
        FDeferredMacEvent NewDeferredEvent;

        NewDeferredEvent.MouseLocation = [NSEvent mouseLocation];

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

                NSNotificationName Name = NewDeferredEvent.NotificationName;
                if (Name == NSWindowDidEnterFullScreenNotification)
                {
                    NewDeferredEvent.ContentFrame     = [EventWindow frame];
                    NewDeferredEvent.bHasContentFrame = true;
                }
                else if (
                    Name == NSWindowDidResizeNotification ||
                    Name == NSWindowDidMoveNotification ||
                    Name == NSWindowDidMiniaturizeNotification ||
                    Name == NSWindowDidDeminiaturizeNotification ||
                    Name == NSWindowDidExitFullScreenNotification)
                {
                    NewDeferredEvent.ContentFrame     = [EventWindow contentRectForFrameRect:EventWindow.frame];
                    NewDeferredEvent.bHasContentFrame = true;
                }
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
                    NewDeferredEvent.ScrollDelta                = Vector2([CurrentEvent scrollingDeltaX], [CurrentEvent scrollingDeltaY]);
                    NewDeferredEvent.bHasPreciseScrollingDeltas = [CurrentEvent hasPreciseScrollingDeltas];
                    break;
                }

                case NSEventTypeMouseMoved:
                case NSEventTypeLeftMouseDragged:
                case NSEventTypeRightMouseDragged:
                case NSEventTypeOtherMouseDragged:
                {
                    NewDeferredEvent.MouseDelta = Vector2([CurrentEvent deltaX], [CurrentEvent deltaY]);
                    break;
                }

                default:
                {
                    break;
                }
            }
        }
        
        TScopedLock Lock(DeferredEventsCS);
        DeferredEvents.Emplace(Move(NewDeferredEvent));
    }
}

NSEvent* FMacApplication::OnNSEvent(NSEvent* Event)
{
    NSWindow* EventWindow = [Event window];
    if (EventWindow && ![EventWindow isKindOfClass:[FCocoaWindow class]])
    {
        UpdateWindowUnderCursor();

        // Modifier state is global rather than per-window
        if (Event.type == NSEventTypeFlagsChanged)
        {
            DeferEvent(Event);
        }

        return Event;
    }

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

    // If the event is returned it is continued to be sent down the responder change,
    // and for events that we want to stop sending we are returning nullptr.
    return ReturnEvent;
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

void FMacApplication::OnWindowDestroyed(const TSharedRef<FMacWindow>& Window)
{
    FCocoaWindow* CocoaWindow = Window->GetCocoaWindow();
    if (CocoaWindow)
    {
        TScopedLock Lock(ClosedCocoaWindowsCS);
        if (!ClosedCocoaWindows.Contains(CocoaWindow))
        {
            ClosedCocoaWindows.Add(CocoaWindow);
        }
    }
    
    // Remove the MacWindow
    {
        TScopedLock Lock(WindowsCS);
        Windows.Remove(Window);
    }
}

void FMacApplication::OnWindowWillResize(const TSharedRef<FMacWindow>& Window)
{
    MessageHandler->OnWindowResizing(Window);
}

void FMacApplication::UpdateWindowUnderCursor()
{
    FCocoaWindow* NewWindowUnderCursor = FindNSWindowUnderCursor();

    TScopedLock Lock(WindowUnderCursorCS);
    if (WindowUnderCursor != NewWindowUnderCursor)
    {
        [WindowUnderCursor release];
        WindowUnderCursor = [NewWindowUnderCursor retain];
    }
}

void FMacApplication::CloseWindow(const TSharedRef<FMacWindow>& Window)
{
    TScopedLock Lock(ClosedWindowsCS);
    
    if (!ClosedWindows.Contains(Window))
    {
        ClosedWindows.Emplace(Window);
    }
}

void FMacApplication::RefreshScreenCache()
{
    SCOPED_AUTORELEASE_POOL();

    CHECK_COCOA_MAIN_THREAD();

    TArray<FMacScreenInfo> NewScreenCache;

    NSScreen* MainScreen = [NSScreen mainScreen];
    for (NSScreen* Screen in [NSScreen screens])
    {
        FMacScreenInfo& ScreenInfo    = NewScreenCache.Emplace();
        ScreenInfo.Frame              = [Screen frame];
        ScreenInfo.VisibleFrame       = [Screen visibleFrame];
        ScreenInfo.BackingScaleFactor = [Screen backingScaleFactor];
        ScreenInfo.DisplayDPI         = MonitorDPIFromScreen(Screen);
        ScreenInfo.DeviceName         = FindMonitorName(Screen);
        ScreenInfo.bIsPrimary         = (Screen == MainScreen);
    }

    const int32 NumScreens = NewScreenCache.Size();

    {
        TScopedLock Lock(ScreenCacheCS);
        ScreenCache = Move(NewScreenCache);
    }

    LOG_INFO("Refreshed screen cache, found %d monitor(s)", NumScreens);
}

const FMacScreenInfo* FMacApplication::FindScreenFromCocoaPoint(CGFloat PositionX, CGFloat PositionY)
{
    if (!GMacApplication || GMacApplication->ScreenCache.IsEmpty())
    {
        return nullptr;
    }

    const NSPoint Position = NSMakePoint(PositionX, PositionY);

    // Find the screen that contains the point
    const FMacScreenInfo* PrimaryScreen = nullptr;
    for (const FMacScreenInfo& CurrentScreen : GMacApplication->ScreenCache)
    {
        if (NSPointInRect(Position, CurrentScreen.Frame))
        {
            return &CurrentScreen;
        }

        if (CurrentScreen.bIsPrimary)
        {
            PrimaryScreen = &CurrentScreen;
        }
    }

    // If no screen contains the point, default to the main screen
    return PrimaryScreen ? PrimaryScreen : &GMacApplication->ScreenCache.First();
}

const FMacScreenInfo* FMacApplication::FindScreenFromEnginePoint(CGFloat PositionX, CGFloat PositionY)
{
    if (!GMacApplication || GMacApplication->ScreenCache.IsEmpty())
    {
        return nullptr;
    }

    // Since EngineX and EngineY are relative to the screen's top-left corner, we need to find the
    // screen that matches these coordinates.
    const FMacScreenInfo* PrimaryScreen = nullptr;
    for (const FMacScreenInfo& CurrentScreen : GMacApplication->ScreenCache)
    {
        // Check if the engine point falls within this screen's bounds
        const CGFloat ScreenWidth  = CurrentScreen.Frame.size.width;
        const CGFloat ScreenHeight = CurrentScreen.Frame.size.height;
        if (PositionX >= 0 && PositionX <= ScreenWidth && PositionY >= 0 && PositionY <= ScreenHeight)
        {
            return &CurrentScreen;
        }

        if (CurrentScreen.bIsPrimary)
        {
            PrimaryScreen = &CurrentScreen;
        }
    }

    // If no screen is found, default to the main screen
    return PrimaryScreen ? PrimaryScreen : &GMacApplication->ScreenCache.First();
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
        else if (NotificationName == NSWindowDidDeminiaturizeNotification)
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
        else if (NotificationName == NSApplicationDidBecomeActiveNotification)
        {
            MessageHandler->OnApplicationActivationChanged(true);
        }
        else if (NotificationName == NSApplicationDidResignActiveNotification)
        {
            MessageHandler->OnApplicationActivationChanged(false);
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

void FMacApplication::ProcessMouseMoveEvent(const FDeferredMacEvent& DeferredEvent)
{
    if (bHighPrecisionMouseEnabled)
    {
        HighPrecisionMouseRemainder = HighPrecisionMouseRemainder + DeferredEvent.MouseDelta;

        const int32 DeltaX = static_cast<int32>(HighPrecisionMouseRemainder.X);
        const int32 DeltaY = static_cast<int32>(HighPrecisionMouseRemainder.Y);
        
        if (DeltaX != 0 || DeltaY != 0)
        {
            HighPrecisionMouseRemainder.X -= static_cast<float>(DeltaX);
            HighPrecisionMouseRemainder.Y -= static_cast<float>(DeltaY);
            MessageHandler->OnHighPrecisionMouseInput(DeltaX, DeltaY);
        }

        return;
    }

    const NSPoint MouseLocation  = DeferredEvent.MouseLocation;
    const NSPoint CursorPosition = ConvertCocoaPointToEngine(MouseLocation.x, MouseLocation.y);
    MacCursor->UpdateCursorPosition(IntVector2(static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y)));

    MessageHandler->OnMouseMove(static_cast<int32>(CursorPosition.x), static_cast<int32>(CursorPosition.y));
}

void FMacApplication::ProcessMouseButtonEvent(const FDeferredMacEvent& DeferredEvent)
{
    // Convert the MouseButton into engine enum
    const EMouseButtonName::Type CurrentMouseButton = FPlatformInputMapper::GetButtonFromIndex(DeferredEvent.MouseButtonNumber);

    // MouseDown otherwise it is a MouseUp event
    if (DeferredEvent.EventType == NSEventTypeLeftMouseDown || DeferredEvent.EventType == NSEventTypeRightMouseDown || DeferredEvent.EventType == NSEventTypeOtherMouseDown)
    {
        if (LastPressedButton == CurrentMouseButton && DeferredEvent.ClickCount == 2)
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
    if (DeferredEvent.ScrollPhase == NSEventPhaseCancelled)
    {
        return;
    }

    CGFloat ScrollDeltaX = DeferredEvent.ScrollDelta.X;
    CGFloat ScrollDeltaY = DeferredEvent.ScrollDelta.Y;

    if (DeferredEvent.bHasPreciseScrollingDeltas)
    {
        constexpr CGFloat PointsPerDetent = 0.1;
        ScrollDeltaX *= PointsPerDetent;
        ScrollDeltaY *= PointsPerDetent;
    }
    else
    {
        ScrollDeltaX = NormalizeWheelDetent(ScrollDeltaX);
        ScrollDeltaY = NormalizeWheelDetent(ScrollDeltaY);
    }

    if (Math::Abs(ScrollDeltaX) > 0.0f)
    {
        MessageHandler->OnMouseScrolled(ScrollDeltaX, EScrollAxis::Horizontal);
    }
    if (Math::Abs(ScrollDeltaY) > 0.0f)
    {
        MessageHandler->OnMouseScrolled(ScrollDeltaY, EScrollAxis::Vertical);
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

    if (DeferredEvent.EventType != NSEventTypeFlagsChanged)
    {
        constexpr uint64 DeviceIndependentFlags = static_cast<uint64>(NSEventModifierFlagDeviceIndependentFlagsMask);
        CurrentModifierFlags = (CurrentModifierFlags & ~DeviceIndependentFlags) | (ModifierFlags & DeviceIndependentFlags);
        return;
    }

    if (ModifierFlags == CurrentModifierFlags)
    {
        return;
    }

    const uint64 PreviousModifierFlags = CurrentModifierFlags;
    CurrentModifierFlags = ModifierFlags;

    ProcessModfierKey(EMacModifierKey::LeftControl, ModifierFlags, PreviousModifierFlags);
    ProcessModfierKey(EMacModifierKey::RightControl, ModifierFlags, PreviousModifierFlags);
    ProcessModfierKey(EMacModifierKey::LeftShift, ModifierFlags, PreviousModifierFlags);
    ProcessModfierKey(EMacModifierKey::RightShift, ModifierFlags, PreviousModifierFlags);
    ProcessModfierKey(EMacModifierKey::LeftCommand, ModifierFlags, PreviousModifierFlags);
    ProcessModfierKey(EMacModifierKey::RightCommand, ModifierFlags, PreviousModifierFlags);
    ProcessModfierKey(EMacModifierKey::LeftAlt, ModifierFlags, PreviousModifierFlags);
    ProcessModfierKey(EMacModifierKey::RightAlt, ModifierFlags, PreviousModifierFlags);
    ProcessModfierKey(EMacModifierKey::CapsLock, ModifierFlags, PreviousModifierFlags);
}

void FMacApplication::ProcessModfierKey(EMacModifierKey::Type MacModifierKey, uint64 ModifierKeyFlags, uint64 PreviousModifierKeyFlags)
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
    };

    static_assert(ARRAY_COUNT(ModifierKeyMask) == ARRAY_COUNT(KeyBoardNames), "Modifier key tables must stay the same length");

    // Ensure that the modifier key is within the allowed range
    CHECK(MacModifierKey >= EMacModifierKey::LeftControl && MacModifierKey < static_cast<int32>(ARRAY_COUNT(ModifierKeyMask)));
    
    // Retrieve the key-name
    const EKeyboardKeyName::Type KeyName = KeyBoardNames[MacModifierKey];

    // Retrieve the key-mask
    const uint64 KeyFlag = ModifierKeyMask[MacModifierKey];

    const bool bIsPressed     = (KeyFlag & ModifierKeyFlags)         != 0;
    const bool bIsPrevPressed = (KeyFlag & PreviousModifierKeyFlags) != 0;

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

    // DeferEvent captures the geometry for every notification that reaches this function
    CHECK(DeferredEvent.bHasContentFrame);

    // Convert the coordinates to the generic ones that are expected
    NSRect ContentFrame = DeferredEvent.ContentFrame;
    ContentFrame = FMacApplication::ConvertCocoaRectToEngine(ContentFrame.size.width, ContentFrame.size.height, ContentFrame.origin.x, ContentFrame.origin.y);

    // Window can move sometimes when resized so send and event about it
    const int32 PositionX = static_cast<int32>(ContentFrame.origin.x);
    const int32 PositionY = static_cast<int32>(ContentFrame.origin.y);
    
    const IntVector2 CachedPosition = DeferredEvent.Window->GetCachedPosition();
    if (CachedPosition.X != PositionX || CachedPosition.Y != PositionY)
    {
        MessageHandler->OnWindowMoved(DeferredEvent.Window, PositionX, PositionY);
        DeferredEvent.Window->SetCachedPosition(IntVector2(PositionX, PositionY));
    }
    
    MessageHandler->OnWindowResized(DeferredEvent.Window, uint32(ContentFrame.size.width), uint32(ContentFrame.size.height));
}

void FMacApplication::ProcessWindowMoved(const FDeferredMacEvent& DeferredEvent)
{
    CHECK(DeferredEvent.bHasContentFrame);

    NSRect ContentFrame = DeferredEvent.ContentFrame;
    ContentFrame = FMacApplication::ConvertCocoaRectToEngine(ContentFrame.size.width, ContentFrame.size.height, ContentFrame.origin.x, ContentFrame.origin.y);
    
    const int32 PositionX = static_cast<int32>(ContentFrame.origin.x);
    const int32 PositionY = static_cast<int32>(ContentFrame.origin.y);
    
    const IntVector2 CachedPosition = DeferredEvent.Window->GetCachedPosition();
    if (CachedPosition.X != PositionX || CachedPosition.Y != PositionY)
    {
        MessageHandler->OnWindowMoved(DeferredEvent.Window, PositionX, PositionY);
        DeferredEvent.Window->SetCachedPosition(IntVector2(PositionX, PositionY));
    }
}
