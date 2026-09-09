#pragma once
#include "Core/Containers/Set.h"
#include "Core/Delegates/Event.h"
#include "CoreApplication/PlatformInterface/IPlatformCursor.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/PlatformInterface/IPlatformApplicationMessageHandler.h"
#include "Application/IApplicationRenderer.h"
#include "Application/InputHandler.h"
#include "Application/ElementPath.h"
#include "Application/Elements/Window.h"

/** @brief Event triggered when the monitor configuration changes (e.g., adding or removing displays). */
DECLARE_EVENT(FOnMonitorConfigChangedEvent, FApplication);

/** @brief Delegate called to repaint a window while it is being resized, after the window carries the new size. */
DECLARE_DELEGATE(FOnWindowLiveResize, const TSharedPtr<FWindow>& /*Window*/);

/** @brief Event raised on both edges of an OS-driven window drag or resize. */
DECLARE_EVENT(FOnWindowInteractionEvent, FApplication, const TSharedPtr<FWindow>& /*Window*/, EWindowInteraction /*Interaction*/, bool /*bIsBeginning*/);

/** @brief Event raised once a window's tree is drawn, for anything painting over it without being in it. */
DECLARE_EVENT(FOnWindowPaintingEvent, FApplication, const TSharedPtr<FWindow>& /*Window*/);

class APPLICATION_API FApplication : public IPlatformApplicationMessageHandler , public TSharedFromThis<FApplication>
{
public:

    /**
     * @brief Creates the singleton instance of FApplication and the associated PlatformApplication.
     * 
     * @return True if both the FApplication and PlatformApplication instances were successfully created, otherwise false.
     */
    static bool Initialize();

    /**
     * @brief Creates the singleton over a platform application the caller already made, which is what
     * lets a headless harness drive the real input, focus and capture paths with no platform behind them.
     *
     * @param InPlatformApplication The platform application to route messages through.
     * @return True if the singleton was created, otherwise false.
     */
    static bool Initialize(const TSharedPtr<IPlatformApplication>& InPlatformApplication);

    /** @brief Releases the singleton FApplication and everything Initialize allocated behind it. */
    static void Release();

    /**
     * @brief Checks if the FApplication instance has been created.
     * 
     * @return True if the instance is valid (i.e., the application is initialized), otherwise false.
     */
    static bool FORCEINLINE IsInitialized()
    {
        return Application.IsValid();
    }

    /**
     * @brief Retrieves a reference to the FApplication singleton.
     *
     * @return A reference to the FApplication instance.
     */
    static FORCEINLINE FApplication& Get()
    {
        CHECK(Application.IsValid());
        return *Application;
    }

    /**
     * @brief Measures and arranges the element tree of a window, from the origin of its client area. The
     * layout is client-relative because that is the space the renderer projects from, so the screen
     * position of the window stays with the platform and the cursor is brought into this space before it
     * is tested against an element.
     *
     * @param InWindow The window to lay out.
     */
    static void LayoutWindow(const TSharedPtr<FWindow>& InWindow);

    /**
     * @brief The shape the leaf-most element on a path asks for, so a field inside a panel wins over the
     * panel. Static and pure, so the resolution can be checked without a platform application behind it.
     *
     * @param Path The path under the cursor, ordered from the window down to the leaf.
     * @return The shape to apply, which is the arrow when nothing on the path has an opinion.
     */
    NODISCARD static ECursor ResolveCursor(const FElementPath& Path);

public:
    FApplication(TSharedPtr<IPlatformApplication> InPlatformApplication);
    virtual ~FApplication();

