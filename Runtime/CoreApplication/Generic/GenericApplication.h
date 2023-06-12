#pragma once
#include "InputDevice.h"
#include "ICursor.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

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

    FString     DeviceName;

    FIntVector2 MainPosition;
    FIntVector2 MainSize;
    
    FIntVector2 WorkPosition;
    FIntVector2 WorkSize;
    
    int32 DisplayDPI;
    int32 DisplayScaleFactor;

    float DisplayScaling;

    bool  bIsPrimary;
};

struct FDisplayInfo
{
    FDisplayInfo()
        : PrimaryDisplayWidth(0)
        , PrimaryDisplayHeight(0)
        , MonitorInfos()
    {
    }

    bool operator==(const FDisplayInfo& Other) const
    {
        return PrimaryDisplayWidth  == Other.PrimaryDisplayWidth
            && PrimaryDisplayHeight == Other.PrimaryDisplayHeight
            && MonitorInfos         == Other.MonitorInfos;
    }

    bool operator!=(const FDisplayInfo& Other) const
    {
        return !(*this == Other);
    }

    int32 PrimaryDisplayWidth;
    int32 PrimaryDisplayHeight;

    TArray<FMonitorInfo> MonitorInfos;
};


class COREAPPLICATION_API FGenericApplication
{
public:
    FGenericApplication(const TSharedPtr<ICursor>& InCursor)
        : Cursor(InCursor)
        , MessageHandler(nullptr)
    {
    }

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

    virtual TSharedRef<FGenericWindow> GetForegroundWindow() const { return nullptr; }

    virtual void GetDisplayInfo(FDisplayInfo& OutDisplayInfo) const { }

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
