#pragma once
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/SharedPtr.h"
#include "CoreApplication/Generic/GenericWindow.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct ICursor;
struct FGenericApplicationMessageHandler;
class FInputDevice;

/**
 * @enum EModifierFlag
 * @brief Represents modifier keys (such as Ctrl, Alt, Shift) as flags.
 * 
 * Each enumerator corresponds to a particular modifier key state. These flags can be combined
 * to represent multiple modifier keys being pressed simultaneously (e.g., Ctrl + Alt).
 */
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

/**
 * @enum EHighPrecisionMouseMode
 * @brief Selects whether the platform reports absolute cursor positions or relative mouse deltas.
 */
enum class EHighPrecisionMouseMode : uint8
{
    Disabled = 0,
    Enabled  = 1,
};

/**
 * @class FModifierKeyState
 * @brief Encapsulates the state of modifier keys (Ctrl, Alt, Shift, etc.).
 *
 * This class provides simple, queryable methods to check whether a given modifier key is
 * currently pressed or active. The underlying flags (EModifierFlag) can represent multiple
 * pressed keys at once.
 */

class FModifierKeyState
{
public:

    /**
     * @brief Default constructor sets the modifier flags to None.
     */
    FModifierKeyState()
        : Flags(EModifierFlag::None)
    {
    }

    /**
     * @brief Constructs a modifier key state with the specified flags.
     *
     * @param InFlags The initial modifier flags.
     */
    FModifierKeyState(EModifierFlag InFlags)
        : Flags(InFlags)
    {
    }

    /**
     * @brief Checks if the Ctrl key is pressed.
     *
     * @return True if Ctrl is pressed, otherwise false.
     */
    bool IsCtrlDown() const
    {
        return (Flags & EModifierFlag::Ctrl) != EModifierFlag::None;
    }
    
    /**
     * @brief Checks if the Alt key is pressed.
     *
     * @return True if Alt is pressed, otherwise false.
     */
    bool IsAltDown() const
    {
        return (Flags & EModifierFlag::Alt) != EModifierFlag::None;
    }
    
    /**
     * @brief Checks if the Shift key is pressed.
     *
     * @return True if Shift is pressed, otherwise false.
     */
    bool IsShiftDown() const
    {
        return (Flags & EModifierFlag::Shift) != EModifierFlag::None;
    }
    
    /**
     * @brief Checks if CapsLock is active.
     *
     * @return True if CapsLock is active, otherwise false.
     */
    bool IsCapsLockDown() const
    {
        return (Flags & EModifierFlag::CapsLock) != EModifierFlag::None;
    }
    
    /**
     * @brief Checks if the Super key (e.g., Windows key on Windows, Command key on macOS) is pressed.
     *
     * @return True if Super is pressed, otherwise false.
     */
    bool IsSuperDown() const
    {
        return (Flags & EModifierFlag::Super) != EModifierFlag::None;
    }
    
    /**
     * @brief Checks if NumLock is active.
     *
     * @return True if NumLock is active, otherwise false.
     */
    bool IsNumPadDown() const
    {
        return (Flags & EModifierFlag::NumLock) != EModifierFlag::None;
    }
    
private:
    /** @brief The combined modifier flags representing the current state of the modifier keys. */
    EModifierFlag Flags;
};

/**
 * @struct FMonitorInfo
 * @brief Contains information about a monitor connected to the system.
 *
 * FMonitorInfo encapsulates various properties of a monitor, including its name, position, size,
 * DPI, scaling factor, and whether it is the primary display. This information is useful for
 * managing window layouts, rendering, and user interface scaling across multiple monitors.
 */

struct FMonitorInfo
{
    /**
     * @brief Default constructor initializes monitor information with default values.
     */
    FMonitorInfo()
        : DisplayDPI(0)
        , DisplayScaling(1.0f)
        , bIsPrimary(false)
    {
    }

    /**
     * @brief Checks if this monitor info is equal to another.
     * 
     * @param Other The other monitor info to compare with.
     * @return True if both monitor info objects are equal.
     */
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

    /**
     * @brief Checks if this monitor info is not equal to another.
     * 
     * @param Other The other monitor info to compare with.
     * @return True if both monitor info objects are not equal.
     */
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

/**
 * @class FGenericApplication
 * @brief Manages the generic application lifecycle, input, and window handling.
 *
 * FGenericApplication serves as an abstract base class for managing application-level functionalities
 * such as window creation, input handling, run loop management, and monitor information retrieval.
 * It provides a standardized interface that can be implemented for various platforms to ensure consistent
 * behavior across different operating systems.
 */

class COREAPPLICATION_API FGenericApplication
{
public:

    /**
     * @brief Creates a basic instance of FGenericApplication.
     * 
     * @return A shared pointer to the created FGenericApplication.
     */
    static TSharedPtr<FGenericApplication> Create();
    
public:

