#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Math/Vector2.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Mac/MacCursor.h"
#include "CoreApplication/Mac/MacWindow.h"
#include "CoreApplication/Mac/GCInputDevice.h"
#include "CoreApplication/PlatformInterface/InputCodes.h"
#include "CoreApplication/PlatformInterface/IPlatformApplication.h"
#include <AppKit/AppKit.h>

@class FCocoaWindow;
@class FMacApplicationObserver;

struct EMacModifierKey
{
    enum Type
    {
        LeftControl = 0,
        RightControl,
        LeftShift,
        RightShift,
        LeftCommand,
        RightCommand,
        LeftAlt,
        RightAlt,
        CapsLock,
    };
};

struct FDeferredMacEvent
{
    FORCEINLINE FDeferredMacEvent()
        : NotificationName(nullptr)
        , Event(nullptr)
        , CocoaWindow(nullptr)
        , Window(nullptr)
        , ContentFrame(NSZeroRect)
        , MouseLocation(NSZeroPoint)
        , EventType((NSEventType)0)
        , ModifierFlags(0)
        , ClickCount(0)
        , ScrollPhase(NSEventPhaseNone)
        , ScrollDelta()
        , MouseDelta()
        , Character((uint32)~0)
        , MouseButtonNumber(0)
        , KeyCode(0)
        , bHasContentFrame(false)
        , bHasPreciseScrollingDeltas(false)
        , bIsRepeat(false)
    {
    }

    FORCEINLINE FDeferredMacEvent(const FDeferredMacEvent& Other)
        : NotificationName(Other.NotificationName ? [Other.NotificationName retain] : nullptr)
        , Event(Other.Event ? [Other.Event retain] : nullptr)
        , CocoaWindow(Other.CocoaWindow ? [Other.CocoaWindow retain] : nullptr)
        , Window(Other.Window)
        , ContentFrame(Other.ContentFrame)
        , MouseLocation(Other.MouseLocation)
        , EventType(Other.EventType)
        , ModifierFlags(Other.ModifierFlags)
        , ClickCount(Other.ClickCount)
        , ScrollPhase(Other.ScrollPhase)
        , ScrollDelta(Other.ScrollDelta)
        , MouseDelta(Other.MouseDelta)
        , Character(Other.Character)
        , MouseButtonNumber(Other.MouseButtonNumber)
        , KeyCode(Other.KeyCode)
        , bHasContentFrame(Other.bHasContentFrame)
        , bHasPreciseScrollingDeltas(Other.bHasPreciseScrollingDeltas)
        , bIsRepeat(Other.bIsRepeat)
    {
    }

