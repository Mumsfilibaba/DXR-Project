#pragma once
#include "InputHandler.h"
#include "Widgets/Window.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/Array.h"
#include "CoreApplication/Generic/ICursor.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"

class APPLICATION_API FApplication : public FGenericApplicationMessageHandler, public TSharedFromThis<FApplication>
{
public:

    // Creates the application instance
    static bool Create();

    // Destroys the application instance
    static void Destroy();

    static bool IsInitialized() 
    {
        return ApplicationInstance.IsValid();
    }

    static FApplication& Get()
    {
        CHECK(ApplicationInstance.IsValid());
        return *ApplicationInstance;
    }

public:
    FApplication();
    virtual ~FApplication();

public:

    // FGenericApplicationMessageHandler Interface
    virtual bool OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue) override final;
    virtual bool OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex) override final;
    virtual bool OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat) override final;
    virtual bool OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModierKeyState) override final;
    virtual bool OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState) override final;
    virtual bool OnKeyChar(uint32 Character) override final;
    virtual bool OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModierKeyState, int32 MouseX, int32 MouseY) override final;
    virtual bool OnMouseButtonDown(const TSharedRef<FGenericWindow>& Window, EMouseButtonName::Type Button, FModifierKeyState ModierKeyState, int32 MouseX, int32 MouseY) override final;
    virtual bool OnMouseMove(int32 MouseX, int32 MouseY) override final;
    virtual bool OnMouseScrolled(float WheelDelta, bool bVertical, int32 MouseX, int32 MouseY) override final;
    virtual bool OnWindowResized(const TSharedRef<FGenericWindow>& Window, uint32 Width, uint32 Height) override final;
    virtual bool OnWindowMoved(const TSharedRef<FGenericWindow>& Window, int32 MouseX, int32 MouseY) override final;
    virtual bool OnWindowFocusLost(const TSharedRef<FGenericWindow>& Window) override final;
    virtual bool OnWindowFocusGained(const TSharedRef<FGenericWindow>& Window) override final;
    virtual bool OnWindowClosed(const TSharedRef<FGenericWindow>& Window) override final;
    virtual bool OnMonitorChange() override final;

public:

    // Creates a platform window and adds the FWindow to the application's windows for event processing
    void InitializeWindow(const TSharedPtr<FWindow>& InWindow);

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
    void SetCursorScreenPosition(const FIntVector2& Position);

    // Retrieves the global cursor position
    FIntVector2 GetCursorScreenPosition() const;

    // Set the cursor type
    void SetCursor(ECursor Cursor);

    // Shows or hides the cursor
    void ShowCursor(bool bIsVisible);

    // Checks if the cursor is currently visible
    bool IsCursorVisibile() const;

    // Checks if there is currently a game-pad connected
    bool IsGamePadConnected() const;

    // Overrides the current platform application
    void OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication);
    
    // Returns the current platform application
    TSharedPtr<FGenericApplication> GetPlatformApplication() const { return PlatformApplication; }

    // Returns the current input device interface
    FInputDevice* GetInputDeviceInterface() const { return PlatformApplication->GetInputDeviceInterface(); }

    // Returns the cursor interface
    TSharedPtr<ICursor> GetCursor() const { return PlatformApplication->Cursor; }

    // Finds a window from a widget
    TSharedPtr<FWindow> FindWindowWidget(const TSharedPtr<FWidget>& InWidget);

    // Get current window that has focus
    TSharedPtr<FWindow> GetFocusWindow() const;

    // Returns a path of widgets that is currently under the cursor
    void FindWidgetsUnderCursor(const FIntVector2& CursorPosition, TArray<TSharedPtr<FWidget>>& OutWidgets);

private:
    TSharedPtr<FWindow> GetWindowFromPlatformWindow(const TSharedRef<FGenericWindow>& PlatformWindow) const;
    void GetActiveWindowWidgets(TArray<TSharedPtr<FWidget>>& OutWidgets);

    TSet<EKeyboardKeyName::Type>      PressedKeys;
    TSet<EMouseButtonName::Type>      PressedMouseButtons;
    FDisplayInfo                      DisplayInfo;
    bool                              bIsTrackingMouse;
    TArray<TSharedPtr<FWindow>>       Windows;
    TSharedPtr<FWindow>               FocusWindow;
    TArray<TSharedPtr<FInputHandler>> InputPreProcessors;
    TArray<TSharedPtr<FWidget>>       TrackedWidgets;

    static TSharedPtr<FApplication>        ApplicationInstance;
    static TSharedPtr<FGenericApplication> PlatformApplication;
};