    /**
     * @brief Constructs an FGenericApplication with the specified cursor.
     * 
     * @param InCursor The cursor to use with this application. Can be null if cursors are unsupported.
     */
    FGenericApplication(const TSharedPtr<ICursor>& InCursor);

    /**
     * @brief Virtual destructor to ensure proper cleanup in derived classes.
     */
    virtual ~FGenericApplication() = default;

    /**
     * @brief Creates an interface for a generic window. The returned window is not fully initialized.
     * Call FGenericWindow::Initialize() to complete setup.
     * 
     * @return A shared reference to the created FGenericWindow, or nullptr if not supported.
     */
    virtual TSharedRef<FGenericWindow> CreateWindow() { return nullptr; }

    /**
     * @brief Processes platform messages and any deferred actions for the application.
     * 
     * @param Delta The time elapsed since the last tick (in seconds).
     */
    virtual void Tick(float Delta) { }

    /**
     * @brief Processes immediate platform events or messages that might have been received.
     */
    virtual void ProcessEvents() { };

    /**
     * @brief Processes any deferred events that were queued for delayed handling.
     */
    virtual void ProcessDeferredEvents() { }

    /**
     * @brief Updates the state of input devices (keyboard, mouse, gamepad, etc.).
     */
    virtual void UpdateInputDevices() { }

    /**
     * @brief Retrieves an input device interface (e.g., gamepad or specialized input device).
     * 
     * @return A pointer to the input device or nullptr if none exists.
     */
    virtual FInputDevice* GetInputDevice() { return nullptr; }

    /**
     * @brief Checks if high-precision (raw) mouse events are supported by this platform.
     *
     * @return True if high-precision mouse events are supported, otherwise false.
     */
    virtual bool SupportsHighPrecisionMouse() const { return false; }

    /**
     * @brief Enables or disables high-precision (relative) mouse events.
     * 
     * @param Window The window that should receive high-precision mouse input. Only used when enabling.
     * @param Mode Whether to enter or leave high-precision mode.
     * @return True if the mode was applied, false if not supported.
     */
    virtual bool SetHighPrecisionMouseMode(const TSharedRef<FGenericWindow>& Window, EHighPrecisionMouseMode Mode) { return true; }

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
    virtual bool ConfineCursorToRect(const TSharedRef<FGenericWindow>& Window, const IntVector2& Position, const IntVector2& Size) { return false; }

    /**
     * @brief Lets the cursor leave the region set by ConfineCursorToRect.
     */
    virtual void ReleaseCursorConfinement() { }

    /**
     * @brief Retrieves the current state of modifier keys (Shift, Ctrl, Alt, etc.).
     * 
     * @return The current modifier key state.
     */
    virtual FModifierKeyState GetModifierKeyState() const { return FModifierKeyState(); }

    /**
     * @brief Sets a new active window.
     * 
     * @param Window The window to set as active.
     */
    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) { }

    /**
     * @brief Sets the window that should have mouse capture. This method is mainly relevant on Windows platforms.
     *
     * @param Window The window to capture the mouse.
     */
    virtual void SetCapture(const TSharedRef<FGenericWindow>& Window) { }

    /**
     * @brief Retrieves the window currently under the mouse cursor.
     * 
     * @return A shared reference to the window under the mouse cursor or nullptr if not supported.
     */
    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const { return nullptr; }

    /**
     * @brief Retrieves the current active (focused) window.
     * 
     * @return A shared reference to the active window or nullptr if none.
     */
    virtual TSharedRef<FGenericWindow> GetActiveWindow() const { return nullptr; }

    /**
     * @brief Retrieves the window that currently has mouse capture.
     * 
     * @return A shared reference to the captured window or nullptr if none.
     */
    virtual TSharedRef<FGenericWindow> GetCapture() const { return nullptr; }

    /**
     * @brief Gathers information on monitors connected to the system.
     * 
     * @param OutMonitorInfo An array to receive the monitor information.
     */
    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const { }

    /**
     * @brief Sets the message handler for this application. The message handler is responsible for
     * processing platform messages (keyboard/mouse events).
     * 
     * @param InMessageHandler The message handler to use.
     */
    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
    { 
        MessageHandler = InMessageHandler;
    }

    /**
     * @brief Retrieves the current message handler.
     * 
     * @return A shared pointer to the message handler, or nullptr if none is set.
     */
    TSharedPtr<FGenericApplicationMessageHandler> GetMessageHandler() const 
    { 
        return MessageHandler; 
    }

    /** 
     * @brief Retrieves the cursor interface for this application. 
     * 
     * @return The cursor interface for the application. May be null on some platforms that do not support 
     * cursors or rely on system defaults.
     */
    TSharedPtr<ICursor> GetCursor() const { return Cursor; }

protected:
    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;

private:
    const TSharedPtr<ICursor> Cursor; 
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
