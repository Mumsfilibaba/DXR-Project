#pragma once
#include "Core/Mac/Mac.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Mac/MacCursor.h"
#include "CoreApplication/Mac/GCInputDevice.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/GenericApplication.h"
#include <AppKit/AppKit.h>

@class FCocoaWindow;
@class FMacApplicationObserver;
class FMacWindow;
class FGenericWindow;

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
            [NotificationName release];
            [Event release];
            [Window release];
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
    
    /**
     * @brief Creates a new MacApplication instance and returns a GenericApplication interface.
     * 
     * This function also initializes the global 'GMacApplication' pointer since the constructor and destructor
     * control the value of the global pointer.
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

    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) override final;

    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const override final;

    virtual TSharedRef<FGenericWindow> GetActiveWindow() const override final;

    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const override final;

    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler) override final;

public:
    
    /**
     * @brief Defers an event to be processed later in the tick function.
     * 
     * @param EventObject The event object to defer, which can be an NSEvent, NSNotificationName, or NSString based on the event type.
     */
    void DeferEvent(NSObject* EventObject);

    /**
     * @brief Handles an NSEvent by deferring it for later processing.
     * 
     * Called from the local event monitor when an event occurs; this function defers the event to be processed
     * during the next processing of deferred events.
     * 
     * @param Event The NSEvent to handle.
     * @return The processed NSEvent.
     */
    NSEvent* OnNSEvent(NSEvent* Event);
        
    /**
     * @brief Handles the destruction of a window.
     * 
     * @param Window A shared reference to the FMacWindow that is being destroyed.
     */
    void OnWindowDestroyed(const TSharedRef<FMacWindow>& Window);

    /**
     * @brief Handles the window resize event before the window is resized.
     * 
     * @param Window A shared reference to the FMacWindow that will be resized.
     */
    void OnWindowWillResize(const TSharedRef<FMacWindow>& Window);

    /**
     * @brief Processes a deferred event.
     * 
     * @param DeferredEvent The deferred event to process.
     */
    void ProcessDeferredEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a mouse move event.
     * 
     * @param DeferredEvent The deferred mouse move event to process.
     */
    void ProcessMouseMoveEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a mouse button event.
     * 
     * @param DeferredEvent The deferred mouse button event to process.
     */
    void ProcessMouseButtonEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a mouse scroll event.
     * 
     * @param DeferredEvent The deferred mouse scroll event to process.
     */
    void ProcessMouseScrollEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a mouse hover event.
     * 
     * @param DeferredEvent The deferred mouse hover event to process.
     */
    void ProcessMouseHoverEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a key event.
     * 
     * @param DeferredEvent The deferred key event to process.
     */
    void ProcessKeyEvent(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a window resized event.
     * 
     * @param DeferredEvent The deferred window resized event to process.
     */
    void ProcessWindowResized(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Processes a window moved event.
     * 
     * @param DeferredEvent The deferred window moved event to process.
     */
    void ProcessWindowMoved(const FDeferredMacEvent& DeferredEvent);

    /**
     * @brief Finds the NSWindow currently under the cursor.
     * 
     * This function uses the current window under the cursor, checks if that window is FCocoaWindow,
     * and returns a valid pointer if these conditions are met. Otherwise, nullptr is returned.
     * This means that the function can return nullptr even if the cursor is hovering over a window,
     * just that this window is not a FCocoaWindow.
     * 
     * @return A pointer to the FCocoaWindow under the cursor, or nullptr if none is found.
     */
    FCocoaWindow* FindNSWindowUnderCursor() const;

    /**
     * @brief Finds a FMacWindow instance from a given NSWindow. If the NSWindow is not a FCocoaWindow, the function simply returns nullptr.
     * @param Window The NSWindow to find the corresponding FMacWindow for.
     * @return A shared reference to the FMacWindow associated with the NSWindow.
     */
    TSharedRef<FMacWindow> FindWindowFromNSWindow(NSWindow* Window) const;

    /**
     * @brief Closes the specified window.
     * 
     * By closing a window, this function enqueues an invocation of the MessageHandler's OnWindowClosed event.
     * This will in turn later call FMacWindow's destroy in the upper engine layers, which then finally enqueues
     * the removal of the FMacWindow and destruction of the FCocoaWindow. This is done to ensure that all engine
     * systems can respond to the window's destruction properly.
     * 
     * @param Window A shared reference to the FMacWindow to be closed.
     */
    void CloseWindow(const TSharedRef<FMacWindow>& Window);

    /**
     * @brief Retrieves the application observer instance.
     * 
     * @return A pointer to the FMacApplicationObserver instance.
     */
    FMacApplicationObserver* GetApplicationObserver() const
    {
        return Observer;
    }

public:

    /**
     * @brief Retrieves the name of the monitor represented by the given NSScreen object.
     * @param Screen The NSScreen object representing the monitor.
     * @return An FString containing the name of the monitor.
     */
    static FString FindMonitorName(NSScreen* Screen);

    /**
     * @brief Calculates the DPI (Dots Per Inch) for the specified NSScreen.
     * 
     * @param Screen The NSScreen object for which to calculate the DPI.
     * @return The DPI value as an unsigned 32-bit integer.
     */
    static uint32 MonitorDPIFromScreen(NSScreen* Screen);

    /**
     * @brief Finds an NSScreen based on a position in the Cocoa coordinate system.
     * 
     * @param PositionX The X-coordinate in the Cocoa coordinate system.
     * @param PositionY The Y-coordinate in the Cocoa coordinate system.
     * @return A pointer to the NSScreen that contains the specified point.
     */
    static NSScreen* FindScreenFromCocoaPoint(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Finds an NSScreen based on a position in the Engine coordinate system.
     * 
     * @param PositionX The X-coordinate in the Engine coordinate system.
     * @param PositionY The Y-coordinate in the Engine coordinate system.
     * @return A pointer to the NSScreen that contains the specified point.
     */
    static NSScreen* FindScreenFromEnginePoint(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a point from the Cocoa coordinate system to the Engine coordinate system.
     * 
     * @param PositionX The X-coordinate in the Cocoa coordinate system.
     * @param PositionY The Y-coordinate in the Cocoa coordinate system.
     * @return The converted NSPoint in the Engine coordinate system.
     */
    static NSPoint ConvertCocoaPointToEngine(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a point from the Engine coordinate system to the Cocoa coordinate system.
     * 
     * @param PositionX The X-coordinate in the Engine coordinate system.
     * @param PositionY The Y-coordinate in the Engine coordinate system.
     * @return The converted NSPoint in the Cocoa coordinate system.
     */
    static NSPoint ConvertEnginePointToCocoa(CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a rectangle from the Engine coordinate system to the Cocoa coordinate system.
     * 
     * @param Width The width of the rectangle in the Engine coordinate system.
     * @param Height The height of the rectangle in the Engine coordinate system.
     * @param PositionX The X-coordinate of the rectangle in the Engine coordinate system.
     * @param PositionY The Y-coordinate of the rectangle in the Engine coordinate system.
     * @return The converted NSRect in the Cocoa coordinate system.
     */
    static NSRect ConvertEngineRectToCocoa(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY);

    /**
     * @brief Converts a rectangle from the Cocoa coordinate system to the Engine coordinate system.
     * 
     * @param Width The width of the rectangle in the Cocoa coordinate system.
     * @param Height The height of the rectangle in the Cocoa coordinate system.
     * @param PositionX The X-coordinate of the rectangle in the Cocoa coordinate system.
     * @param PositionY The Y-coordinate of the rectangle in the Cocoa coordinate system.
     * @return The converted NSRect in the Engine coordinate system.
     */
    static NSRect ConvertCocoaRectToEngine(CGFloat Width, CGFloat Height, CGFloat PositionX, CGFloat PositionY);
        
private:

    id LocalEventMonitor;
    id GlobalMouseMovedEventMonitor;

    FMacApplicationObserver* Observer;
    FCocoaWindow*            WindowUnderCursor;

    NSUInteger             PreviousModifierFlags;
    EMouseButtonName::Type LastPressedButton;

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

/** 
 * @brief Global pointer to the FMacApplication instance.
 * This pointer is used throughout the application to access the Mac-specific application functionalities.
 */
extern FMacApplication* GMacApplication;
