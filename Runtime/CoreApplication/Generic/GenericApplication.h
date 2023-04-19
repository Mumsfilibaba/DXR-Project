#pragma once
#include "InputDevice.h"
#include "ICursor.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FMonitorDesc
{
    float DisplayScaling = 1.0f;
};

class COREAPPLICATION_API FGenericApplication
{
public:
    FGenericApplication(const TSharedPtr<ICursor>& InCursor)
        : Cursor(InCursor)
        , MessageHandler(nullptr)
    { }

    virtual ~FGenericApplication() = default;

    virtual TSharedRef<FGenericWindow> CreateWindow() { return nullptr; }

    virtual void Tick(float Delta) { }

    virtual void PollInputDevices() { }

    virtual FInputDevice* GetInputDeviceInterface() { return nullptr; }

    virtual bool SupportsHighPrecisionMouse() const { return false; }

    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) { return true; }

    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) { }
    
    virtual void SetCapture(const TSharedRef<FGenericWindow>& Window) { }

    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const { return nullptr; }
    
    virtual TSharedRef<FGenericWindow> GetCapture() const { return nullptr; }
    
    virtual TSharedRef<FGenericWindow> GetActiveWindow() const { return nullptr; }

    virtual FMonitorDesc GetMonitorDescFromWindow(const TSharedRef<FGenericWindow>& Window) const { return FMonitorDesc{}; }

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
