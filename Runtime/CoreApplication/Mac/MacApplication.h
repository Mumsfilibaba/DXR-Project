#pragma once
#include "MacCursor.h"

#include "Core/Mac/Mac.h"
#include "Core/Containers/Array.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Generic/GenericApplication.h"

#include <AppKit/AppKit.h>

@class FCocoaAppDelegate;
@class CCocoaWindow;

class CMacWindow;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SDeferredMacEvent

struct SDeferredMacEvent
{
    FORCEINLINE SDeferredMacEvent()
        : NotificationName(nil)
        , Event(nil)
        , Window(nil)
		, Size()
		, Position()
		, Character(uint32(-1))
    { }

    FORCEINLINE SDeferredMacEvent(const SDeferredMacEvent& Other)
        : NotificationName(Other.NotificationName ? [Other.NotificationName retain] : nil)
        , Event(Other.Event ? [Other.Event retain] : nil)
        , Window(Other.Window ? [Other.Window retain] : nil)
		, Size(Other.Size)
		, Position(Other.Position)
		, Character(Other.Character)
    { }

    FORCEINLINE ~SDeferredMacEvent()
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
	
	CCocoaWindow*      Window;
	CGSize             Size;
	CGPoint            Position;

	uint32             Character;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMacApplication

class FMacApplication final : public FGenericApplication
{
private:

    FMacApplication();
    ~FMacApplication();

public:

	static FMacApplication* CreateMacApplication();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FGenericApplication Interface

    virtual FGenericWindowRef CreateWindow() override final;

    virtual void Tick(float Delta) override final;

    virtual void SetActiveWindow(const FGenericWindowRef& Window) override final;

    virtual FGenericWindowRef GetWindowUnderCursor() const override final;

    virtual FGenericWindowRef GetActiveWindow() const override final;

public:
    
    TSharedRef<CMacWindow> GetWindowFromNSWindow(NSWindow* Window) const;
    
    void CloseWindow(const TSharedRef<CMacWindow>& Window);
    
    void DeferEvent(NSObject* EventOrNotificationObject);

private:
    void ProcessDeferredEvent(const SDeferredMacEvent& Notification);

    TArray<TSharedRef<CMacWindow>> Windows;
    mutable FCriticalSection       WindowsCS;
    
    TArray<TSharedRef<CMacWindow>> ClosedWindows;
    FCriticalSection               ClosedWindowsCS;

    TArray<SDeferredMacEvent>      DeferredEvents;
    FCriticalSection               DeferredEventsCS;
};

extern FMacApplication* MacApplication;
