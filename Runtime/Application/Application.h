#pragma once
#include "Core/Containers/Set.h"
#include "Core/Delegates/Event.h"
#include "CoreApplication/Generic/ICursor.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"
#include "Application/InputHandler.h"
#include "Application/WidgetPath.h"
#include "Application/Widgets/Window.h"

/** 
 * @brief Event triggered when the monitor configuration changes (e.g., adding or removing displays).
 */
DECLARE_EVENT(FOnMonitorConfigChangedEvent, FApplicationInterface);

/**
 * @class FApplicationInterface
 * @brief Central application class that handles event routing, window management, input processing, and more.
 *
 * FApplicationInterface extends FGenericApplicationMessageHandler to receive and process various input and windowing events.
 * It also manages a platform-specific application (FGenericApplication) and interacts with a set of FWindow objects. 
 * The class is intended to be a singleton-like interface that can be accessed throughout the engine via Get().
 */
class APPLICATION_API FApplicationInterface : public FGenericApplicationMessageHandler , public TSharedFromThis<FApplicationInterface>
{
public:

    /**
     * @brief Creates the singleton instance of FApplicationInterface and the associated PlatformApplication.
     * 
     * @return True if both the FApplicationInterface and PlatformApplication instances were successfully created, otherwise false.
     */
    static bool Create();

    /**
     * @brief Destroys the singleton FApplicationInterface and PlatformApplication instances.
     *
     * Cleans up resources, windows, and any other data allocated in Create().
     */
    static void Destroy();

    /**
     * @brief Checks if the FApplicationInterface instance has been created.
     * 
     * @return True if the instance is valid (i.e., the application is initialized), otherwise false.
     */
    static bool FORCEINLINE IsInitialized()
    {
        return GApplicationInstance.IsValid();
    }

    /**
     * @brief Retrieves a reference to the FApplicationInterface singleton.
     * 
     * This function checks that the instance is valid before returning a reference.
     * @return A reference to the FApplicationInterface instance.
     */
    static FORCEINLINE FApplicationInterface& Get()
    {
        CHECK(GApplicationInstance.IsValid());
        return *GApplicationInstance;
    }
    
public:

    FApplicationInterface();
    virtual ~FApplicationInterface();

public:

    // FGenericApplicationMessageHandler Interface Overrides
    virtual bool OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex) override final;

    virtual bool OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat) override final;

    virtual bool OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue) override final;

    virtual bool OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModifierKeyState) override final;

    virtual bool OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModifierKeyState) override final;

    virtual bool OnKeyChar(uint32 Character) override final;

    virtual bool OnMouseMove(int32 MouseX, int32 MouseY) override final;

    virtual bool OnMouseButtonDown(const TSharedRef<FGenericWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState) override final;

    virtual bool OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState) override final;

    virtual bool OnMouseButtonDoubleClick(EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState) override final;

    virtual bool OnMouseScrolled(float WheelDelta, bool bVertical) override final;

    virtual bool OnMouseEntered() override final;

    virtual bool OnMouseLeft() override final;

    virtual bool OnHighPrecisionMouseInput(int32 MouseX, int32 MouseY) override final;

    virtual bool OnWindowResized(const TSharedRef<FGenericWindow>& Window, uint32 Width, uint32 Height) override final;

    virtual bool OnWindowResizing(const TSharedRef<FGenericWindow>& Window) override final;

    virtual bool OnWindowMoved(const TSharedRef<FGenericWindow>& Window, int32 MouseX, int32 MouseY) override final;

    virtual bool OnWindowFocusLost(const TSharedRef<FGenericWindow>& Window) override final;

    virtual bool OnWindowFocusGained(const TSharedRef<FGenericWindow>& Window) override final;

    virtual bool OnWindowClosed(const TSharedRef<FGenericWindow>& Window) override final;

    virtual bool OnMonitorConfigurationChange() override final;

