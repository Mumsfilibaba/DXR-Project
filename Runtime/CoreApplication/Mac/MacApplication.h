#pragma once
#include "MacCursor.h"
#include "GCInputDevice.h"
#include "Core/Mac/Mac.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/GenericApplication.h"

#include <AppKit/AppKit.h>

@class FCocoaWindow;
@class FMacApplicationObserver;
class FMacWindow;
class FGenericWindow;

// The application runs on a separate thread; the main thread is primarily responsible for handling UI-related tasks.
// We defer events when they occur and process them on the application thread.

struct FDeferredMacEvent
{
    FORCEINLINE FDeferredMacEvent()
        : NotificationName(nil)
        , Event(nil)
        , Window(nil)
        , Character(uint32(-1))
    {
    }

    FORCEINLINE FDeferredMacEvent(const FDeferredMacEvent& Other)
        : NotificationName(Other.NotificationName ? [Other.NotificationName retain] : nil)
        , Event(Other.Event ? [Other.Event retain] : nil)
        , Window(Other.Window ? [Other.Window retain] : nil)
        , Character(Other.Character)
    {
    }

    FORCEINLINE ~FDeferredMacEvent()
    {
        @autoreleasepool
        {
            NSSafeRelease(NotificationName);
            NSSafeRelease(Event);
            NSSafeRelease(Window);
        }
    }

    NSNotificationName NotificationName;
    NSEvent*           Event;
    FCocoaWindow*      Window;
    uint32             Character;
};

class COREAPPLICATION_API FMacApplication final : public FGenericApplication
{
public:
    
    // Create a new MacApplication and return a GenericApplication interface. This function will also initialize the
    // global 'MacApplication' pointer since the constructor and destructor control the value of the global pointer.
    static TSharedPtr<FGenericApplication> Create();

public:
    FMacApplication(const TSharedPtr<FMacCursor>& InCursor);
    virtual ~FMacApplication();

    // FGenericApplication Interface
    virtual TSharedRef<FGenericWindow> CreateWindow() override final;
    virtual void Tick(float Delta) override final;
    virtual void UpdateInputDevices() override final;
    virtual FInputDevice* GetInputDeviceInterface() override final;
    virtual bool SupportsHighPrecisionMouse() const override final;
    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) override final;
    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) override final;
    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const override final;
    virtual TSharedRef<FGenericWindow> GetActiveWindow() const override final;
    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const override final;
    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler) override final;

    // Defer an event to be processed later in the tick function. EventObject can be an NSEvent, NSNotificationName,
    // or NSString based on what type of event we are handling.
    void DeferEvent(NSObject* EventObject);

    // Called from the local event monitor when an event occurs; this function then defers the event to be processed
    // during the next processing of deferred events.
    NSEvent* OnNSEvent(NSEvent* Event);
    
    void OnWindowDestroyed(const TSharedRef<FMacWindow>& Window);
    void OnWindowWillResize(const TSharedRef<FMacWindow>& Window);

    void ProcessDeferredEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseMoveEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseButtonEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseScrollEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseHoverEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessKeyEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessWindowResized(const FDeferredMacEvent& DeferredEvent);
    void ProcessWindowMoved(const FDeferredMacEvent& DeferredEvent);

    // This function uses the current window under the cursor, checks if that window is FCocoaWindow and returns
    // a valid pointer if these conditions are met. Otherwise nullptr is returned. This means that this function
    // can return nullptr even if the cursor is howering over a window, just that this window is not a FCocoaWindow.
    FCocoaWindow* FindNSWindowUnderCursor() const;
    
    // Finds a MacWindow from a NSWindow. If the NSWindow is not a FCocoaWindow the function simply returns nullptr.
    TSharedRef<FMacWindow> FindWindowFromNSWindow(NSWindow* Window) const;
    
    // By closing a window we simply enqueuing an invocation of the MessageHandler's OnWindowClosed event. This
    // will in turn later on call FMacWindow destroy in the upper engine layers, which then finally enqueues the
    // removal of the FMacWindow and destruction of the FCocoaWindow. This is done in order for all engine systems
    // to be able to respond to the window-destruction properly.
    void CloseWindow(const TSharedRef<FMacWindow>& Window);

    FMacApplicationObserver* GetApplicationObserver() const
    {
        return Observer;
    }
    
public:
    
    // Retrieve an FString with the name of the monitor represented by this NSScreen object
    static FString FindMonitorName(NSScreen* Screen);

    // Calculate the DPI for the specified NSScreen
    static uint32 MonitorDPIFromScreen(NSScreen* Screen);

    // Finds a NSScreen based on a position
    static NSScreen* FindScreenFromCocoaPoint(CGFloat PositionX, CGFloat PositionY);

    // Converts a NSPoint specified in Cococa coordinate-system and converts it into the coordinate-system that the engine expectes.
    static NSPoint ConvertCocoaPointToEngine(CGFloat PositionX, CGFloat PositionY);
    
    // Converts a rectangle from Engine coordinate-system into the Cocoa coordinate-system.
    static NSRect ConvertNSRect(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY);
    
private:

    // Event-monitors
    id LocalEventMonitor;
    id GlobalMouseMovedEventMonitor;

    // Observer that checks for monitor changes
    FMacApplicationObserver* Observer;
    FCocoaWindow*            WindowUnderCursor;
    
    // We need to store this modifier-flag in order to handle modifier keys correctly
    NSUInteger             PreviousModifierFlags;
    EMouseButtonName::Type LastPressedButton;

    // InputDevice handling gamepads
    TSharedPtr<FGCInputDevice> InputDevice;
    TSharedPtr<FMacCursor>     MacCursor;
    
    TArray<TSharedRef<FMacWindow>> Windows;
    mutable FCriticalSection WindowsCS;
    
    TArray<FCocoaWindow*> ClosedCocoaWindows;
    FCriticalSection ClosedCocoaWindowsCS;
    
    TArray<TSharedRef<FMacWindow>> ClosedWindows;
    FCriticalSection ClosedWindowsCS;
    
    TArray<FDeferredMacEvent> DeferredEvents;
    FCriticalSection DeferredEventsCS;
};

extern FMacApplication* MacApplication;
