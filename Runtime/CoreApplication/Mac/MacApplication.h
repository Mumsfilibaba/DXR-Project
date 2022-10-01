#pragma once
#include "MacCursor.h"

#include "Core/Mac/Mac.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"

#include "CoreApplication/Generic/GenericApplication.h"

#include <AppKit/AppKit.h>

@class FCocoaAppDelegate;
@class FCocoaWindow;

class FMacWindow;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDeferredMacEvent

struct FDeferredMacEvent
{
    FORCEINLINE FDeferredMacEvent()
        : NotificationName(nil)
        , Event(nil)
        , Window(nil)
		, Size()
		, Position()
		, Character(uint32(-1))
    { }

    FORCEINLINE FDeferredMacEvent(const FDeferredMacEvent& Other)
        : NotificationName(Other.NotificationName ? [Other.NotificationName retain] : nil)
        , Event(Other.Event ? [Other.Event retain] : nil)
        , Window(Other.Window ? [Other.Window retain] : nil)
		, Size(Other.Size)
		, Position(Other.Position)
		, Character(Other.Character)
    { }

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacApplication

class COREAPPLICATION_API FMacApplication final
    : public FGenericApplication
{
public:
    FMacApplication();
    ~FMacApplication();

    virtual FGenericWindowRef CreateWindow() override final;

    virtual void Tick(float Delta) override final;

    virtual void SetActiveWindow(const FGenericWindowRef& Window) override final;

    virtual FGenericWindowRef GetWindowUnderCursor() const override final;
    virtual FGenericWindowRef GetActiveWindow()      const override final;

public:
    TSharedRef<FMacWindow> GetWindowFromNSWindow(NSWindow* Window) const;
    
    void CloseWindow(const TSharedRef<FMacWindow>& Window);
    
    void DeferEvent(NSObject* EventOrNotificationObject);

private:
    void ProcessDeferredEvent(const FDeferredMacEvent& Notification);

    TArray<TSharedRef<FMacWindow>> Windows;
    mutable FCriticalSection       WindowsCS;
    
    TArray<TSharedRef<FMacWindow>> ClosedWindows;
    FCriticalSection               ClosedWindowsCS;

    TArray<FDeferredMacEvent>      DeferredEvents;
    FCriticalSection               DeferredEventsCS;
};

extern FMacApplication* MacApplication;
