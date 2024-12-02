#pragma once
#include "Core/Containers/Set.h"
#include "Core/Delegates/Event.h"
#include "CoreApplication/Generic/ICursor.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"
#include "Application/InputHandler.h"
#include "Application/WidgetPath.h"
#include "Application/Widgets/Window.h"

/** @brief Event triggered when the monitor configuration changes. */
DECLARE_EVENT(FOnMonitorConfigChangedEvent, FApplicationInterface);

class APPLICATION_API FApplicationInterface : public FGenericApplicationMessageHandler, public TSharedFromThis<FApplicationInterface>
{
public:

    /**
     * @brief Creates the ApplicationInterface instance and the PlatformApplication instance.
     * @return True if the instances were successfully created; false otherwise.
     */
    static bool Create();

    /**
     * @brief Destroys the ApplicationInterface instance and the PlatformApplication instance.
     */
    static void Destroy();

    /**
     * @brief Checks if the ApplicationInterface instance has been created.
     * @return True if the ApplicationInterface instance is created; false otherwise.
     */
    static bool FORCEINLINE IsInitialized()
    {
        return ApplicationInstance.IsValid();
    }

    /**
     * @brief Retrieves a reference to the ApplicationInterface instance.
     * 
     * Before the reference is returned, the function checks that the instance is initialized.
     * @return Reference to the ApplicationInterface instance.
     */
    static FORCEINLINE FApplicationInterface& Get()
    {
        CHECK(ApplicationInstance.IsValid());
        return *ApplicationInstance;
    }
    
public:

    /**
     * @brief Constructs the FApplicationInterface.
     */
    FApplicationInterface();

    /**
     * @brief Destructor for FApplicationInterface.
     */
    virtual ~FApplicationInterface();

    // FGenericApplicationMessageHandler Interface

    /**
     * @brief Handles gamepad button up events.
     * @param Button The gamepad button that was released.
     * @param GamepadIndex The index of the gamepad.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex) override final;

    /**
     * @brief Handles gamepad button down events.
     * @param Button The gamepad button that was pressed.
     * @param GamepadIndex The index of the gamepad.
     * @param bIsRepeat True if the event is a repeat; false otherwise.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat) override final;

    /**
     * @brief Handles analog gamepad input changes.
     * @param AnalogSource The analog input source that changed.
     * @param GamepadIndex The index of the gamepad.
     * @param AnalogValue The new value of the analog input.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue) override final;

    /**
     * @brief Handles key up events.
     * @param KeyCode The key code of the key that was released.
     * @param ModifierKeyState The state of modifier keys (Shift, Ctrl, Alt).
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModifierKeyState) override final;

    /**
     * @brief Handles key down events.
     * @param KeyCode The key code of the key that was pressed.
     * @param bIsRepeat True if the event is a repeat; false otherwise.
     * @param ModifierKeyState The state of modifier keys (Shift, Ctrl, Alt).
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModifierKeyState) override final;

    /**
     * @brief Handles character input events.
     * @param Character The Unicode character code.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnKeyChar(uint32 Character) override final;

    /**
     * @brief Handles mouse movement events.
     * @param MouseX The new X position of the mouse.
     * @param MouseY The new Y position of the mouse.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnMouseMove(int32 MouseX, int32 MouseY) override final;

    /**
     * @brief Handles mouse button down events.
     * @param PlatformWindow The window where the event occurred.
     * @param Button The mouse button that was pressed.
     * @param ModifierKeyState The state of modifier keys (Shift, Ctrl, Alt).
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnMouseButtonDown(const TSharedRef<FGenericWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState) override final;

    /**
     * @brief Handles mouse button up events.
     * @param Button The mouse button that was released.
     * @param ModifierKeyState The state of modifier keys (Shift, Ctrl, Alt).
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState) override final;

    /**
     * @brief Handles mouse button double-click events.
     * @param Button The mouse button that was double-clicked.
     * @param ModifierKeyState The state of modifier keys (Shift, Ctrl, Alt).
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnMouseButtonDoubleClick(EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState) override final;

    /**
     * @brief Handles mouse scroll events.
     * @param WheelDelta The amount of wheel rotation.
     * @param bVertical True if the scroll is vertical; false if horizontal.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnMouseScrolled(float WheelDelta, bool bVertical) override final;

    /**
     * @brief Handles mouse entered events.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnMouseEntered() override final;

    /**
     * @brief Handles mouse left events.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnMouseLeft() override final;

    /**
     * @brief Handles high-precision mouse input events.
     * @param MouseX The high-precision X movement.
     * @param MouseY The high-precision Y movement.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnHighPrecisionMouseInput(int32 MouseX, int32 MouseY) override final;

    /**
     * @brief Handles window resized events.
     * @param Window The window that was resized.
     * @param Width The new width of the window.
     * @param Height The new height of the window.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnWindowResized(const TSharedRef<FGenericWindow>& Window, uint32 Width, uint32 Height) override final;

    /**
     * @brief Handles window resizing events.
     * @param Window The window that is being resized.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnWindowResizing(const TSharedRef<FGenericWindow>& Window) override final;

    /**
     * @brief Handles window moved events.
     * @param Window The window that was moved.
     * @param MouseX The new X position of the window.
     * @param MouseY The new Y position of the window.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnWindowMoved(const TSharedRef<FGenericWindow>& Window, int32 MouseX, int32 MouseY) override final;

    /**
     * @brief Handles window focus lost events.
     * @param Window The window that lost focus.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnWindowFocusLost(const TSharedRef<FGenericWindow>& Window) override final;

    /**
     * @brief Handles window focus gained events.
     * @param Window The window that gained focus.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnWindowFocusGained(const TSharedRef<FGenericWindow>& Window) override final;

    /**
     * @brief Handles window closed events.
     * @param Window The window that was closed.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnWindowClosed(const TSharedRef<FGenericWindow>& Window) override final;

    /**
     * @brief Handles monitor configuration change events.
     * @return True if the event was handled; false otherwise.
     */
    virtual bool OnMonitorConfigurationChange() override final;

public:
    
