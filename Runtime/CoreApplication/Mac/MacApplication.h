#pragma once
#include "MacCursor.h"
#include "Core/Mac/Mac.h"
#include "Core/Input/InputCodes.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "CoreApplication/Generic/GenericApplication.h"

#include <AppKit/AppKit.h>

@class FCocoaWindow;
@class FMacApplicationObserver;

class FMacWindow;

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


class COREAPPLICATION_API FMacApplication final : public FGenericApplication
{
    FMacApplication();
    ~FMacApplication();

public:
    static FMacApplication* CreateMacApplication();

    virtual TSharedRef<FGenericWindow> CreateWindow() override final;

    virtual void Tick(float Delta) override final;

    virtual void SetActiveWindow(const TSharedRef<FGenericWindow>& Window) override final;

    virtual TSharedRef<FGenericWindow> GetWindowUnderCursor() const override final;

    virtual TSharedRef<FGenericWindow> GetActiveWindow() const override final;

public:
    TSharedRef<FMacWindow> GetWindowFromNSWindow(NSWindow* Window) const;
    
    void CloseWindow(const TSharedRef<FMacWindow>& Window);
    
    void DeferEvent(NSObject* EventOrNotificationObject);

private:
    void ProcessDeferredEvent(const FDeferredMacEvent& Notification);
    
    TArray<TSharedRef<FMacWindow>> Windows;
    mutable FCriticalSection WindowsCS;
    
    TArray<TSharedRef<FMacWindow>> ClosedWindows;
    FCriticalSection ClosedWindowsCS;

    TArray<FDeferredMacEvent> DeferredEvents;
    FCriticalSection DeferredEventsCS;
    
    FMacApplicationObserver* Observer;
    EMouseButtonName::Type LastPressedButton;
};

extern FMacApplication* MacApplication;
