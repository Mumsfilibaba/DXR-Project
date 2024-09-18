#pragma once
#include "InputHandler.h"
#include "Widgets/Window.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/Array.h"
#include "CoreApplication/Generic/ICursor.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Generic/GenericApplicationMessageHandler.h"

class FPath
{
public:
    FPath()
        : Filter(EVisibility::Visible)
        , Widgets()
    {
    }

    FPath(EVisibility InFilter)
        : Filter(InFilter)
        , Widgets()
    {
    }

    void Add(EVisibility InVisibility, const TSharedPtr<FWidget>& InWidget)
    {
        CHECK(InWidget != nullptr);

        if (AcceptVisbility(InVisibility))
        {
            Widgets.Add(InWidget);
        }
    }

    void Insert(EVisibility InVisibility, const TSharedPtr<FWidget>& InWidget, int32 Position)
    {
        CHECK(InWidget != nullptr);

        if (AcceptVisbility(InVisibility))
        {
            Widgets.Insert(Position, InWidget);
        }
    }

    bool AcceptVisbility(EVisibility Visibility) const
    {
        return (Filter & Visibility) != EVisibility::None;
    }

    FORCEINLINE bool IsEmpty() const
    {
        return Widgets.IsEmpty();
    }

    FORCEINLINE bool Contains(const TSharedPtr<FWidget>& InWidget) const
    {
        return Widgets.Contains(InWidget);
    }

    FORCEINLINE void Remove(const TSharedPtr<FWidget>& InWidget)
    {
        Widgets.Remove(InWidget);
    }

    FORCEINLINE void RemoveAt(int32 Position)
    {
        Widgets.RemoveAt(Position);
    }

    FORCEINLINE int32 LastIndex() const
    {
        return Widgets.LastElementIndex();
    }

    FORCEINLINE int32 Size() const
    {
        return Widgets.Size();
    }

    EVisibility GetFilter() const
    {
        return Filter;
    }

    TArray<TSharedPtr<FWidget>>& GetWidgets()
    {
        return Widgets;
    }

    const TArray<TSharedPtr<FWidget>>& GetWidgets() const
    {
        return Widgets;
    }

    FORCEINLINE TSharedPtr<FWidget>& operator[](int32 Index)
    {
        return Widgets[Index];
    }

    FORCEINLINE const TSharedPtr<FWidget>& operator[](int32 Index) const
    {
        return Widgets[Index];
    }

private:
    EVisibility                 Filter;
    TArray<TSharedPtr<FWidget>> Widgets;
};

class APPLICATION_API FWindowedApplication : public FGenericApplicationMessageHandler, public TSharedFromThis<FWindowedApplication>
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

    static FWindowedApplication& Get()
    {
        CHECK(ApplicationInstance.IsValid());
        return *ApplicationInstance;
    }

    FWindowedApplication();
    virtual ~FWindowedApplication();

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

    // Get current window that has focus
    TSharedPtr<FWindow> GetFocusWindow() const;

    // Finds a window from a widget
    TSharedPtr<FWindow> FindWindowWidget(const TSharedPtr<FWidget>& InWidget);
    
    // Returns the window that has the specified platform window
    TSharedPtr<FWindow> FindWindowFromGenericWindow(const TSharedRef<FGenericWindow>& PlatformWindow) const;

    // Returns a path of widgets that is currently under the cursor
    void FindWidgetsUnderCursor(const FIntVector2& CursorPosition, FPath& OutCursorPath);

private:

    TSet<EKeyboardKeyName::Type>      PressedKeys;
    TSet<EMouseButtonName::Type>      PressedMouseButtons;
    FDisplayInfo                      DisplayInfo;
    bool                              bIsTrackingMouse;
    TArray<TSharedPtr<FWindow>>       Windows;
    FPath                             FocusPath;
    FPath                             TrackedWidgets;
    TArray<TSharedPtr<FInputHandler>> InputHandlers;

    static TSharedPtr<FWindowedApplication> ApplicationInstance;
    static TSharedPtr<FGenericApplication>  PlatformApplication;
};