    // IPlatformApplicationMessageHandler Interface Overrides
    virtual bool OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex) override final;
    virtual bool OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat) override final;
    virtual bool OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue) override final;

    virtual bool OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModifierKeyState) override final;
    virtual bool OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModifierKeyState) override final;
    virtual bool OnKeyChar(uint32 Character) override final;

    virtual bool OnMouseMove(int32 MouseX, int32 MouseY) override final;
    virtual bool OnMouseButtonDown(const TSharedRef<IPlatformWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState) override final;
    virtual bool OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState) override final;
    virtual bool OnMouseButtonDoubleClick(EMouseButtonName::Type Button, FModifierKeyState ModifierKeyState) override final;
    virtual bool OnMouseScrolled(float WheelDelta, EScrollAxis ScrollAxis) override final;
    virtual bool OnMouseEntered() override final;
    virtual bool OnMouseLeft() override final;
    virtual bool OnHighPrecisionMouseInput(int32 MouseX, int32 MouseY) override final;

    virtual bool OnWindowResized(const TSharedRef<IPlatformWindow>& Window, uint32 Width, uint32 Height) override final;
    virtual bool OnWindowResizing(const TSharedRef<IPlatformWindow>& Window, uint32 Width, uint32 Height) override final;
    virtual bool OnOSPaint(const TSharedRef<IPlatformWindow>& Window) override final;
    virtual bool BeginWindowInteraction(const TSharedRef<IPlatformWindow>& Window, EWindowInteraction Interaction) override final;
    virtual bool EndWindowInteraction(const TSharedRef<IPlatformWindow>& Window, EWindowInteraction Interaction) override final;
    virtual bool OnWindowMoved(const TSharedRef<IPlatformWindow>& Window, int32 MouseX, int32 MouseY) override final;
    virtual bool OnWindowFocusLost(const TSharedRef<IPlatformWindow>& Window) override final;
    virtual bool OnWindowFocusGained(const TSharedRef<IPlatformWindow>& Window) override final;
    virtual bool OnWindowClosed(const TSharedRef<IPlatformWindow>& Window) override final;

    virtual bool OnMonitorConfigurationChange() override final;
    virtual bool OnApplicationActivationChanged(bool bIsActive) override final;

    /**
     * @brief Adds a new window, creates and shows its platform window, and starts ticking it every frame.
     *
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
     * @brief Records every visible window into the renderer, using the layout produced by the last Tick.
     * Does nothing when no renderer is registered, which is how the headless tests and any run without an
     * RHI device behave.
     */
    void DrawWindows();

    /**
     * @brief Records a single window into the renderer, laying it out first when its layout is stale, for a
     * repaint of one window outside the frame the others are recorded in.
     *
     * @param InWindow The window to record, which is ignored when it is hidden or no renderer is registered.
     */
    void DrawWindow(const TSharedPtr<FWindow>& InWindow);

    /**
     * @brief Sets the renderer the windows record into.
     *
     * @param InRenderer The renderer, or null to stop drawing.
     */
    void SetRenderer(const TSharedPtr<IApplicationRenderer>& InRenderer);

    /**
     * @brief Sets the delegate called on the main thread to repaint a window the OS has asked for while it owns
     * the message pump, which is where a repaint scoped to a live resize is driven from. The window already
     * carries the new size when it runs. Only called from the platform paths that already run on the main
     * thread, since the delegate reads the element tree.
     *
     * @param InOnWindowLiveResize The delegate to set, or an unbound one to stop being called.
     */
    void SetOnWindowLiveResize(const FOnWindowLiveResize& InOnWindowLiveResize);

    /** @return The event raised on both edges of an OS-driven window drag or resize. */
    NODISCARD FORCEINLINE FOnWindowInteractionEvent& GetOnWindowInteractionEvent()
    {
        return OnWindowInteractionEvent;
    }

    /**
     * @return The event raised once a window's tree is drawn and before the deferred paints are drained,
     * which is where a subscriber calls FWindow::QueueDeferredPainting to paint over that window.
     */
    NODISCARD FORCEINLINE FOnWindowPaintingEvent& GetOnWindowPaintingEvent()
    {
        return OnWindowPaintingEvent;
    }

    /** @return The window the OS is running a modal drag or resize loop for, or null when there is none. */
    NODISCARD TSharedPtr<FWindow> GetInteractingWindow();

    /**
     * @brief Gets what the OS is currently doing to a window, which only means anything while
     * GetInteractingWindow reports one.
     *
     * @return The kind of the interaction in progress.
     */
    NODISCARD FORCEINLINE EWindowInteraction GetWindowInteraction() const
    {
        return WindowInteraction;
    }

    /** @return The renderer the windows record into, or null when none is registered. */
    NODISCARD FORCEINLINE TSharedPtr<IApplicationRenderer> GetRenderer() const
    {
        return Renderer;
    }

    /** @brief Drives the platform message pump, distributing events to the rest of the system. */
    void ProcessEvents();

    /** @brief Processes the events the platform batched or deferred to keep them out of a reentrant path. */
    void ProcessDeferredEvents();

    /** @brief Updates the input devices that do not report through the platform event queue. */
    void UpdateInputDevices();

    /**
     * @brief Checks if a gamepad is currently connected.
     *
     * @return True if a gamepad is connected, otherwise false.
     */
    bool IsGamePadConnected() const;

    /**
     * @brief Retrieves the primary input device interface (e.g., for gamepads).
     * 
     * @return A pointer to the current IPlatformInputDevice instance, or nullptr if none.
     */
    FORCEINLINE IPlatformInputDevice* GetInputDevice() const
    {
        return PlatformApplication->GetInputDevice();
    }

    /**
     * @brief Registers an input handler, which is offered every event before the elements are.
     *
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
     * @brief Checks if the application supports high-precision mouse input (raw input).
     * 
     * @return True if high-precision mouse input is supported, otherwise false.
     */
    bool SupportsHighPrecisionMouse() const;

    /**
     * @brief Enables or disables high-precision (relative) mouse input, if the platform supports it.
     * On Windows this leverages raw input events, on macOS it detaches the cursor from the pointer.
     *
     * @param Window The window that should receive raw input. Only used when enabling.
     * @param Mode Whether to enter or leave high-precision mode.
     * @return True if the mode was applied, otherwise false.
     */
    bool SetHighPrecisionMouseMode(const TSharedPtr<FWindow>& Window, EHighPrecisionMouseMode Mode);

    /**
     * @brief Keeps the cursor inside a region of the screen until the confinement is released.
     * High-precision mode reports movement but leaves the pointer free to wander off the window, so
     * confining it is a separate request.
     *
     * @param Window The window the region belongs to.
     * @param ScreenRect The region to confine the cursor to, in absolute screen coordinates.
     * @return True if the cursor was confined, otherwise false.
     */
    bool ConfineCursorToRect(const TSharedPtr<FWindow>& Window, const FRectangle& ScreenRect);

    /** @brief Lets the cursor leave the region set by ConfineCursorToRect. */
    void ReleaseCursorConfinement();

    /**
     * @brief Retrieves the current modifier key state (e.g., whether Ctrl, Alt, or Shift are pressed).
     * 
     * @return A struct that contains the current state of modifier keys.
     */
    FModifierKeyState GetModifierKeyState() const;

    /**
     * @brief Sets the global cursor position. Moves the system cursor to the specified position in screen coordinates.
     * 
     * @param Position The new absolute screen coordinates for the cursor.
     */
    void SetCursorPosition(const IntVector2& Position);

    /**
     * @brief Retrieves the global cursor position in screen coordinates.
     * 
     * @return The current (X, Y) position of the system cursor.
     */
    IntVector2 GetCursorPosition() const;

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
     * @brief Checks if the application is currently tracking a mouse drag, which lasts from the first
     * button press until the last release, or for as long as an element holds the capture.
     *
     * @return True if a mouse drag operation is in progress, otherwise false.
     */
    FORCEINLINE bool IsTrackingCursor() const
    {
        return bIsTrackingCursor;
    }

    /**
     * @brief Assigns persistent mouse capture to an element and binds platform capture to its window.
     *
     * @param Element The element that should receive mouse events while capture is held.
     * @return True if capture was assigned, otherwise false.
     */
    bool CaptureMouse(const TSharedPtr<FVisualElement>& Element);

    /**
     * @brief Releases persistent mouse capture previously taken by CaptureMouse.
     *
     * @param Element The element that currently owns capture. Pass nullptr to clear capture
     *               regardless of which element holds it. Non-null is ignored if it is not the captor.
     */
    void ReleaseMouseCapture(const TSharedPtr<FVisualElement>& Element = nullptr);

    /**
     * @brief Checks whether an element currently holds persistent mouse capture.
     *
     * @return True if MouseCaptor is valid.
     */
    FORCEINLINE bool HasMouseCapture() const
    {
        return MouseCaptor.IsValid();
    }

    /**
     * @brief Returns the element that currently holds persistent mouse capture.
     *
     * @return The capturing element, or nullptr if none.
     */
    FORCEINLINE TSharedPtr<FVisualElement> GetMouseCaptor() const
    {
        return MouseCaptor.IsValid() ? TSharedPtr<FVisualElement>(MouseCaptor) : nullptr;
    }

    /**
     * @brief Retrieves the cursor interface being used by the platform application.
     * 
     * @return A shared pointer to the IPlatformCursor interface, or nullptr if unsupported.
     */
    FORCEINLINE TSharedPtr<IPlatformCursor> GetCursor() const
    {
        return PlatformApplication->GetCursor();
    }

    /**
     * @brief Sets focus to the specified element and all of its parents up to the top-level window.
     * 
     * @param FocusElement The element that should receive focus.
     */
    void SetFocusElement(const TSharedPtr<FVisualElement>& FocusElement);

    /**
     * @brief Sets a new element path as the focus hierarchy, which runs from a window down to the
     * element that reads the keyboard.
     *
     * @param NewFocusPath The element path to set focus to.
     */
    void SetFocusElements(const FElementPath& NewFocusPath);

    /**
     * @brief Hands the keyboard to the deepest element on a cursor path that wants it, if any. Focus stays
     * where it is when the path resolves to an ancestor of whatever is focused, so clicking the chrome
     * around a focused field does not take the keyboard away from it.
     *
     * @param CursorPath The path under the cursor, ordered from the window down to the element hit.
     */
    void SetFocusFromCursorPath(const FElementPath& CursorPath);

    /**
     * @brief Gets the element that currently has focus (for receiving keyboard input, etc.).
     *
     * @return A shared pointer to the focused element, or nullptr if none.
     */
    TSharedPtr<FVisualElement> GetFocusElementLeaf() const;

    /**
     * @brief Gets the window that currently has focus (for receiving keyboard input, etc.).
     * 
     * @return A shared pointer to the focused window, or nullptr if none.
     */
    TSharedPtr<FWindow> GetFocusWindow() const;

    /**
     * @brief Finds the lowest-level window (top-level FWindow) that contains the specified element.
     * 
     * @param InElement The element to search for.
     * @return A shared pointer to the top-level FWindow that contains the element, or nullptr if not found.
     */
    TSharedPtr<FWindow> FindWindow(const TSharedPtr<FVisualElement>& InElement);

    /**
     * @brief Finds the FWindow that corresponds to a given platform window (IPlatformWindow).
     * 
     * @param PlatformWindow The IPlatformWindow to match against known windows.
     * @return A shared pointer to the corresponding FWindow, or nullptr if not found.
     */
    TSharedPtr<FWindow> FindWindowFromPlatformWindow(const TSharedRef<IPlatformWindow>& PlatformWindow) const;

    /**
     * @brief Returns the window currently under the mouse cursor.
     * 
     * @return A shared pointer to the FWindow under the cursor, or nullptr if none.
     */
    TSharedPtr<FWindow> FindWindowUnderCursor() const;

    /** @return Every window registered and not yet destroyed, menu popups and floating windows included. */
    NODISCARD FORCEINLINE const TArray<TSharedPtr<FWindow>>& GetWindows() const
    {
        return Windows;
    }

    /**
     * @brief Retrieves a path of elements currently under the mouse cursor.
     * 
     * @param OutCursorPath A element path object that will be populated with the elements under the cursor.
     */
    void FindElementsUnderCursor(FElementPath& OutCursorPath);

    /**
     * @brief Populates an element path with elements that lie under a screen coordinate.
     * 
     * @param ScreenPosition The coordinate in screen space, converted to client space per window.
     * @param OutCursorPath A element path object to populate.
     */
    void FindElementsUnderCursor(const IntVector2& ScreenPosition, FElementPath& OutCursorPath);

    /** @brief Re-reads the resolution, DPI and primary flag of every monitor after a setup change. */
    void UpdateMonitorInfo();

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
    FORCEINLINE FOnMonitorConfigChangedEvent& GetOnMonitorConfigChangedEvent()
    {
        return OnMonitorConfigChangedEvent;
    }

    /**
     * @brief Checks if the application is the active one on the desktop.
     *
     * @return True if the application is active, otherwise false.
     */
    FORCEINLINE bool IsApplicationActive() const
    {
        return bIsApplicationActive;
    }

    /**
     * @brief Overrides the existing platform application with a new IPlatformApplication instance.
     *
     * @param InPlatformApplication The new platform application to set.
     */
    void OverridePlatformApplication(const TSharedPtr<IPlatformApplication>& InPlatformApplication);

    /**
     * @brief Retrieves the current platform application interface.
     * 
     * @return A shared pointer to the current IPlatformApplication.
     */
    FORCEINLINE TSharedPtr<IPlatformApplication> GetPlatformApplication() const
    {
        return PlatformApplication;
    }

