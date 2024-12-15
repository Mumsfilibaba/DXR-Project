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
 * to represent multiple modifier keys being pressed simultaneously.
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
 * This class provides simple, queryable methods to check whether a given modifier key is currently pressed or active.
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

    /** @brief Name of the monitor as decided by the platform. */
    FString DeviceName;
    
    /** @brief The workspace position of the monitor without considering UI elements like menu-bars or docks. */
    FIntVector2 MainPosition;

    /** @brief The workspace size of the monitor without considering UI elements like menu-bars or docks. */
    FIntVector2 MainSize;

    /** @brief The usable workspace position of the monitor, considering UI elements like menu-bars or docks. */
    FIntVector2 WorkPosition;

    /** @brief The usable workspace size of the monitor, considering UI elements like menu-bars or docks. */
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
     * This is essentially a null-application and can be used when FPlatformApplication is not desired.
     * @return A shared pointer to the created FGenericApplication.
     */
    static TSharedPtr<FGenericApplication> Create();
    
public:
    /**
     * @brief Constructs an FGenericApplication with the specified cursor.
     * 
     * @param InCursor The cursor to use with this application. Can be null on some platforms.
     */
    FGenericApplication(const TSharedPtr<ICursor>& InCursor);

    /**
     * @brief Virtual destructor to ensure proper cleanup in derived classes.
     */
    virtual ~FGenericApplication() = default;

    /**
     * @brief Creates an interface for a generic window. 
     * 
     * The returned window is not fully initialized and requires a call to FGenericWindow::Initialize.
     * By default, this method returns nullptr and should be overridden by platform-specific implementations.
     * @return A shared reference to the created FGenericWindow, or nullptr if not supported.
     */
    virtual TSharedRef<FGenericWindow> CreateWindow() { return nullptr; }

    /**
     * @brief Sets a new active window.
     * 
     * By default, this method does nothing and should be overridden by platform-specific implementations.
     * @param Window The window to set as active.
     */
    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) { }

    /**
     * @brief Sets the window that should have mouse capture.
     *
     * This is currently specific to Windows platforms. By default, this method does nothing and
     * should be overridden by platform-specific implementations that support mouse capture.
     * @param Window The window to capture the mouse.
     */
    virtual void SetCapture(const TSharedRef<FGenericWindow>& Window) { }

    /**
     * @brief Gets the window currently under the mouse cursor.
     * 
     * By default, this returns nullptr. Platform-specific implementations should override this
     * to return the appropriate window.
     * @return A shared reference to the window under the mouse cursor or nullptr if not supported.
     */
    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const { return nullptr; }

    /**
     * @brief Gets the window that currently has mouse capture.
     * 
     * By default, this returns nullptr. Platform-specific implementations should override this
     * to return the window that currently has capture.
     * @return A shared reference to the captured window or nullptr if none.
     */
    virtual TSharedRef<FGenericWindow> GetCapture() const { return nullptr; }

    /**
     * @brief Gets the current active window.
     * 
     * By default, this returns nullptr. Platform-specific implementations should override this
     * to return the active window.
     * @return A shared reference to the active window or nullptr if none.
     */
    virtual TSharedRef<FGenericWindow> GetActiveWindow() const { return nullptr; }

    /**
     * @brief Updates the application by processing platform messages and any deferred actions.
     * 
     * By default, this method does nothing. Platform-specific implementations should override this
     * to handle per-frame updates.
     * @param Delta The time elapsed since the last tick (in seconds).
     */
    virtual void Tick(float Delta) { }

    virtual void ProcessEvents() { };

    virtual void ProcessDeferredEvents() { }

    /**
     * @brief Updates the input devices currently available to the application.
     * 
     * By default, this method does nothing. Platform-specific implementations should override this
     * to reflect changes in input devices.
     */
    virtual void UpdateInputDevices() { }

    /**
     * @brief Retrieves the input device interface (e.g., for a gamepad or a specialized input device).
     * 
     * By default, returns nullptr. Platform-specific implementations should override this to return
     * a platform-specific input device pointer.
     * @return A pointer to the input device or nullptr if none.
     */
    virtual FInputDevice* GetInputDevice() { return nullptr; }

    /**
     * @brief Checks if high-precision mouse events are supported by this platform.
     *
     * By default, this returns false. On Windows, this corresponds to raw input, and on macOS it is currently unsupported.
     * @return True if high-precision mouse events are supported, otherwise false.
     */
    virtual bool SupportsHighPrecisionMouse() const { return false; }

    /**
     * @brief Enables high-precision mouse events for a specific window, if supported by the platform.
     * 
     * By default, this returns true but does nothing. Platform-specific implementations should
     * override this to enable platform-specific raw input modes.
     * @param Window The window for which to enable high-precision mouse events.
     * @return True if successfully enabled, otherwise false.
     */
    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) { return true; }

    /**
     * @brief Retrieves the current state of the modifier keys (Shift, Ctrl, Alt, etc.).
     * 
     * By default, returns an empty FModifierKeyState. Platform-specific code should override
     * this to accurately reflect the current modifier key state.
     * @return The current modifier key state.
     */
    virtual FModifierKeyState GetModifierKeyState() const { return FModifierKeyState(); }

    /**
     * @brief Queries the system for monitors currently connected and retrieves their information.
     * 
     * By default, this method does nothing. Platform-specific implementations should populate
     * OutMonitorInfo with the details of all connected monitors.
     * @param OutMonitorInfo An array to store the monitor information.
     */
    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const { }

    /**
     * @brief Sets the message handler for the application.
     * 
     * The message handler is responsible for handling platform messages (e.g., key presses, mouse events).
     * @param InMessageHandler The message handler to use.
     */
    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
    { 
        MessageHandler = InMessageHandler;
    }

    /**
     * @brief Gets the current message handler.
     * 
     * @return A shared pointer to the message handler, or nullptr if none is set.
     */
    TSharedPtr<FGenericApplicationMessageHandler> GetMessageHandler() const 
    { 
        return MessageHandler; 
    }

public:

    /** @brief The cursor associated with this application. Can be null if unsupported on a platform. */
    const TSharedPtr<ICursor> Cursor; 

protected:
    /** @brief The message handler used by the application. Handles windowing and input events. */
    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