    FORCEINLINE FDeferredMacEvent(FDeferredMacEvent&& Other)
        : NotificationName(Other.NotificationName)
        , Event(Other.Event)
        , CocoaWindow(Other.CocoaWindow)
        , Window(Move(Other.Window))
        , ContentFrame(Other.ContentFrame)
        , MouseLocation(Other.MouseLocation)
        , EventType(Other.EventType)
        , ModifierFlags(Other.ModifierFlags)
        , ClickCount(Other.ClickCount)
        , ScrollPhase(Other.ScrollPhase)
        , ScrollDelta(Other.ScrollDelta)
        , MouseDelta(Other.MouseDelta)
        , Character(Other.Character)
        , MouseButtonNumber(Other.MouseButtonNumber)
        , KeyCode(Other.KeyCode)
        , bHasContentFrame(Other.bHasContentFrame)
        , bHasPreciseScrollingDeltas(Other.bHasPreciseScrollingDeltas)
        , bIsRepeat(Other.bIsRepeat)
    {
        Other.NotificationName = nullptr;
        Other.Event            = nullptr;
        Other.CocoaWindow      = nullptr;
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

    FORCEINLINE FDeferredMacEvent& operator=(const FDeferredMacEvent& Other)
    {
        if (this != &Other)
        {
            NSNotificationName NewNotificationName = Other.NotificationName ? [Other.NotificationName retain] : nullptr;
            NSEvent*           NewEvent            = Other.Event            ? [Other.Event retain]            : nullptr;
            FCocoaWindow*      NewCocoaWindow      = Other.CocoaWindow      ? [Other.CocoaWindow retain]      : nullptr;

            @autoreleasepool
            {
                [NotificationName release];
                [Event release];
                [CocoaWindow release];
            }

            NotificationName = NewNotificationName;
            Event            = NewEvent;
            CocoaWindow      = NewCocoaWindow;

            Window                     = Other.Window;
            ContentFrame               = Other.ContentFrame;
            MouseLocation              = Other.MouseLocation;
            EventType                  = Other.EventType;
            ModifierFlags              = Other.ModifierFlags;
            ClickCount                 = Other.ClickCount;
            ScrollPhase                = Other.ScrollPhase;
            ScrollDelta                = Other.ScrollDelta;
            MouseDelta                 = Other.MouseDelta;
            Character                  = Other.Character;
            MouseButtonNumber          = Other.MouseButtonNumber;
            KeyCode                    = Other.KeyCode;
            bHasContentFrame           = Other.bHasContentFrame;
            bHasPreciseScrollingDeltas = Other.bHasPreciseScrollingDeltas;
            bIsRepeat                  = Other.bIsRepeat;
        }

        return *this;
    }

    FORCEINLINE FDeferredMacEvent& operator=(FDeferredMacEvent&& Other)
    {
        if (this != &Other)
        {
            @autoreleasepool
            {
                [NotificationName release];
                [Event release];
                [CocoaWindow release];
            }

            NotificationName = Other.NotificationName;
            Event            = Other.Event;
            CocoaWindow      = Other.CocoaWindow;

            Other.NotificationName = nullptr;
            Other.Event            = nullptr;
            Other.CocoaWindow      = nullptr;

            Window                     = Move(Other.Window);
            ContentFrame               = Other.ContentFrame;
            MouseLocation              = Other.MouseLocation;
            EventType                  = Other.EventType;
            ModifierFlags              = Other.ModifierFlags;
            ClickCount                 = Other.ClickCount;
            ScrollPhase                = Other.ScrollPhase;
            ScrollDelta                = Other.ScrollDelta;
            MouseDelta                 = Other.MouseDelta;
            Character                  = Other.Character;
            MouseButtonNumber          = Other.MouseButtonNumber;
            KeyCode                    = Other.KeyCode;
            bHasContentFrame           = Other.bHasContentFrame;
            bHasPreciseScrollingDeltas = Other.bHasPreciseScrollingDeltas;
            bIsRepeat                  = Other.bIsRepeat;
        }

        return *this;
    }

    /** @brief The name of the notification associated with this event, if any. */
    NSNotificationName NotificationName;

    /** @brief The original NSEvent object, retained for reference. */
    NSEvent* Event;

    /** @brief The native Cocoa window (FCocoaWindow) that the event pertains to, if applicable. */
    FCocoaWindow* CocoaWindow;

    /** @brief A shared reference to the engine-level FMacWindow associated with this event. */
    TSharedRef<FMacWindow> Window;

    /** @brief The content rect of the window, in Cocoa coordinates, valid when bHasContentFrame. */
    NSRect ContentFrame;

    /** @brief The cursor position in Cocoa coordinates at the time the event was deferred. */
    NSPoint MouseLocation;

    /** @brief The NSEventType code (e.g., mouse move, key down, etc.). */
    NSEventType EventType;

    /** @brief Flags representing which modifier keys were active during the event (NSEventModifierFlags). */
    NSEventModifierFlags ModifierFlags;

    /** @brief Number of mouse clicks associated with this event (e.g., 1 for single click, 2 for double-click). */
    NSInteger ClickCount;

    /** @brief Phase of a scroll event (e.g., begun, changed, ended). */
    NSEventPhase ScrollPhase;

    /** @brief Scroll deltas (X, Y) for wheel/trackpad scrolling. */
    Vector2 ScrollDelta;

    /** @brief Relative motion (X, Y) in Cocoa coordinates, valid for move and drag events. */
    Vector2 MouseDelta;

    /** @brief Character code for keyboard events, if applicable (e.g., key down). */
    uint32 Character;

    /** @brief The mouse button number (e.g., left=0, right=1, middle=2) for mouse events. */
    int32  MouseButtonNumber;

    /** @brief The key code for keyboard events (e.g., ANSI code). */
    uint16 KeyCode;

    /** @brief Indicates if ContentFrame holds a captured geometry, which only geometry notifications do. */
    bool bHasContentFrame;

    /** @brief Indicates if the scroll deltas are precise (e.g., from a trackpad). */
    bool bHasPreciseScrollingDeltas;

    /** @brief Indicates if this key event is a repeated event (a key held down). */
    bool bIsRepeat;
};

struct FMacScreenInfo
{
    /** @brief The full resolution frame of the monitor, in Cocoa coordinates. */
    NSRect Frame;

    /** @brief The usable frame of the monitor, excluding the menu-bar and the dock. */
    NSRect VisibleFrame;

    /** @brief The scale factor between points and backing store pixels. */
    CGFloat BackingScaleFactor;

    /** @brief The dots-per-inch of the monitor. */
    uint32 DisplayDPI;

    /** @brief A human readable name for the monitor. */
    String DeviceName;

    /** @brief True if this is the primary monitor. */
    bool bIsPrimary;
};

class COREAPPLICATION_API FMacApplication final : public IPlatformApplication
{
public:
    static TSharedPtr<IPlatformApplication> Create();

    static String FindMonitorName(NSScreen* Screen);
    static uint32 MonitorDPIFromScreen(NSScreen* Screen);
    static NSPoint ConvertCocoaPointToEngine(CGFloat PositionX, CGFloat PositionY);
    static NSPoint ConvertEnginePointToCocoa(CGFloat PositionX, CGFloat PositionY);
    static NSRect ConvertEngineRectToCocoa(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY);
    static NSRect ConvertCocoaRectToEngine(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY);

public:
    FMacApplication(const TSharedPtr<FMacCursor>& InCursor);
    virtual ~FMacApplication();

