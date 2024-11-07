#pragma once
#include "Core/Containers/Set.h"
#include "Core/Delegates/Event.h"
#include "CoreApplication/Generic/ICursor.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"
#include "Application/InputHandler.h"
#include "Application/WidgetPath.h"
#include "Application/Widgets/Window.h"

DECLARE_EVENT(FOnMonitorConfigChangedEvent, FApplicationInterface);

class APPLICATION_API FApplicationInterface : public FGenericApplicationMessageHandler, public TSharedFromThis<FApplicationInterface>
{
public:

    // Create the ApplicationInterface instance and the PlatformApplication instance
    static bool Create();

    // Destroy the ApplicationInterface instance and the PlatformApplication instance
    static void Destroy();

    // Returns true if the ApplicationInterface instance is created
    static bool FORCEINLINE IsInitialized()
    {
        return ApplicationInstance.IsValid();
    }

    // Retrieve a reference to the ApplicationInterface instance. Before the pointer is dereferenced, the function 
    // checks that the pointer is initialized.
    static FORCEINLINE FApplicationInterface& Get()
    {
        CHECK(ApplicationInstance.IsValid());
        return *ApplicationInstance;
    }
    
public:
    FApplicationInterface();
    virtual ~FApplicationInterface();

    // FGenericApplicationMessageHandler Interface
    virtual bool OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex) override final;
    virtual bool OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat) override final;
    virtual bool OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue) override final;
    virtual bool OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModierKeyState) override final;
    virtual bool OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState) override final;
    virtual bool OnKeyChar(uint32 Character) override final;
    virtual bool OnMouseMove(int32 MouseX, int32 MouseY) override final;
    virtual bool OnMouseButtonDown(const TSharedRef<FGenericWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModierKeyState) override final;
    virtual bool OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModierKeyState) override final;
    virtual bool OnMouseButtonDoubleClick(EMouseButtonName::Type Button, FModifierKeyState ModierKeyState) override final;
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
    
    // Adds a new window. Adding a window means that it will be processed each frame within FApplicationInstance::Tick.
    // This function also creates and displays a platform-window based on the info set in the FWindow.
    void CreateWindow(const TSharedPtr<FWindow>& InWindow);

    // Destroys a window. It also destroys the platform-window which will stop displaying the window.
    void DestroyWindow(const TSharedPtr<FWindow>& InWindow);

    // Updates all windows, pumps the message queue and input devices. This function will ensure that the application
    // moves forward and handles all the events that the platform has sent to the application since the last call to
    // FApplicationInterface::Tick.
    void Tick(float Delta);

    // Updates and game-controller that might be conntected and any other device that does not go through the standard
    // platform message-pump. Might support plugins for different input devices in the future.
    void UpdateInputDevices();

    // This will update the monitor information. It will query the information from the platform if the platform has
    // notified the application that the monitor-setup has changed.
    void UpdateMonitorInfo();

    // Registers a new input-handler
    void RegisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    // Unregisters a input-handler
    void UnregisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    // Enables high-precision mouse input for the FWindow. This essentallu enables input via the FWidget::OnHighPrecisionMouseInput
    // function to be called for the selected FWindow if the platform supports HighPrecisionMouse (On windows this is raw-input).
    bool EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window);

    // Returns true if the application support high-precision mouse input (On windows this corresponds to raw-input)
    bool SupportsHighPrecisionMouse() const;

    // Set the global cursor position
    void SetCursorPosition(const FIntVector2& Position);

    // Retrieve the global cursor position
    FIntVector2 GetCursorPosition() const;

    // Set the cursor type, basically the cursor-face
    void SetCursor(ECursor Cursor);

    // Shows or hides the cursor
    void ShowCursor(bool bIsVisible);

    // Returns true if the cursor is currently visible
    bool IsCursorVisibile() const;

    // Returns true if there is currently a gamepad connected. Returns false if there is no valid FInputDevice Interface
    bool IsGamePadConnected() const;

    // Returns true if we are currently tracking a mouse drag-operation. This happens when we press-down the left
    // mousebutton and this function will return true until the mousebutton is released.
    bool IsTrackingCursor() const { return bIsTrackingCursor; }
    
    // Overrides the current platform application with a new FGenericApplication instance
    void OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication);
    
    // Returns the current platform application, current FGenericApplication instance
    TSharedPtr<FGenericApplication> GetPlatformApplication() const { return PlatformApplication; }

    // Returns the current FInputDevice interface
    FInputDevice* GetInputDevice() const { return PlatformApplication->GetInputDevice(); }

    // Returns the cursor interface
    TSharedPtr<ICursor> GetCursor() const { return PlatformApplication->Cursor; }

    // Get current window that has focus
    TSharedPtr<FWindow> GetFocusWindow() const;

    // Sets the widget that should currently have focus. This function will first find a widget-path until it finds the 
    // lowest level window. Then it will set the focus to this found widget-path. In practise this means that it will 
    // set the focus to the specified widget, all it's parent widgets, and finally the window containing these widgets.
    void SetFocusWidget(const TSharedPtr<FWidget>& FocusWidget);

    // Sets the new widget-path which will now have focus. All these widgets  will after a call to this function recieve 
    // key events (including gamepad events).
    void SetFocusWidgets(const FWidgetPath& NewFocusPath);

    // Finds the lowest level window that contains this widget. This function will find a path to the lowest level window
    // and then returns that window.
    TSharedPtr<FWindow> FindWindowWidget(const TSharedPtr<FWidget>& InWidget);
    
    // Returns the window that corresponds the specified platform window
    TSharedPtr<FWindow> FindWindowFromGenericWindow(const TSharedRef<FGenericWindow>& PlatformWindow) const;
    
    // Returns the window that is currently under the cursor
    TSharedPtr<FWindow> FindWindowUnderCursor() const;

    // Returns a path of widgets that is currently under the cursor
    void FindWidgetsUnderCursor(FWidgetPath& OutCursorPath);
    
    // Returns a path of widgets that is currently under the cursor and contains the current mouse pointer. 
    void FindWidgetsUnderCursor(const FIntVector2& Point, FWidgetPath& OutCursorPath);

    // Retrieve cached display-info
    void GetDisplayInfo(TArray<FMonitorInfo>& OutMonitorInfo);

    // Retrieve the monitor config changed event
    FOnMonitorConfigChangedEvent& GetOnMonitorConfigChangedEvent() { return OnMonitorConfigChangedEvent; }

private:
    TSet<EKeyboardKeyName::Type>      PressedKeys;
    TSet<EMouseButtonName::Type>      PressedMouseButtons;
    TArray<FMonitorInfo>              MonitorInfos;
    bool                              bIsMonitorInfoValid;
    bool                              bIsTrackingCursor;
    TArray<TSharedPtr<FWindow>>       Windows;
    FWidgetPath                       FocusPath;
    FWidgetPath                       TrackedWidgets;
    TArray<TSharedPtr<FInputHandler>> InputHandlers;
    FOnMonitorConfigChangedEvent      OnMonitorConfigChangedEvent;

    static TSharedPtr<FApplicationInterface> ApplicationInstance;
    static TSharedPtr<FGenericApplication>   PlatformApplication;
};
