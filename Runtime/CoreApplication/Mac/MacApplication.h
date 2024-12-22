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
 * @brief Lists Mac-specific variants of modifier keys (e.g., left/right Shift, left/right Control).
 *
 * On macOS, it can be useful to distinguish between left/right keys, such as Control or Shift.
 * EMacModifierKey enumerates these specific variants as well as CapsLock and NumLock.
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
 * @brief Stores information about a macOS event (NSEvent) that is deferred for later processing.
 *
 * Certain macOS events (like keyboard, mouse, or notifications) can be queued to be processed
 * at a convenient time (e.g., within FMacApplication::ProcessDeferredEvents). 
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

    /** @brief The native Cocoa window (FCocoaWindow) that the event pertains to, if applicable. */
    FCocoaWindow* CocoaWindow;

    /** @brief A shared reference to the engine-level FMacWindow associated with this event. */
    TSharedRef<FMacWindow> Window;
    
    /** @brief The NSEventType code (e.g., mouse move, key down, etc.). */
    NSEventType EventType;

    /** @brief Flags representing which modifier keys were active during the event (NSEventModifierFlags). */
    NSEventModifierFlags ModifierFlags;

    /** @brief Number of mouse clicks associated with this event (e.g., 1 for single click, 2 for double-click). */
    NSInteger ClickCount;

    /** @brief Phase of a scroll event (e.g., begun, changed, ended). */
    NSEventPhase ScrollPhase;

    /** @brief Scroll deltas (X, Y) for wheel/trackpad scrolling. */
    FVector2 ScrollDelta;

    /** @brief Character code for keyboard events, if applicable (e.g., key down). */
    uint32 Character;

    /** @brief The mouse button number (e.g., left=0, right=1, middle=2) for mouse events. */
    int32  MouseButtonNumber;

    /** @brief The key code for keyboard events (e.g., ANSI code). */
    uint16 KeyCode;

    /** @brief Indicates if the scroll deltas are precise (e.g., from a trackpad). */
    bool bHasPreciseScrollingDeltas;

    /** @brief Indicates if this key event is a repeated event (a key held down). */
    bool bIsRepeat;
};

/**
 * @class FMacApplication
 * @brief The macOS-specific implementation of the FGenericApplication interface.
 *
 * FMacApplication integrates with Cocoa to manage macOS windows, input devices, and the event loop.
 * It defers certain native events (FDeferredMacEvent) and processes them during the engine tick,
 * ensuring a consistent update loop across the engine. 
 */

class COREAPPLICATION_API FMacApplication final : public FGenericApplication
{
public:

    /**
     * @brief Creates a new MacApplication instance and returns it as a FGenericApplication interface.
     * 
     * This function also initializes the global GMacApplication pointer, referencing the application instance.
     * 
     * @return A shared pointer to the newly created FGenericApplication instance.
     */
    static TSharedPtr<FGenericApplication> Create();

public:

    FMacApplication(const TSharedPtr<FMacCursor>& InCursor);
    virtual ~FMacApplication();

public:

    // FGenericApplication Interface Overrides
    virtual TSharedRef<FGenericWindow> CreateWindow() override final;

    virtual void Tick(float Delta) override final;

    virtual void ProcessEvents() override final;

    virtual void ProcessDeferredEvents() override final;

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
     * @brief Defers an NSObject event (NSEvent, NSNotification, etc.) for later processing.
     * 
     * Stores the event in a queue until ProcessDeferredEvents is called.
     * @param EventObject The macOS NSObject representing the event.
     */
    void DeferEvent(NSObject* EventObject);

    /**
     * @brief NSEvent handler invoked by the local event monitor callback.
     * 
     * This function intercepts macOS events and defers them for processing.
     * @param Event The NSEvent being handled.
     * @return The processed NSEvent (returned unchanged by default).
     */
    NSEvent* OnNSEvent(NSEvent* Event);

    /**
     * @brief Handles the destruction of a window, ensuring engine-side cleanup.
     * 
     * @param Window The engine-level window that was destroyed.
     */
    void OnWindowDestroyed(const TSharedRef<FMacWindow>& Window);

    /**
     * @brief Called before a macOS window is resized. Provides a chance for pre-resize logic.
     * 
     * @param Window The FMacWindow about to be resized.
     */
    void OnWindowWillResize(const TSharedRef<FMacWindow>& Window);

    /**
     * @brief Finds the Cocoa (NSWindow) currently under the mouse cursor, if any.
     * 
     * @return A pointer to the FCocoaWindow under the cursor, or nullptr if none found.
     */
    FCocoaWindow* FindNSWindowUnderCursor() const;

    /**
     * @brief Finds the FMacWindow associated with a given NSWindow.
     * 
     * @param Window The native NSWindow to locate in the engine-level window array.
     * @return A shared reference to the corresponding FMacWindow, or nullptr if not found.
     */
    TSharedRef<FMacWindow> FindWindowFromNSWindow(NSWindow* Window) const;