    /**
     * @brief Adds a new window to the application.
     * 
     * Adding a window means that it will be processed each frame within FApplicationInterface::Tick.
     * This function also creates and displays a platform window based on the information set in the FWindow.
     * @param InWindow The window to add.
     */
    void CreateWindow(const TSharedPtr<FWindow>& InWindow);

    /**
     * @brief Destroys a window.
     * 
     * It also destroys the platform window, which will stop displaying the window.
     * @param InWindow The window to destroy.
     */
    void DestroyWindow(const TSharedPtr<FWindow>& InWindow);

    /**
     * @brief Updates all windows, pumps the message queue, and processes input devices.
     * 
     * This function ensures that the application moves forward and handles all the events that the platform has sent
     * to the application since the last call to FApplicationInterface::Tick.
     * @param Delta The time elapsed since the last tick.
     */
    void Tick(float Delta);

    /**
     * @brief Updates any game controllers or devices that do not go through the standard platform message pump.
     * 
     * Might support plugins for different input devices in the future.
     */
    void UpdateInputDevices();

    /**
     * @brief Updates the monitor information.
     * 
     * It queries the information from the platform if the platform has notified the application that the monitor setup has changed.
     */
    void UpdateMonitorInfo();

    /**
     * @brief Registers a new input handler.
     * @param InputHandler The input handler to register.
     */
    void RegisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    /**
     * @brief Unregisters an input handler.
     * @param InputHandler The input handler to unregister.
     */
    void UnregisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    /**
     * @brief Enables high-precision mouse input for the specified window.
     * 
     * This essentially enables input via the FWidget::OnHighPrecisionMouseInput function to be called for the selected
     * window if the platform supports high-precision mouse input (on Windows, this is raw input).
     * @param Window The window to enable high-precision mouse input for.
     * @return True if high-precision mouse input was successfully enabled; false otherwise.
     */
    bool EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window);

    /**
     * @brief Checks if the application supports high-precision mouse input.
     * 
     * On Windows, this corresponds to raw input.
     * @return True if high-precision mouse input is supported; false otherwise.
     */
    bool SupportsHighPrecisionMouse() const;

    /**
     * @brief Sets the global cursor position.
     * @param Position The new cursor position.
     */
    void SetCursorPosition(const FIntVector2& Position);

    /**
     * @brief Retrieves the global cursor position.
     * @return The current cursor position.
     */
    FIntVector2 GetCursorPosition() const;

    /**
     * @brief Sets the cursor type (appearance).
     * @param Cursor The cursor type to set.
     */
    void SetCursor(ECursor Cursor);

    /**
     * @brief Shows or hides the cursor.
     * @param bIsVisible True to show the cursor; false to hide it.
     */
    void ShowCursor(bool bIsVisible);

    /**
     * @brief Checks if the cursor is currently visible.
     * @return True if the cursor is visible; false otherwise.
     */
    bool IsCursorVisible() const;

    /**
     * @brief Checks if there is currently a gamepad connected.
     * 
     * Returns false if there is no valid FInputDevice interface.
     * @return True if a gamepad is connected; false otherwise.
     */
    bool IsGamePadConnected() const;

    /**
     * @brief Checks if the application is currently tracking a mouse drag operation.
     * 
     * This happens when the left mouse button is pressed down, and this function will return true until the mouse button is released.
     * @return True if a mouse drag operation is being tracked; false otherwise.
     */
    bool IsTrackingCursor() const { return bIsTrackingCursor; }
    
    /**
     * @brief Overrides the current platform application with a new FGenericApplication instance.
     * @param InPlatformApplication The new platform application to set.
     */
    void OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication);
    
    /**
     * @brief Returns the current platform application (FGenericApplication instance).
     * @return A shared pointer to the current platform application.
     */
    TSharedPtr<FGenericApplication> GetPlatformApplication() const { return PlatformApplication; }

    /**
     * @brief Returns the current FInputDevice interface.
     * @return A pointer to the input device interface.
     */
    FInputDevice* GetInputDevice() const { return PlatformApplication->GetInputDevice(); }

    /**
     * @brief Returns the cursor interface.
     * @return A shared pointer to the cursor interface.
     */
    TSharedPtr<ICursor> GetCursor() const { return PlatformApplication->Cursor; }

    /**
     * @brief Gets the current window that has focus.
     * @return A shared pointer to the focus window.
     */
    TSharedPtr<FWindow> GetFocusWindow() const;

    /**
     * @brief Sets the widget that should currently have focus.
     * 
     * This function will first find a widget path until it reaches the lowest-level window.
     * It will then set focus to this widget path. In practice, this means it will set focus to the specified widget,
     * all its parent widgets, and finally the window containing these widgets.
     * @param FocusWidget The widget to set focus to.
     */
    void SetFocusWidget(const TSharedPtr<FWidget>& FocusWidget);

    /**
     * @brief Sets the new widget path which will now have focus.
     * 
     * All these widgets will receive key events (including gamepad events) after a call to this function.
     * @param NewFocusPath The widget path to set focus to.
     */
    void SetFocusWidgets(const FWidgetPath& NewFocusPath);

    /**
     * @brief Finds the lowest-level window that contains the specified widget.
     * 
     * This function will find a path to the lowest-level window and then return that window.
     * @param InWidget The widget to search for.
     * @return A shared pointer to the window containing the widget.
     */
    TSharedPtr<FWindow> FindWindowWidget(const TSharedPtr<FWidget>& InWidget);
    
    /**
     * @brief Returns the window that corresponds to the specified platform window.
     * @param PlatformWindow The platform window to find.
     * @return A shared pointer to the corresponding window.
     */
    TSharedPtr<FWindow> FindWindowFromGenericWindow(const TSharedRef<FGenericWindow>& PlatformWindow) const;
    
    /**
     * @brief Returns the window that is currently under the cursor.
     * @return A shared pointer to the window under the cursor.
     */
    TSharedPtr<FWindow> FindWindowUnderCursor() const;

    /**
     * @brief Finds the widgets that are currently under the cursor.
     * @param OutCursorPath The widget path to populate with widgets under the cursor.
     */
    void FindWidgetsUnderCursor(FWidgetPath& OutCursorPath);
    
    /**
     * @brief Finds the widgets that are under a specific point.
     * 
     * Returns a path of widgets that is currently under the cursor and contains the specified point.
     * @param Point The point to check.
     * @param OutCursorPath The widget path to populate with widgets under the point.
     */
    void FindWidgetsUnderCursor(const FIntVector2& Point, FWidgetPath& OutCursorPath);

    /**
     * @brief Retrieves cached display information.
     * @param OutMonitorInfo An array to populate with monitor information.
     */
    void GetDisplayInfo(TArray<FMonitorInfo>& OutMonitorInfo);

    /**
     * @brief Retrieves the monitor configuration changed event.
     * @return A reference to the monitor configuration changed event.
     */
    FOnMonitorConfigChangedEvent& GetOnMonitorConfigChangedEvent() { return OnMonitorConfigChangedEvent; }

