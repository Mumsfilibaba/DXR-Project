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
    FString DeviceName;
    
    /** @brief The workspace position of the monitor without considering UI elements like menu-bars or docks. */
    FIntVector2 MainPosition;

    /** @brief The workspace size of the monitor without considering UI elements like menu-bars or docks. */
    FIntVector2 MainSize;

    /** @brief The usable workspace position of the monitor, taking UI elements into account. */
    FIntVector2 WorkPosition;

    /** @brief The usable workspace size of the monitor, taking UI elements into account. */
    FIntVector2 WorkSize;

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
     * This acts as a null-application and can be used when a full-fledged platform application is not required.
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
     * @brief Creates an interface for a generic window. 
     * 
     * The returned window is not fully initialized; call FGenericWindow::Initialize() to complete setup.
     * By default, returns nullptr. Platform-specific implementations should override this to create a real window.
     * 
     * @return A shared reference to the created FGenericWindow, or nullptr if not supported.
     */
    virtual TSharedRef<FGenericWindow> CreateWindow() { return nullptr; }

    /**
     * @brief Sets a new active window.
     * 
     * By default, this method does nothing; platform-specific applications should override it to implement actual behavior.
     * @param Window The window to set as active.
     */
    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) { }

    /**
     * @brief Sets the window that should have mouse capture.
     *
     * This method is mainly relevant on Windows platforms. By default, does nothing; override for actual capture logic.
     * @param Window The window to capture the mouse.
     */
    virtual void SetCapture(const TSharedRef<FGenericWindow>& Window) { }

    /**
     * @brief Retrieves the window currently under the mouse cursor.
     * 
     * By default, returns nullptr. Override to return an appropriate window reference on supported platforms.
     * @return A shared reference to the window under the mouse cursor or nullptr if not supported.
     */
    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const { return nullptr; }

    /**
     * @brief Retrieves the window that currently has mouse capture.
     * 
     * By default, returns nullptr. Override to return the actual captured window on supported platforms.
     * @return A shared reference to the captured window or nullptr if none.
     */
    virtual TSharedRef<FGenericWindow> GetCapture() const { return nullptr; }

    /**
     * @brief Retrieves the current active (focused) window.
     * 
     * By default, returns nullptr. Platform implementations should override to return the window with focus.
     * @return A shared reference to the active window or nullptr if none.
     */
    virtual TSharedRef<FGenericWindow> GetActiveWindow() const { return nullptr; }

    /**
     * @brief Processes platform messages and any deferred actions for the application.
     * 
     * @param Delta The time elapsed since the last tick (in seconds).
     * By default, does nothing; platform-specific code should override for per-frame updates.
     */
    virtual void Tick(float Delta) { }

    /**
     * @brief Processes immediate platform events or messages that might have been received.
     *
     * By default, does nothing; override in derived classes to handle OS-specific event loops or message pumps.
     */
    virtual void ProcessEvents() { };

    /**
     * @brief Processes any deferred events that were queued for delayed handling.
     *
     * By default, does nothing; platform-specific code should override if relevant to your message/event model.
     */
    virtual void ProcessDeferredEvents() { }

    /**
     * @brief Updates the state of input devices (keyboard, mouse, gamepad, etc.).
     * 
     * By default, does nothing. Platform-specific implementations should override for actual device updates.
     */
    virtual void UpdateInputDevices() { }

    /**
     * @brief Retrieves an input device interface (e.g., gamepad or specialized input device).
     * 
     * By default, returns nullptr. Override in platform-specific classes that manage input devices.
     * @return A pointer to the input device or nullptr if none exists.
     */
    virtual FInputDevice* GetInputDevice() { return nullptr; }

    /**
     * @brief Checks if high-precision (raw) mouse events are supported by this platform.
     *
     * By default, returns false. For Windows, this corresponds to raw input. macOS does not currently support it.
     * @return True if high-precision mouse events are supported, otherwise false.
     */
    virtual bool SupportsHighPrecisionMouse() const { return false; }

    /**
     * @brief Enables high-precision mouse events for a specific window, if the platform supports it.
     * 
     * By default, returns true but does nothing. Override in platform-specific classes to enable raw input or equivalent.
     * @param Window The window to enable high-precision mouse input for.
     * @return True if successfully enabled, false if not supported.
     */
    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) { return true; }

    /**
     * @brief Retrieves the current state of modifier keys (Shift, Ctrl, Alt, etc.).
     * 
     * By default, returns a default-constructed FModifierKeyState (no keys pressed).
     * Override in platform-specific classes to provide accurate key state information.
     * @return The current modifier key state.
     */
    virtual FModifierKeyState GetModifierKeyState() const { return FModifierKeyState(); }

    /**
     * @brief Gathers information on monitors connected to the system.
     * 
     * By default, does nothing. Platform-specific implementations should populate OutMonitorInfo 
     * with details of each connected monitor (resolution, DPI, primary status, etc.).
     * @param OutMonitorInfo An array to receive the monitor information.
     */
    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const { }

    /**
     * @brief Sets the message handler for this application.
     * 
     * The message handler is responsible for processing platform messages (e.g., keyboard/mouse events).
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

public:

    /** 
     * @brief The cursor associated with this application. 
     * 
     * May be null on some platforms that do not support cursors or rely on system defaults.
     */
    const TSharedPtr<ICursor> Cursor; 

protected:
    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
