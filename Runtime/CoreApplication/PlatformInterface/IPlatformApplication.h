#pragma once
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/PlatformInterface/IPlatformWindow.h"

struct IPlatformCursor;
struct IPlatformApplicationMessageHandler;
struct IPlatformInputDevice;

enum class EHighPrecisionMouseMode : uint8
{
    Disabled = 0,
    Enabled  = 1,
};

enum class EModifierFlag : uint32
{
    None     = 0,
    Ctrl     = FLAG(1),
    Alt      = FLAG(2),
    Shift    = FLAG(3),
    CapsLock = FLAG(4),
    Super    = FLAG(5),
    NumLock  = FLAG(6),
};

ENUM_CLASS_OPERATORS(EModifierFlag);

class FModifierKeyState
{
public:
    FModifierKeyState()
        : Flags(EModifierFlag::None)
    {
    }

    FModifierKeyState(EModifierFlag InFlags)
        : Flags(InFlags)
    {
    }

    bool IsCtrlDown() const
    {
        return (Flags & EModifierFlag::Ctrl) != EModifierFlag::None;
    }

    bool IsAltDown() const
    {
        return (Flags & EModifierFlag::Alt) != EModifierFlag::None;
    }

    bool IsShiftDown() const
    {
        return (Flags & EModifierFlag::Shift) != EModifierFlag::None;
    }

    bool IsCapsLockDown() const
    {
        return (Flags & EModifierFlag::CapsLock) != EModifierFlag::None;
    }

    bool IsSuperDown() const
    {
        return (Flags & EModifierFlag::Super) != EModifierFlag::None;
    }

    bool IsNumPadDown() const
    {
        return (Flags & EModifierFlag::NumLock) != EModifierFlag::None;
    }
    
private:
    EModifierFlag Flags;
};

struct FMonitorInfo
{
    FMonitorInfo()
        : DisplayDPI(0)
        , DisplayScaling(1.0f)
        , bIsPrimary(false)
    {
    }

    bool operator==(const FMonitorInfo& Other) const
    {
        return DeviceName     == Other.DeviceName
            && MainPosition   == Other.MainPosition
            && MainSize       == Other.MainSize
            && WorkPosition   == Other.WorkPosition
            && WorkSize       == Other.WorkSize
            && DisplayDPI     == Other.DisplayDPI
            && DisplayScaling == Other.DisplayScaling
            && bIsPrimary     == Other.bIsPrimary;
    }

    bool operator!=(const FMonitorInfo& Other) const
    {
        return !(*this == Other);
    }

    /** @brief Name of the monitor as provided by the platform. */
    String DeviceName;
    
    /** @brief The workspace position of the monitor without considering UI elements like menu-bars or docks. */
    IntVector2 MainPosition;

    /** @brief The workspace size of the monitor without considering UI elements like menu-bars or docks. */
    IntVector2 MainSize;

    /** @brief The usable workspace position of the monitor, taking UI elements into account. */
    IntVector2 WorkPosition;

    /** @brief The usable workspace size of the monitor, taking UI elements into account. */
    IntVector2 WorkSize;

    /** @brief The DPI (dots per inch) of the monitor. */
    int32 DisplayDPI;
    
    /** @brief A scaling factor used to scale content according to the monitor's DPI. */
    float DisplayScaling;

    /** @brief Indicates if this monitor is marked as the primary display by the platform. */
    bool bIsPrimary;
};

struct IPlatformApplication
{
    static TSharedPtr<IPlatformApplication> Create()
    {
        return nullptr;
    }

    virtual ~IPlatformApplication() = default;

    /**
     * @brief Creates an interface for a platform window. The returned window is not fully initialized.
     * Call IPlatformWindow::Initialize() to complete setup.
     * 
     * @return A shared reference to the created IPlatformWindow, or nullptr if not supported.
     */
    virtual TSharedRef<IPlatformWindow> CreateWindow() = 0;

    /**
     * @brief Processes platform messages and any deferred actions for the application.
     * 
     * @param Delta The time elapsed since the last tick (in seconds).
     */
    virtual void Tick(float Delta) = 0;

    /** @brief Processes immediate platform events or messages that might have been received. */
    virtual void ProcessEvents() = 0;