private:

    /** @brief Set of currently pressed keys. */
    TSet<EKeyboardKeyName::Type> PressedKeys;

    /** @brief Set of currently pressed mouse buttons. */
    TSet<EMouseButtonName::Type> PressedMouseButtons;

    /** @brief Cached monitor information. */
    TArray<FMonitorInfo> MonitorInfos;

    /** @brief Indicates if monitor info is valid. */
    bool bIsMonitorInfoValid;

    /** @brief Indicates if a mouse drag operation is being tracked. */
    bool bIsTrackingCursor;

    /** @brief Array of windows managed by the application. */
    TArray<TSharedPtr<FWindow>> Windows;

    /** @brief The current widget path that has focus. */
    FWidgetPath FocusPath;

    /** @brief Widgets currently being tracked for events. */
    FWidgetPath TrackedWidgets;

    /** @brief Array of registered input handlers. */
    TArray<TSharedPtr<FInputHandler>> InputHandlers;

    /** @brief Event triggered when monitor configuration changes. */
    FOnMonitorConfigChangedEvent OnMonitorConfigChangedEvent;

    /** @brief Static instance of the application interface. */
    static TSharedPtr<FApplicationInterface> ApplicationInstance;
    /** @brief Static instance of the platform application. */
    static TSharedPtr<FGenericApplication> PlatformApplication;
};
