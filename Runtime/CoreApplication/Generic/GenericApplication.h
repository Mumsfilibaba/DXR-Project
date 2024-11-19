#pragma once
#include "GenericWindow.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/SharedPtr.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct ICursor;
struct FGenericApplicationMessageHandler;
class FInputDevice;

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

    // This is just a name of the monitor decided by the platform
    FString DeviceName;

    // This is the workspace position of the monitor without taking menu-bars, dock, etc.
    // into account, so basically the "raw" position.
    FIntVector2 MainPosition;
    
    // This is the workspace size of the monitor without taking menu-bars, dock, etc.
    // into account, so basically the "raw" size.
    FIntVector2 MainSize;
    
    // This is the workspace position of the monitor that is usable. This is the postion
    // when taking menu-bars, dock, etc. into account.
    FIntVector2 WorkPosition;
    
    // This is the workspace size of the monitor that is usable. This is the size when
    // taking menu-bars, dock, etc. into account.
    FIntVector2 WorkSize;
    
    // The DPI of the montior
    int32 DisplayDPI;

    // This is a floating point value that can be used to scale content easily to the
    // DPI of the monitor.
    float DisplayScaling;

    // This monitor is marked as the primary display by the platform.
    bool bIsPrimary;
};

class COREAPPLICATION_API FGenericApplication
{
public:
    
    // Create a basic instance of a FGenericApplication, this is basically a null-application and can
    // be used when the FPlatformApplication is not desired.
    static TSharedPtr<FGenericApplication> Create();
    
public:
    FGenericApplication(const TSharedPtr<ICursor>& InCursor);
    virtual ~FGenericApplication() = default;

    // Creates a FGenericWindow interface, this window is not initialized though
    virtual TSharedRef<FGenericWindow> CreateWindow() { return nullptr; }
    
    // Update the FGenericApplication by pump messages from the platform and the process any deferred
    // messages that has been queued up to be processed.
    virtual void Tick(float Delta) { }
    
    // Updates any InputDevices that are currently available to the FGenericApplication
    virtual void UpdateInputDevices() { }
    
    // Retrieve the gamepad interface
    virtual FInputDevice* GetInputDevice() { return nullptr; }
    
    // Returns true if the FGenericApplication supports high-precision mouse-events. This corresponds
    // to raw-input on windows and is currently unsupported on macOS.
    virtual bool SupportsHighPrecisionMouse() const { return false; }
    
    // Enables high-precision mouse-events for a certain window if the platform supports this
    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) { return true; }
    
    // Set a new active window
    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) { }
    
    // Set the window that should have mouse-capture. This is currently specific to Windows.
    virtual void SetCapture(const TSharedRef<FGenericWindow>& Window) { }
    
    // Returns the current window that us under the mouse-cursor
    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const { return nullptr; }
    
    // Returns the current window that has the mouse-capture
    virtual TSharedRef<FGenericWindow> GetCapture() const { return nullptr; }
    
    // Returns the current window that is the current active window
    virtual TSharedRef<FGenericWindow> GetActiveWindow() const { return nullptr; }

    // This function queries directly from platform functions the different monitors that are currently
    // contected to the system and return this array of information for these monitors.
    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const { }

    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler)
    { 
        MessageHandler = InMessageHandler;
    }

    TSharedPtr<FGenericApplicationMessageHandler> GetMessageHandler() const 
    { 
        return MessageHandler; 
    }

public:
    const TSharedPtr<ICursor> Cursor;

protected:
    TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