    /** @brief Processes any deferred events that were queued for delayed handling. */
    virtual void ProcessDeferredEvents() = 0;

    /** @brief Updates the state of input devices (keyboard, mouse, gamepad, etc.). */
    virtual void UpdateInputDevices() = 0;

    /**
     * @brief Retrieves an input device interface (e.g., gamepad or specialized input device).
     * 
     * @return A pointer to the input device or nullptr if none exists.
     */
    virtual IPlatformInputDevice* GetInputDevice() = 0;

    /**
     * @brief Checks if high-precision (raw) mouse events are supported by this platform.
     *
     * @return True if high-precision mouse events are supported, otherwise false.
     */
    virtual bool SupportsHighPrecisionMouse() const = 0;

    /**
     * @brief Enables or disables high-precision (relative) mouse events.
     * 
     * @param Window The window that should receive high-precision mouse input. Only used when enabling.
     * @param Mode Whether to enter or leave high-precision mode.
     * @return True if the mode was applied, false if not supported.
     */
    virtual bool SetHighPrecisionMouseMode(const TSharedRef<IPlatformWindow>& Window, EHighPrecisionMouseMode Mode) = 0;

    /**
     * @brief Keeps the cursor inside a region of the screen until the confinement is released. 
     * High-precision mode reports movement but leaves the pointer free to wander off the window,
     * so a game that wants the pointer to stay put has to ask for it separately.
     * 
     * @param Window The window the region belongs to.
     * @param Position Top-left corner of the region, in absolute screen coordinates.
     * @param Size Width and height of the region, in pixels.
     * @return True if the cursor was confined, false if not supported.
     */
    virtual bool ConfineCursorToRect(const TSharedRef<IPlatformWindow>& Window, const IntVector2& Position, const IntVector2& Size) = 0;

    /** @brief Lets the cursor leave the region set by ConfineCursorToRect. */
    virtual void ReleaseCursorConfinement() = 0;

    /**
     * @brief Retrieves the current state of modifier keys (Shift, Ctrl, Alt, etc.).
     * 
     * @return The current modifier key state.
     */
    virtual FModifierKeyState GetModifierKeyState() const = 0;

    /**
     * @brief Sets a new active window.
     * 
     * @param Window The window to set as active.
     */
    virtual void SetActiveWindow(const TSharedRef<IPlatformWindow>& Window) = 0;

    /**
     * @brief Retrieves the current active (focused) window.
     * 
     * @return A shared reference to the active window or nullptr if none.
     */
    virtual TSharedRef<IPlatformWindow> GetActiveWindow() const = 0;

    /**
     * @brief Sets the window that should have mouse capture. This method is mainly relevant on Windows platforms.
     *
     * @param Window The window to capture the mouse.
     */
    virtual void SetCapture(const TSharedRef<IPlatformWindow>& Window) = 0;

    /**
     * @brief Retrieves the window that currently has mouse capture.
     * 
     * @return A shared reference to the captured window or nullptr if none.
     */
    virtual TSharedRef<IPlatformWindow> GetCapture() const = 0;

    /**
     * @brief Retrieves the window currently under the mouse cursor.
     * 
     * @return A shared reference to the window under the mouse cursor or nullptr if not supported.
     */
    virtual TSharedRef<IPlatformWindow> GetWindowUnderCursor() const = 0;

    /**
     * @brief Gathers information on monitors connected to the system.
     * 
     * @param OutMonitorInfo An array to receive the monitor information.
     */
    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const = 0;

    /**
     * @brief Sets the message handler for this application. The message handler is responsible for
     * processing platform messages (keyboard/mouse events).
     * 
     * @param InMessageHandler The message handler to use.
     */
    virtual void SetMessageHandler(const TSharedPtr<IPlatformApplicationMessageHandler>& InMessageHandler) = 0;

    /**
     * @brief Retrieves the current message handler.
     * 
     * @return A shared pointer to the message handler, or nullptr if none is set.
     */
    virtual TSharedPtr<IPlatformApplicationMessageHandler> GetMessageHandler() const = 0;

    /** 
     * @brief Retrieves the cursor interface for this application. 
     * 
     * @return The cursor interface for the application. May be null on some platforms that do not support 
     * cursors or rely on system defaults.
     */
    virtual TSharedPtr<IPlatformCursor> GetCursor() const = 0;
};