private:
    void ReleaseAllPressedInput();
    void ResolveMouseDispatchPath(FElementPath& OutPath);
    void UpdateCursor();
    IntVector2 GetClientOrigin();
    void RecordWindow(const TSharedPtr<FWindow>& InWindow);

    TSharedPtr<IPlatformApplication>   PlatformApplication;
    TSet<EKeyboardKeyName::Type>       PressedKeys;
    TSet<EMouseButtonName::Type>       PressedMouseButtons;
    TArray<FMonitorInfo>               MonitorInfos;
    FElementPath                       FocusPath;
    FElementPath                       TrackedElements;
    TArray<TSharedPtr<FWindow>>        Windows;
    TSharedPtr<IApplicationRenderer>   Renderer;
    TArray<TSharedPtr<FInputHandler>>  InputHandlers;
    FOnMonitorConfigChangedEvent       OnMonitorConfigChangedEvent;
    FOnWindowLiveResize                OnWindowLiveResizeDelegate;
    FOnWindowInteractionEvent          OnWindowInteractionEvent;
    FOnWindowPaintingEvent             OnWindowPaintingEvent;
    TWeakPtr<FWindow>                  InteractingWindow;
    EWindowInteraction                 WindowInteraction;
    TWeakPtr<FWindow>                  FocusWindow;
    TWeakPtr<FVisualElement>           MouseCaptor;
    IntVector2                         LastCursorPosition;
    bool                               bIsMonitorInfoValid : 1;
    bool                               bIsCursorPositionValid : 1;
    bool                               bIsTrackingCursor : 1;
    bool                               bIsApplicationActive : 1;

    static TSharedPtr<FApplication> Application;
};
