#pragma once
#include "MacCursor.h"

#include "Core/Mac/Mac.h"
#include "Core/Containers/Array.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Generic/GenericApplication.h"

#include <AppKit/AppKit.h>

@class CCocoaAppDelegate;
@class CCocoaWindow;

class CMacWindow;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SMacApplicationEvent

// TODO: Finish
struct SMacApplicationEvent
{
    FORCEINLINE SMacApplicationEvent()
        : NotificationName(nullptr)
        , Event(nullptr)
        , Window(nullptr)
		, Size()
		, Position()
		, Character(uint32(-1))
    { }

    FORCEINLINE SMacApplicationEvent(const SMacApplicationEvent& Other)
        : NotificationName(Other.NotificationName ? [Other.NotificationName retain] : nullptr)
        , Event(Other.Event ? [Other.Event retain] : nullptr)
        , Window(Other.Window ? [Other.Window retain] : nullptr)
		, Size(Other.Size)
		, Position(Other.Position)
		, Character(Other.Character)
    { }

    FORCEINLINE ~SMacApplicationEvent()
    {
		SCOPED_AUTORELEASE_POOL();
		
        if (NotificationName)
        {
            [NotificationName release];
        }

        if (Event)
        {
            [Event release];
        }

        if (Window)
        {
            [Window release];
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
// CMacApplication

class CMacApplication final : public CGenericApplication
{
private:

    CMacApplication();
    ~CMacApplication();

public:

	static CMacApplication* CreateMacApplication();
    
    TSharedRef<CMacWindow> GetWindowFromNSWindow(NSWindow* Window) const;

    void DeferEvent(NSObject* EventOrNotificationObject);
	
    FORCEINLINE CCocoaAppDelegate* GetAppDelegate() const { return AppDelegate; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericApplication Interface

    virtual TSharedRef<CGenericWindow> MakeWindow() override final;

    virtual bool Initialize()      override final;
    
    virtual void Tick(float Delta) override final;

    virtual void SetActiveWindow(const TSharedRef<CGenericWindow>& Window) override final;

    virtual TSharedRef<CGenericWindow> GetActiveWindow()      const override final;

	virtual TSharedRef<CGenericWindow> GetWindowUnderCursor() const override final;

private:
    bool InitializeAppMenu();

    void HandleEvent(const SMacApplicationEvent& Notification);

    CCocoaAppDelegate*             AppDelegate = nullptr;

    TArray<TSharedRef<CMacWindow>> Windows;
    mutable CCriticalSection       WindowsMutex;

    TArray<SMacApplicationEvent>   DeferredEvents;
    CCriticalSection               DeferredEventsMutex;

    bool                           bIsTerminating = false;
};

extern CMacApplication* MacApplication;
