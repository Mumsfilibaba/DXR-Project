#pragma once
#include "MacCursor.h"
#include "GCInputDevice.h"
#include "Core/Mac/Mac.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/GenericApplication.h"

#include <AppKit/AppKit.h>

@class FCocoaWindow;
@class FMacApplicationObserver;

struct FDeferredMacEvent
{
    FORCEINLINE FDeferredMacEvent()
        : NotificationName(nil)
        , Event(nil)
        , Window(nil)
        , Character(uint32(-1))
    {
    }

    FORCEINLINE FDeferredMacEvent(const FDeferredMacEvent& Other)
        : NotificationName(Other.NotificationName ? [Other.NotificationName retain] : nil)
        , Event(Other.Event ? [Other.Event retain] : nil)
        , Window(Other.Window ? [Other.Window retain] : nil)
        , Character(Other.Character)
    {
    }

    FORCEINLINE ~FDeferredMacEvent()
    {
        @autoreleasepool
        {
            NSSafeRelease(NotificationName);
            NSSafeRelease(Event);
            NSSafeRelease(Window);
        }
    }

    NSNotificationName NotificationName;
    NSEvent*           Event;
    FCocoaWindow*      Window;
    uint32             Character;
};

class FGenericWindow;
class FMacWindow;

class COREAPPLICATION_API FMacApplication final : public FGenericApplication
{
public:
    static FString GetMonitorNameFromNSScreen(NSScreen* Screen);
    static uint32  GetDPIFromNSScreen(NSScreen* Screen);
    static NSPoint GetCorrectedMouseLocation();
    
    static TSharedPtr<FMacApplication>     CreateMacApplication();
    static TSharedPtr<FGenericApplication> Create();
    
    FMacApplication();
    virtual ~FMacApplication();

    virtual TSharedRef<FGenericWindow> CreateWindow() override final;
    virtual void Tick(float Delta) override final;
    virtual void UpdateInputDevices() override final;
    virtual FInputDevice* GetInputDeviceInterface() override final;
    virtual bool SupportsHighPrecisionMouse() const override final;
    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) override final;
    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) override final;
    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const override final;
    virtual TSharedRef<FGenericWindow> GetActiveWindow() const override final;
    virtual void QueryDisplayInfo(FDisplayInfo& OutDisplayInfo) const override final;
    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler) override final;

    void ProcessDeferredEvent(const FDeferredMacEvent& DeferredEvent);
    void OnMouseMoveEvent();
    void OnMouseButtonEvent(const FDeferredMacEvent& DeferredEvent);
    void OnMouseScrollEvent(const FDeferredMacEvent& DeferredEvent);
    void OnKeyEvent(const FDeferredMacEvent& DeferredEvent);
    void OnWindowResized(const FDeferredMacEvent& DeferredEvent);
    void OnWindowMoved(const FDeferredMacEvent& DeferredEvent);
    
    TSharedRef<FMacWindow> GetWindowFromNSWindow(NSWindow* Window) const;
    void CloseWindow(const TSharedRef<FMacWindow>& Window);
    void DeferEvent(NSObject* EventOrNotificationObject);

    FMacApplicationObserver* GetApplicationObserver() const
    {
        return Observer;
    }

private:
    
    // Observer that checks for monitor changes
    FMacApplicationObserver*       Observer;

    // We need to store this modifier-flag in order to handle modifier keys correctly
    NSUInteger                     PreviousModifierFlags;

    // InputDevice handling gamepads
    TSharedPtr<FGCInputDevice>     InputDevice;
    EMouseButtonName::Type         LastPressedButton;

    TArray<TSharedRef<FMacWindow>> Windows;
    mutable FCriticalSection       WindowsCS;
    TArray<TSharedRef<FMacWindow>> ClosedWindows;
    FCriticalSection               ClosedWindowsCS;
    TArray<FDeferredMacEvent>      DeferredEvents;
    FCriticalSection               DeferredEventsCS;
};

extern FMacApplication* MacApplication;
