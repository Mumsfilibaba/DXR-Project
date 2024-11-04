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
    
    // Creates a platform window and adds the FWindow to the application's windows for event processing
    void CreateWindow(const TSharedPtr<FWindow>& InWindow);

    // Destroys a window, invokes any callbacks belonging to the window and removes it from the application's list of windows
    void DestroyWindow(const TSharedPtr<FWindow>& InWindow);

    // Pumps the event queue and updates input etc.
    void Tick(float Delta);

    // Update any extra input device (Such as a controller that might be connected)
    void UpdateInputDevices();

    // Updates the stored monitor information
    void UpdateMonitorInfo();

    // Registers a new input-handler
    void RegisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    // Unregisters a input-handler
    void UnregisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    // Enables high-precision mouse input for the window
    bool EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window);

    // Checks if the application can support high-precision mouse input
    bool SupportsHighPrecisionMouse() const;

    // Sets the global cursor position on the screen
    void SetCursorPosition(const FIntVector2& Position);

    // Retrieves the global cursor position
    FIntVector2 GetCursorPosition() const;

    // Set the cursor type
    void SetCursor(ECursor Cursor);

    // Shows or hides the cursor
    void ShowCursor(bool bIsVisible);

    // Checks if the cursor is currently visible
    bool IsCursorVisibile() const;

    // Checks if there is currently a game-pad connected
    bool IsGamePadConnected() const;

    // Returns true if we are currently tracking the mouse in a drag-operation
    bool IsTrackingCursor() const { return bIsTrackingCursor; }
    
    // Overrides the current platform application
    void OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication);
    
    // Returns the current platform application
    TSharedPtr<FGenericApplication> GetPlatformApplication() const { return PlatformApplication; }

    // Returns the current input device interface
    FInputDevice* GetInputDeviceInterface() const { return PlatformApplication->GetInputDeviceInterface(); }

    // Returns the cursor interface
    TSharedPtr<ICursor> GetCursor() const { return PlatformApplication->Cursor; }

    // Get current window that has focus
    TSharedPtr<FWindow> GetFocusWindow() const;

    // Sets the widget that should currently have focus
    void SetFocusWidget(const TSharedPtr<FWidget>& FocusWidget);

    // Sets the new widget-path which will now have focus
    void SetFocusWidgets(const FWidgetPath& NewFocusPath);

    // Finds a window from a widget
    TSharedPtr<FWindow> FindWindowWidget(const TSharedPtr<FWidget>& InWidget);
    
    // Returns the window that has the specified platform window
    TSharedPtr<FWindow> FindWindowFromGenericWindow(const TSharedRef<FGenericWindow>& PlatformWindow) const;
    
    // Returns the window that is currently under the cursor
    TSharedPtr<FWindow> FindWindowUnderCursor() const;

    // Returns a path of widgets that is currently under the cursor
    void FindWidgetsUnderCursor(FWidgetPath& OutCursorPath);
    
    // Returns a path of widgets that is currently under the cursor
    void FindWidgetsUnderCursor(const FIntVector2& CursorPosition, FWidgetPath& OutCursorPath);

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
