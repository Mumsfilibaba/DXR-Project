#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Math/Vector2.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Mac/MacCursor.h"
#include "CoreApplication/Mac/MacWindow.h"
#include "CoreApplication/Mac/GCInputDevice.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/GenericApplication.h"
#include <AppKit/AppKit.h>

@class FCocoaWindow;
@class FMacApplicationObserver;

/**
 * @enum EMacModifierKey
 * @brief Represents different Mac-specific modifier keys.
 *
 * Each enumerator corresponds to a particular modifier key on macOS systems, allowing for detection
 * of left/right variations of keys like Control, Shift, Command (Super), and Alt, as well as Caps Lock.
 */

enum EMacModifierKey
{
    MacModifierKey_LeftControl = 0,
    MacModifierKey_RightControl,
    MacModifierKey_LeftShift,
    MacModifierKey_RightShift,
    MacModifierKey_LeftCommand,
    MacModifierKey_RightCommand,
    MacModifierKey_LeftAlt,
    MacModifierKey_RightAlt,
    MacModifierKey_CapsLock,
    MacModifierKey_NumLock,
};

/**
 * @struct FDeferredMacEvent
 * @brief Contains data related to a macOS event that has been deferred for later processing.
 *
 * FDeferredMacEvent stores detailed information about an event such as the original NSEvent,
 * associated window, event type, modifiers, mouse state, and more. It allows events received
 * by the application at various times to be processed in a controlled manner (e.g., in the Tick function).
 */

struct FDeferredMacEvent
{
    FORCEINLINE FDeferredMacEvent()
        : NotificationName(nullptr)
        , Event(nullptr)
        , CocoaWindow(nullptr)
        , Window(nullptr)
        , EventType((NSEventType)0)
        , ModifierFlags(0)
        , ClickCount(0)
        , ScrollPhase(NSEventPhaseNone)
        , ScrollDelta()
        , Character((uint32)~0)
        , MouseButtonNumber(0)
        , KeyCode(0)
        , bHasPreciseScrollingDeltas(false)
        , bIsRepeat(false)
    {
    }

    FORCEINLINE FDeferredMacEvent(const FDeferredMacEvent& Other)
        : NotificationName(Other.NotificationName ? [Other.NotificationName retain] : nullptr)
        , Event(Other.Event ? [Other.Event retain] : nullptr)
        , CocoaWindow(Other.CocoaWindow ? [Other.CocoaWindow retain] : nullptr)
        , Window(Other.Window)
        , EventType(Other.EventType)
        , ModifierFlags(Other.ModifierFlags)
        , ClickCount(Other.ClickCount)
        , ScrollPhase(Other.ScrollPhase)
        , ScrollDelta(Other.ScrollDelta)
        , Character(Other.Character)
        , MouseButtonNumber(Other.MouseButtonNumber)
        , KeyCode(Other.KeyCode)
        , bHasPreciseScrollingDeltas(Other.bHasPreciseScrollingDeltas)
        , bIsRepeat(Other.bIsRepeat)
    {
    }
    
    FORCEINLINE ~FDeferredMacEvent()
    {
        @autoreleasepool
        {
            [NotificationName release];
            [Event release];
            [CocoaWindow release];
        }
    }

    /** @brief The name of the notification associated with this event, if any. */
    NSNotificationName NotificationName;

    /** @brief The original NSEvent object, retained for reference. */
    NSEvent* Event;

    /** @brief The Cocoa window (FCocoaWindow) that the event pertains to, if any. */
    FCocoaWindow* CocoaWindow;

    /** @brief The engine window (FMacWindow) associated with this event. */
    TSharedRef<FMacWindow> Window;
    
    /** @brief The type of the event (mouse, keyboard, etc.). */
    NSEventType EventType;

    /** @brief Flags representing which modifier keys were active during the event. */
    NSEventModifierFlags ModifierFlags;

    /** @brief Number of mouse clicks (e.g., single, double click). */
    NSInteger ClickCount;

    /** @brief Phase of a scroll event (e.g., begun, changed, ended). */
    NSEventPhase ScrollPhase;

    /** @brief Scroll deltas for mouse wheel events. */
    FVector2 ScrollDelta;

    /** @brief Character code for keyboard events, if applicable. */
    uint32 Character;

    /** @brief The mouse button number (e.g., left, right, middle) for mouse events. */
    int32  MouseButtonNumber;

    /** @brief The key code for keyboard events. */
    uint16 KeyCode;

    /** @brief Indicates if the scroll deltas are precise (e.g., from a trackpad). */
    bool bHasPreciseScrollingDeltas;

    /** @brief Indicates if this is a repeated key event (key held down). */
    bool bIsRepeat;
};

