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
    
    /** @brief The workspace position of the monitor without considering menu-bars, docks, etc., i.e., the "raw" position. */
    FIntVector2 MainPosition;

    /** @brief The workspace size of the monitor without considering menu-bars, docks, etc., i.e., the "raw" size. */
    FIntVector2 MainSize;

    /** @brief The usable workspace position of the monitor, considering menu-bars, docks, etc. */
    FIntVector2 WorkPosition;

    /** @brief The usable workspace size of the monitor, considering menu-bars, docks, etc. */
    FIntVector2 WorkSize;

    /** @brief The DPI (dots per inch) of the monitor. */
    int32 DisplayDPI;
    
    /** @brief A scaling factor used to scale content to the DPI of the monitor. */
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
     * @param InCursor The cursor to use with this application.
     */
    FGenericApplication(const TSharedPtr<ICursor>& InCursor);

    /**
     * @brief Virtual destructor to ensure proper cleanup in derived classes.
     */
    virtual ~FGenericApplication() = default;

    /**
     * @brief Creates an interface for a generic window. 
     * 
     * The window is not initialized, and FGenericWindow::Initialize needs to be called before the window is fully initialized.
     * @return A shared reference to the created FGenericWindow.
     */
    virtual TSharedRef<FGenericWindow> CreateWindow() { return nullptr; }

    /**
     * @brief Sets a new active window.
     * 
     * @param Window The window to set as active.
     */
    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) { }

    /**
     * @brief Sets the window that should have mouse capture.
     *
     * This is currently specific to Windows.
     * @param Window The window to capture the mouse.
     */
    virtual void SetCapture(const TSharedRef<FGenericWindow>& Window) { }

    /**
     * @brief Gets the window currently under the mouse cursor.
     * 
     * @return A shared reference to the window under the mouse cursor.
     */
    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const { return nullptr; }

    /**
     * @brief Gets the window that currently has mouse capture.
     * 
     * @return A shared reference to the window with mouse capture.
     */
    virtual TSharedRef<FGenericWindow> GetCapture() const { return nullptr; }

    /**
     * @brief Gets the current active window.
     * 
     * @return A shared reference to the active window.
     */
    virtual TSharedRef<FGenericWindow> GetActiveWindow() const { return nullptr; }

    /**
     * @brief Updates the application by processing platform messages and any deferred messages.
     * 
     * @param Delta The time elapsed since the last tick.
     */
    virtual void Tick(float Delta) { }

    /**
     * @brief Updates the input devices currently available to the application.
     */
    virtual void UpdateInputDevices() { }

    /**
     * @brief Retrieves the input device interface (e.g., gamepad).
     * 
     * @return A pointer to the input device.
     */
    virtual FInputDevice* GetInputDevice() { return nullptr; }

    /**
     * @brief Checks if high-precision mouse events are supported.
     *
     * This corresponds to raw input on Windows and is currently unsupported on macOS.
     * @return True if high-precision mouse events are supported.
     */
    virtual bool SupportsHighPrecisionMouse() const { return false; }

    /**
     * @brief Enables high-precision mouse events for a specific window, if supported by the platform.
     * 
     * @param Window The window for which to enable high-precision mouse events.
     * @return True if high-precision mouse events were successfully enabled.
     */
    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) { return true; }

    /**
     * @brief Queries the system for monitors currently connected and retrieves their information.
     * 
     * @param OutMonitorInfo An array to store the monitor information.
     */
    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const { }

    /**
     * @brief Sets the message handler for the application.
     * 
     * @param InMessageHandler The message handler to use.
     */
    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
    { 
        MessageHandler = InMessageHandler;
    }

    /**
     * @brief Gets the current message handler.
     * 
     * @return A shared pointer to the message handler.
     */
    TSharedPtr<FGenericApplicationMessageHandler> GetMessageHandler() const 
    { 
        return MessageHandler; 
    }

public:

    /** @brief The cursor associated with this application. */
    const TSharedPtr<ICursor> Cursor; 

protected:

    /** @brief The message handler used by the application */
    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
