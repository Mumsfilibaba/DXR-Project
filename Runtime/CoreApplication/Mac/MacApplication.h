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
        , Size()
        , Position()
        , Character(uint32(-1))
    {
    }

    FORCEINLINE FDeferredMacEvent(const FDeferredMacEvent& Other)
        : NotificationName(Other.NotificationName ? [Other.NotificationName retain] : nil)
        , Event(Other.Event ? [Other.Event retain] : nil)
        , Window(Other.Window ? [Other.Window retain] : nil)
        , Size(Other.Size)
        , Position(Other.Position)
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
    CGSize             Size;
    CGPoint            Position;

    uint32             Character;
};

class FGenericWindow;
class FMacWindow;

class COREAPPLICATION_API FMacApplication final : public FGenericApplication
{
public:
    FMacApplication();
    virtual ~FMacApplication();

    static TSharedPtr<FMacApplication> CreateMacApplication();

    virtual TSharedRef<FGenericWindow> CreateWindow() override final;

    virtual void Tick(float Delta) override final;

    virtual void UpdateGamepadDevices() override final;

    virtual FInputDevice* GetInputDeviceInterface() override final;

    virtual bool SupportsHighPrecisionMouse() const override final;

    virtual bool EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window) override final;

    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) override final;

    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const override final;

    virtual TSharedRef<FGenericWindow> GetActiveWindow() const override final;

    virtual void GetDisplayInfo(FDisplayInfo& OutDisplayInfo) const override final;

    virtual void SetMessageHandler(const TSharedPtr<FGenericApplicationMessageHandler>& InMessageHandler) override final;

public:
    TSharedRef<FMacWindow> GetWindowFromNSWindow(NSWindow* Window) const;
    
    void CloseWindow(const TSharedRef<FMacWindow>& Window);
    
    void DeferEvent(NSObject* EventOrNotificationObject);

    FMacApplicationObserver* GetApplicationObserver() const
    {
        return Observer;
    }

private:
    void ProcessDeferredEvent(const FDeferredMacEvent& Notification);
    
    mutable FDisplayInfo DisplayInfo;
    
    // Observer that checks for monitor changes
    FMacApplicationObserver* Observer;
    
    // InputDevice handling gamepads
    TSharedPtr<FGCInputDevice> InputDevice;
    
    EMouseButtonName::Type LastPressedButton;
    mutable bool           bHasDisplayInfoChanged;
    
    TArray<TSharedRef<FMacWindow>> Windows;
    mutable FCriticalSection WindowsCS;
    
    TArray<TSharedRef<FMacWindow>> ClosedWindows;
    FCriticalSection ClosedWindowsCS;

    TArray<FDeferredMacEvent> DeferredEvents;
    FCriticalSection DeferredEventsCS;
};

extern FMacApplication* MacApplication;