    // IPlatformApplication Interface
    virtual TSharedRef<IPlatformWindow> CreateWindow() override final;

    virtual void Tick(float Delta) override final;
    virtual void ProcessEvents() override final;
    virtual void ProcessDeferredEvents() override final;

    virtual void UpdateInputDevices() override final;
    virtual IPlatformInputDevice* GetInputDevice() override final;

    virtual bool SupportsHighPrecisionMouse() const override final;
    virtual bool SetHighPrecisionMouseMode(const TSharedRef<IPlatformWindow>& Window, EHighPrecisionMouseMode Mode) override final;
    virtual bool ConfineCursorToRect(const TSharedRef<IPlatformWindow>& Window, const IntVector2& Position, const IntVector2& Size) override final;
    virtual void ReleaseCursorConfinement() override final;

    virtual FModifierKeyState GetModifierKeyState() const override final;

    virtual void SetActiveWindow(const TSharedRef<IPlatformWindow>& Window) override final;
    virtual TSharedRef<IPlatformWindow> GetActiveWindow() const override final;
    virtual void SetCapture(const TSharedRef<IPlatformWindow>& Window) override final;
    virtual TSharedRef<IPlatformWindow> GetCapture() const override final;
    virtual TSharedRef<IPlatformWindow> GetWindowUnderCursor() const override final;

    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const override final;

    virtual void SetMessageHandler(const TSharedPtr<IPlatformApplicationMessageHandler>& InMessageHandler) override final;

    virtual TSharedPtr<IPlatformApplicationMessageHandler> GetMessageHandler() const override final
    {
        return MessageHandler;
    }

    virtual TSharedPtr<IPlatformCursor> GetCursor() const override final
    {
        return MacCursor;
    }

    void DeferEvent(NSObject* EventObject);
    NSEvent* OnNSEvent(NSEvent* Event);

    FCocoaWindow* FindNSWindowUnderCursor() const;
    TSharedRef<FMacWindow> FindWindowFromNSWindow(NSWindow* Window) const;
    void UpdateWindowUnderCursor();

    void OnWindowDestroyed(const TSharedRef<FMacWindow>& Window);
    void OnWindowWillResize(const TSharedRef<FMacWindow>& Window);
    void CloseWindow(const TSharedRef<FMacWindow>& Window);

    void RefreshScreenCache();

    FMacApplicationObserver* GetApplicationObserver() const
    {
        return Observer;
    }

private:
    static const FMacScreenInfo* FindScreenFromCocoaPoint(CGFloat PositionX, CGFloat PositionY);
    static const FMacScreenInfo* FindScreenFromEnginePoint(CGFloat PositionX, CGFloat PositionY);

    void ProcessDeferredEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseMoveEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseButtonEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseScrollEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessMouseHoverEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessKeyEvent(const FDeferredMacEvent& DeferredEvent);
    void ProcessUpdatedModfierFlags(const FDeferredMacEvent& DeferredEvent);
    void ProcessModfierKey(EMacModifierKey::Type MacModifierKey, uint64 ModifierKeyFlags, uint64 PreviousModifierKeyFlags);
    void ProcessWindowResized(const FDeferredMacEvent& DeferredEvent);
    void ProcessWindowMoved(const FDeferredMacEvent& DeferredEvent);

    void ClampCursorToConfinement();

    id                                             LocalEventMonitor;
    id                                             GlobalMouseMovedEventMonitor;
    FMacApplicationObserver*                       Observer;
    FCocoaWindow*                                  WindowUnderCursor;
    mutable FCriticalSection                       WindowUnderCursorCS;
    FCocoaWindow*                                  CapturedWindow;
    mutable FCriticalSection                       CapturedWindowCS;
    NSUInteger                                     CurrentModifierFlags;
    EMouseButtonName::Type                         LastPressedButton;
    Vector2                                        HighPrecisionMouseRemainder;
    IntVector2                                     CursorConfinementPosition;
    IntVector2                                     CursorConfinementSize;
    bool                                           bHighPrecisionMouseEnabled;
    bool                                           bCursorConfined;
    TSharedPtr<FMacCursor>                         MacCursor;
    TSharedPtr<FGCInputDevice>                     InputDevice;
    TArray<FMacScreenInfo>                         ScreenCache;
    mutable FCriticalSection                       ScreenCacheCS;
    TArray<TSharedRef<FMacWindow>>                 Windows;
    mutable FCriticalSection                       WindowsCS;
    TArray<FCocoaWindow*>                          ClosedCocoaWindows;
    FCriticalSection                               ClosedCocoaWindowsCS;
    TArray<TSharedRef<FMacWindow>>                 ClosedWindows;
    FCriticalSection                               ClosedWindowsCS;
    TArray<FDeferredMacEvent>                      DeferredEvents;
    FCriticalSection                               DeferredEventsCS;
    TSharedPtr<IPlatformApplicationMessageHandler> MessageHandler;
};

extern FMacApplication* GMacApplication;