public:

    /**
     * @brief Adds a new window to the application and creates its underlying platform window.
     * 
     * After creation, the window will be managed and ticked each frame. The platform-specific window
     * representation will also be shown.
     * @param InWindow The FWindow object describing the new window.
     */
    void CreateWindow(const TSharedPtr<FWindow>& InWindow);

    /**
     * @brief Destroys a managed window and its underlying platform window.
     * 
     * @param InWindow The FWindow to destroy.
     */
    void DestroyWindow(const TSharedPtr<FWindow>& InWindow);

    /**
     * @brief Updates all windows, processes queued messages, and updates input devices.
     * 
     * @param Delta The time (in seconds) elapsed since the last tick.
     */
    void Tick(float Delta);

    /**
     * @brief Processes OS-level or platform-level events.
     *
     * This method typically drives the platform message pump, distributing events to the rest of the system.
     */
    void ProcessEvents();

    /**
     * @brief Processes events that have been deferred for later handling.
     *
     * Some events may be batched or scheduled to run after other operations (e.g., to avoid reentrancy issues).
     */
    void ProcessDeferredEvents();

    /**
     * @brief Updates input devices not handled via standard platform events.
     *
     * This could include specialized controllers or future plugin-based devices. 
     */
    void UpdateInputDevices();

    /**
     * @brief Updates the cached monitor information if the platform reported a monitor setup change.
     *
     * This includes gathering information such as resolution, DPI, and primary monitor status.
     */
    void UpdateMonitorInfo();

    /**
     * @brief Registers a new input handler with the application.
     *
     * Input handlers can intercept and process input events before they reach the default logic.
     * @param InputHandler The input handler to register.
     */
    void RegisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    /**
     * @brief Unregisters an input handler from the application.
     *
     * @param InputHandler The input handler to remove.
     */
    void UnregisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    /**
     * @brief Enables high-precision mouse input (raw input) for a specified window, if the platform supports it.
     * 
     * On Windows, this leverages raw input events. On other platforms, this may be unavailable.
     * @param Window The window to enable raw input on.
     * @return True if successfully enabled, otherwise false.
     */
    bool EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window);

    /**
     * @brief Retrieves the current modifier key state (e.g., whether Ctrl, Alt, or Shift are pressed).
     * 
     * @return A struct that contains the current state of modifier keys.
     */
    FModifierKeyState GetModifierKeyState() const;

    /**
     * @brief Checks if the application supports high-precision mouse input (raw input).
     * 
     * @return True if high-precision mouse input is supported, otherwise false.
     */
    bool SupportsHighPrecisionMouse() const;

    /**
     * @brief Sets the global cursor position.
     * 
     * Moves the system cursor to the specified position in screen coordinates.
     * @param Position The new absolute screen coordinates for the cursor.
     */
    void SetCursorPosition(const FIntVector2& Position);

    /**
     * @brief Retrieves the global cursor position in screen coordinates.
     * 
     * @return The current (X, Y) position of the system cursor.
     */
    FIntVector2 GetCursorPosition() const;

    /**
     * @brief Sets the cursor appearance (pointer, hand, crosshair, etc.).
     * 
     * @param Cursor The cursor enum representing the desired cursor shape.
     */
    void SetCursor(ECursor Cursor);

    /**
     * @brief Shows or hides the system cursor.
     * 
     * @param bIsVisible True to show the cursor, false to hide it.
     */
    void ShowCursor(bool bIsVisible);

    /**
     * @brief Checks if the system cursor is currently visible.
     * 
     * @return True if the cursor is visible, otherwise false.
     */
    bool IsCursorVisible() const;

    /**
     * @brief Checks if a gamepad is currently connected.
     * 
     * Internally checks if an FInputDevice is registered and if that device reports as connected.
     * @return True if a gamepad is connected, otherwise false.
     */
    bool IsGamePadConnected() const;

    /**
     * @brief Checks if the application is currently tracking a mouse drag operation.
     * 
     * This is set to true when the left mouse button is pressed, and remains true until it is released.
     * @return True if a mouse drag operation is in progress, otherwise false.
     */
    bool IsTrackingCursor() const { return bIsTrackingCursor; }
    
    /**
     * @brief Overrides the existing platform application with a new FGenericApplication instance.
     * 
     * @param InPlatformApplication The new platform application to set.
     */
    void OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication);
    
    /**
     * @brief Retrieves the current platform application interface.
     * 
     * @return A shared pointer to the current FGenericApplication.
     */
    TSharedPtr<FGenericApplication> GetPlatformApplication() const { return GPlatformApplication; }

    /**
     * @brief Retrieves the primary input device interface (e.g., for gamepads).
     * 
     * @return A pointer to the current FInputDevice instance, or nullptr if none.
     */
    FInputDevice* GetInputDevice() const { return GPlatformApplication->GetInputDevice(); }

    /**
     * @brief Retrieves the cursor interface being used by the platform application.
     * 
     * @return A shared pointer to the ICursor interface, or nullptr if unsupported.
     */
    TSharedPtr<ICursor> GetCursor() const { return GPlatformApplication->Cursor; }

    /**
     * @brief Gets the window that currently has focus (for receiving keyboard input, etc.).
     * 
     * @return A shared pointer to the focused window, or nullptr if none.
     */
    TSharedPtr<FWindow> GetFocusWindow() const;

    /**
     * @brief Sets focus to the specified widget and all of its parents up to the top-level window.
     * 
     * @param FocusWidget The widget that should receive focus.
     */
    void SetFocusWidget(const TSharedPtr<FWidget>& FocusWidget);

    /**
     * @brief Sets a new widget path as the focus hierarchy.
     * 
     * This path typically contains the target widget and all parent widgets along the path to a window.
     * @param NewFocusPath The widget path to set focus to.
     */
    void SetFocusWidgets(const FWidgetPath& NewFocusPath);

    /**
     * @brief Finds the lowest-level window (top-level FWindow) that contains the specified widget.
     * 
     * @param InWidget The widget to search for.
     * @return A shared pointer to the top-level FWindow that contains the widget, or nullptr if not found.
     */
    TSharedPtr<FWindow> FindWindowWidget(const TSharedPtr<FWidget>& InWidget);
    
    /**
     * @brief Finds the FWindow that corresponds to a given platform window (FGenericWindow).
     * 
     * @param PlatformWindow The FGenericWindow to match against known windows.
     * @return A shared pointer to the corresponding FWindow, or nullptr if not found.
     */
    TSharedPtr<FWindow> FindWindowFromGenericWindow(const TSharedRef<FGenericWindow>& PlatformWindow) const;
    
    /**
     * @brief Returns the window currently under the mouse cursor.
     * 
     * @return A shared pointer to the FWindow under the cursor, or nullptr if none.
     */
    TSharedPtr<FWindow> FindWindowUnderCursor() const;

    /**
     * @brief Retrieves a path of widgets currently under the mouse cursor.
     * 
     * @param OutCursorPath A widget path object that will be populated with the widgets under the cursor.
     */
    void FindWidgetsUnderCursor(FWidgetPath& OutCursorPath);
    
    /**
     * @brief Populates a widget path with widgets that lie under a specific screen coordinate.
     * 
     * @param Point A 2D screen coordinate (X, Y).
     * @param OutCursorPath A widget path object to populate.
     */
    void FindWidgetsUnderCursor(const FIntVector2& Point, FWidgetPath& OutCursorPath);

    /**
     * @brief Retrieves cached monitor/display information (e.g., resolution, DPI).
     * 
     * @param OutMonitorInfo An array to receive the available monitor configurations.
     */
    void GetDisplayInfo(TArray<FMonitorInfo>& OutMonitorInfo);

    /**
     * @brief Accessor for the monitor configuration changed event.
     * 
     * @return A reference to the event triggered when monitors are added/removed or their configuration changes.
     */
    FOnMonitorConfigChangedEvent& GetOnMonitorConfigChangedEvent()
    {
        return OnMonitorConfigChangedEvent;
    }

private:
    TSet<EKeyboardKeyName::Type> PressedKeys;
    TSet<EMouseButtonName::Type> PressedMouseButtons;
    TArray<FMonitorInfo>         MonitorInfos;

    bool bIsMonitorInfoValid;
    bool bIsTrackingCursor;

    FWidgetPath FocusPath;
    FWidgetPath TrackedWidgets;

    TArray<TSharedPtr<FWindow>>       Windows;
    TArray<TSharedPtr<FInputHandler>> InputHandlers;
    FOnMonitorConfigChangedEvent      OnMonitorConfigChangedEvent;

    static TSharedPtr<FGenericApplication>   GPlatformApplication;
    static TSharedPtr<FApplicationInterface> GApplicationInstance;
};