/**
 * @class FMacApplication
 * @brief The Mac-specific implementation of the FGenericApplication interface.
 *
 * FMacApplication integrates with the macOS application lifecycle, handling event processing,
 * window management, input devices, and platform-specific operations. It translates native macOS
 * events (NSEvents) into engine-level events, manages Mac windows (FMacWindow and FCocoaWindow),
 * and defers certain events for processing during the application's Tick function.
 */

class COREAPPLICATION_API FMacApplication final : public FGenericApplication
{
public:
    /**
     * @brief Creates a new MacApplication instance and returns it as a FGenericApplication interface.
     * 
     * This also initializes the global GMacApplication pointer, which references the application instance.
     * 
     * @return A shared pointer to the newly created FGenericApplication instance.
     */
    static TSharedPtr<FGenericApplication> Create();

public:

    FMacApplication(const TSharedPtr<FMacCursor>& InCursor);
    virtual ~FMacApplication();

public:

    // FGenericApplication Interface
    virtual TSharedRef<FGenericWindow> CreateWindow() override final;

    virtual void Tick(float Delta) override final;

    virtual void UpdateInputDevices() override final;

    virtual FInputDevice* GetInputDevice() override final;

    virtual bool SupportsHighPrecisionMouse() const override final;

    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) override final;

    virtual FModifierKeyState GetModifierKeyState() const override final;

    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) override final;

    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const override final;

    virtual TSharedRef<FGenericWindow> GetActiveWindow() const override final;

    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const override final;

    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler) override final;