    /**
     * @brief Closes the specified engine window, eventually destroying its platform counterpart.
     * 
     * Once closed, OnWindowClosed is triggered, leading to final removal of references and memory cleanup.
     * @param Window The FMacWindow to close.
     */
    void CloseWindow(const TSharedRef<FMacWindow>& Window);

    /**
     * @brief Retrieves the application observer that manages system-level macOS notifications.
     * 
     * @return A pointer to the FMacApplicationObserver instance, or nullptr if none.
     */
    FMacApplicationObserver* GetApplicationObserver() const
    {
        return Observer;
    }

public:
    /**
     * @brief Retrieves a human-readable name for a monitor (NSScreen).
     * 
     * @param Screen The NSScreen representing the monitor.
     * @return An FString containing the monitor's name.
     */
    static FString FindMonitorName(NSScreen* Screen);

    /**
     * @brief Calculates the DPI for a specific NSScreen.
     * 
     * @param Screen The NSScreen representing the monitor.
     * @return The monitor's DPI as a 32-bit integer.
     */
    static uint32 MonitorDPIFromScreen(NSScreen* Screen);

    /**
     * @brief Finds an NSScreen containing a specific point in Cocoa coordinates.
     * 
     * @param PositionX The X-coordinate in Cocoa space.
     * @param PositionY The Y-coordinate in Cocoa space.
     * @return A pointer to the NSScreen containing the point, or nullptr if none.
     */
    static NSScreen* FindScreenFromCocoaPoint(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Finds an NSScreen based on a position in engine (virtual) coordinates.
     * 
     * @param PositionX The X-coordinate in engine coordinates.
     * @param PositionY The Y-coordinate in engine coordinates.
     * @return A pointer to the NSScreen containing that point, or nullptr if none.
     */
    static NSScreen* FindScreenFromEnginePoint(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a point from Cocoa (macOS) coordinates to engine coordinates.
     * 
     * @param PositionX The X-coordinate in Cocoa space.
     * @param PositionY The Y-coordinate in Cocoa space.
     * @return The converted NSPoint in engine coordinates.
     */
    static NSPoint ConvertCocoaPointToEngine(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a point from engine coordinates to Cocoa (macOS) coordinates.
     * 
     * @param PositionX The X-coordinate in engine space.
     * @param PositionY The Y-coordinate in engine space.
     * @return The converted NSPoint in Cocoa coordinates.
     */
    static NSPoint ConvertEnginePointToCocoa(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a rectangle from engine coordinates to Cocoa coordinates.
     * 
     * @param Width The rectangle's width in engine coordinates.
     * @param Height The rectangle's height in engine coordinates.
     * @param PositionX The rectangle's X-position in engine coordinates.
     * @param PositionY The rectangle's Y-position in engine coordinates.
     * @return The converted rectangle as an NSRect in Cocoa coordinates.
     */
    static NSRect ConvertEngineRectToCocoa(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a rectangle from Cocoa coordinates to engine coordinates.
     * 
     * @param Width The rectangle's width in Cocoa coordinates.
     * @param Height The rectangle's height in Cocoa coordinates.
     * @param PositionX The rectangle's X-position in Cocoa coordinates.
     * @param PositionY The rectangle's Y-position in Cocoa coordinates.
     * @return The converted rectangle as an NSRect in engine coordinates.
     */
    static NSRect ConvertCocoaRectToEngine(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY);

private:
    void ProcessDeferredEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseMoveEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseButtonEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseScrollEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseHoverEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessKeyEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessUpdatedModfierFlags(const FDeferredMacEvent& DeferredEvent);
    void ProcessModfierKey(EMacModifierKey MacModifierKey, uint64 ModifierKeyFlags);
    void ProcessWindowResized(const FDeferredMacEvent& DeferredEvent);
    void ProcessWindowMoved(const FDeferredMacEvent& DeferredEvent);

    id LocalEventMonitor;
    id GlobalMouseMovedEventMonitor;

    FMacApplicationObserver* Observer;
    FCocoaWindow*            WindowUnderCursor;

    NSUInteger             CurrentModifierFlags;
    EMouseButtonName::Type LastPressedButton;

    TSharedPtr<FMacCursor>     MacCursor;
    TSharedPtr<FGCInputDevice> InputDevice;

    TArray<TSharedRef<FMacWindow>> Windows;
    mutable FCriticalSection       WindowsCS;
    TArray<FCocoaWindow*>          ClosedCocoaWindows;
    FCriticalSection               ClosedCocoaWindowsCS;
    TArray<TSharedRef<FMacWindow>> ClosedWindows;
    FCriticalSection               ClosedWindowsCS;
    TArray<FDeferredMacEvent>      DeferredEvents;
    FCriticalSection               DeferredEventsCS;
};

extern FMacApplication* GMacApplication;