public:
    
    /**
     * @brief Defers an event (NSEvent, NSNotificationName, etc.) to be processed later.
     * 
     * Events are not processed immediately but stored for processing in the Tick function,
     * ensuring that event processing aligns with the application's update loop.
     * 
     * @param EventObject The event object to defer.
     */
    void DeferEvent(NSObject* EventObject);

    /**
     * @brief Handler for NSEvents received by the local event monitor.
     * 
     * Called by macOS when an event occurs. This function defers the event for later processing.
     * 
     * @param Event The NSEvent to handle.
     * @return The processed NSEvent (can be returned as-is).
     */
    NSEvent* OnNSEvent(NSEvent* Event);
        
    /**
     * @brief Handles the destruction of a window.
     * 
     * @param Window The window that is being destroyed.
     */
    void OnWindowDestroyed(const TSharedRef<FMacWindow>& Window);

    /**
     * @brief Called before a window is resized, allowing for any necessary pre-resize logic.
     * 
     * @param Window The window that will be resized.
     */
    void OnWindowWillResize(const TSharedRef<FMacWindow>& Window);

    /**
     * @brief Processes a deferred event that was previously queued by DeferEvent.
     * 
     * @param DeferredEvent The deferred event to process.
     */
    void ProcessDeferredEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a deferred mouse move event.
     * 
     * @param DeferredEvent The deferred mouse move event to process.
     */
    void ProcessMouseMoveEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a deferred mouse button event (click, release).
     * 
     * @param DeferredEvent The deferred mouse button event to process.
     */
    void ProcessMouseButtonEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a deferred mouse scroll event.
     * 
     * @param DeferredEvent The deferred mouse scroll event to process.
     */
    void ProcessMouseScrollEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a deferred mouse hover event.
     * 
     * @param DeferredEvent The deferred mouse hover event to process.
     */
    void ProcessMouseHoverEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a deferred keyboard event (key down/up).
     * 
     * @param DeferredEvent The deferred key event to process.
     */
    void ProcessKeyEvent(const FDeferredMacEvent& DeferredEvent);
    
    /**
     * @brief Processes updates to all modifier keys (Ctrl, Alt, Shift, Command).
     *
     * @param DeferredEvent The deferred event that may contain new modifier states.
     */
    void ProcessUpdatedModfierFlags(const FDeferredMacEvent& DeferredEvent);
    
    /**
     * @brief Processes the state change of a specific modifier key, dispatching appropriate events if needed.
     *
     * @param MacModifierKey The specific Mac modifier key to process.
     * @param ModifierKeyFlags The new modifier key state flags.
     */
    void ProcessModfierKey(EMacModifierKey MacModifierKey, uint64 ModifierKeyFlags);

    /**
     * @brief Processes a window resized event.
     * 
     * @param DeferredEvent The deferred window resized event.
     */
    void ProcessWindowResized(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a window moved event.
     * 
     * @param DeferredEvent The deferred window moved event.
     */
    void ProcessWindowMoved(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Finds the Cocoa (NSWindow) currently under the mouse cursor, if any.
     * 
     * @return A pointer to the FCocoaWindow under the cursor, or nullptr if none found.
     */
    FCocoaWindow* FindNSWindowUnderCursor() const;

    /**
     * @brief Finds the FMacWindow associated with a given NSWindow.
     * 
     * If the NSWindow is not an FCocoaWindow, returns nullptr.
     * 
     * @param Window The NSWindow to find a corresponding FMacWindow for.
     * @return A shared reference to the associated FMacWindow, or nullptr if not found.
     */
    TSharedRef<FMacWindow> FindWindowFromNSWindow(NSWindow* Window) const;

    /**
     * @brief Closes the specified window.
     * 
     * Initiates a process where OnWindowClosed will eventually be called, leading to final destruction
     * of the window objects. Ensures proper cleanup and notification across engine systems.
     * 
     * @param Window The window to close.
     */
    void CloseWindow(const TSharedRef<FMacWindow>& Window);

    /**
     * @brief Retrieves the application observer instance used for macOS application notifications.
     * 
     * @return A pointer to the FMacApplicationObserver instance.
     */
    FMacApplicationObserver* GetApplicationObserver() const
    {
        return Observer;
    }

public:

    /**
     * @brief Retrieves a human-readable name for the given monitor (NSScreen).
     * 
     * @param Screen The NSScreen representing the monitor.
     * @return The monitor name as an FString.
     */
    static FString FindMonitorName(NSScreen* Screen);

    /**
     * @brief Calculates the DPI for a given NSScreen.
     * 
     * @param Screen The NSScreen for which to calculate DPI.
     * @return The DPI as an unsigned 32-bit integer.
     */
    static uint32 MonitorDPIFromScreen(NSScreen* Screen);

    /**
     * @brief Finds an NSScreen based on a given position in Cocoa (macOS) coordinate space.
     * 
     * @param PositionX The X-coordinate in Cocoa coordinates.
     * @param PositionY The Y-coordinate in Cocoa coordinates.
     * @return A pointer to the NSScreen that contains the specified point, or nullptr if none.
     */
    static NSScreen* FindScreenFromCocoaPoint(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Finds an NSScreen based on a given position in the Engine coordinate system.
     * 
     * @param PositionX The X-coordinate in Engine coordinates.
     * @param PositionY The Y-coordinate in Engine coordinates.
     * @return A pointer to the NSScreen that contains the specified point, or nullptr if none.
     */
    static NSScreen* FindScreenFromEnginePoint(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a point from Cocoa coordinates to Engine coordinates.
     * 
     * @param PositionX The X-coordinate in Cocoa coordinates.
     * @param PositionY The Y-coordinate in Cocoa coordinates.
     * @return The point converted to Engine coordinates as an NSPoint.
     */
    static NSPoint ConvertCocoaPointToEngine(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a point from Engine coordinates to Cocoa coordinates.
     * 
     * @param PositionX The X-coordinate in Engine coordinates.
     * @param PositionY The Y-coordinate in Engine coordinates.
     * @return The point converted to Cocoa coordinates as an NSPoint.
     */
    static NSPoint ConvertEnginePointToCocoa(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a rectangle from Engine coordinates to Cocoa coordinates.
     * 
     * @param Width The width of the rectangle in Engine coordinates.
     * @param Height The height of the rectangle in Engine coordinates.
     * @param PositionX The X-position of the rectangle in Engine coordinates.
     * @param PositionY The Y-position of the rectangle in Engine coordinates.
     * @return The NSRect converted to Cocoa coordinates.
     */
    static NSRect ConvertEngineRectToCocoa(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a rectangle from Cocoa coordinates to Engine coordinates.
     * 
     * @param Width The width of the rectangle in Cocoa coordinates.
     * @param Height The height of the rectangle in Cocoa coordinates.
     * @param PositionX The X-position of the rectangle in Cocoa coordinates.
     * @param PositionY The Y-position of the rectangle in Cocoa coordinates.
     * @return The NSRect converted to Engine coordinates.
     */
    static NSRect ConvertCocoaRectToEngine(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY);
        
private:

    id LocalEventMonitor;
    id GlobalMouseMovedEventMonitor;

    FMacApplicationObserver* Observer;
    FCocoaWindow* WindowUnderCursor;

    NSUInteger CurrentModifierFlags;
    EMouseButtonName::Type LastPressedButton;

    TSharedPtr<FMacCursor> MacCursor;
    TSharedPtr<FGCInputDevice> InputDevice;

    TArray<TSharedRef<FMacWindow>> Windows;
    mutable FCriticalSection WindowsCS;

    TArray<FCocoaWindow*> ClosedCocoaWindows;
    FCriticalSection ClosedCocoaWindowsCS;

    TArray<TSharedRef<FMacWindow>> ClosedWindows;
    FCriticalSection ClosedWindowsCS;

    TArray<FDeferredMacEvent> DeferredEvents;
    FCriticalSection DeferredEventsCS;
};

/** 
 * @brief A global pointer to the FMacApplication instance, accessible throughout the application.
 */
extern FMacApplication* GMacApplication;
