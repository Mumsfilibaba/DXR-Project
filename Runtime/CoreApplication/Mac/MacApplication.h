#pragma once

#if PLATFORM_MACOS
#include "MacCursor.h"

#include "Core/Containers/Array.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Interface/PlatformApplication.h"

#if defined(__OBJC__)
@class NSNotification;
@class NSWindow;
@class NSEvent;
@class NSString;
@class CCocoaAppDelegate;
@class CCocoaWindow;
#else
class NSNotification;
class NSWindow;
class NSEvent;
class NSString;
class CCocoaAppDelegate;
class CCocoaWindow;
#endif

class CMacWindow;

///////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: Finish
struct SMacApplicationEvent
{
    FORCEINLINE SMacApplicationEvent()
        : NotificationName( nullptr )
        , Event( nullptr )
        , Window( nullptr )
    {
    }

    FORCEINLINE SMacApplicationEvent( const SMacApplicationEvent& Other )
        : NotificationName( Other.NotificationName ? [Other.NotificationName retain] : nullptr )
        , Event( Other.Event ? [Other.Event retain] : nullptr )
        , Window( Other.Window ? [Other.Window retain] : nullptr )
    {
    }

    FORCEINLINE ~SMacApplicationEvent()
    {
        if ( NotificationName )
        {
            [NotificationName release];
        }

        if ( Event )
        {
            [Event release];
        }

        if ( Window )
        {
            [Window release];
        }
    }

    // Name of notification, nullptr if not a notification
    NSNotificationName* NotificationName;
    // Event object, nullptr if not an event
    NSEvent* Event;
    // Window for the event, nullptr if no associated window exists
    NSWindow* Window;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

/* Mac specific implementation of the application */
class CMacApplication final : public CPlatformApplication
{
public:

    /* Creates the mac application */
	static TSharedPtr<CMacApplication> Make();

    /* Public destructor for TSharedPtr */
    ~CMacApplication();

    /* Create a window */
    virtual TSharedRef<CPlatformWindow> MakeWindow() override final;

    /* Initialized the application */
    virtual bool Initialize() override final;

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) override final;

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( const TSharedRef<CPlatformWindow>& Window ) override final;

    /* Retrieves the window that is currently active */
    virtual TSharedRef<CPlatformWindow> GetActiveWindow() const override final;

	/* Retrieve the window that is currently under the cursor, if no window is under the cursor, the value is nullptr */
	virtual TSharedRef<CPlatformWindow> GetWindowUnderCursor() const override final;
	
    /* Retrieves a from a NSWindow */
    TSharedRef<CMacWindow> GetWindowFromNSWindow( NSWindow* Window ) const;

    /* Store event for handling later in the main loop */
    void StoreEvent( NSObject* EventOrNotificationObject );

    /* Returs the native appdelegate */
    FORCEINLINE CCocoaAppDelegate* GetAppDelegate() const
    {
        return AppDelegate;
    }

private:

    CMacApplication();

    /* Initializes the applications menu in the menubar */
    bool InitializeAppMenu();

    /* Handles a notification */
    void HandleNotification( NSNotification* Notification );

    /* Handles an event */
    void HandleEvent( NSEvent* Event );

    /* Handle key typed event */
    void HandleKeyTypedEvent( NSString* Text );

    /* Delegate to talk with macOS */
    CCocoaAppDelegate* AppDelegate = nullptr;

    /* All the windows of the application */
    TArray<TSharedRef<CMacWindow>> Windows;
    CCriticalSection               WindowsMutex;

    /* Deferred events, events are not processed directly */
    TArray<SMacApplicationEvent> DeferredEvents;
    CCriticalSection             DeferredEventsMutex;

    /* If the application has been terminating or not */
    bool IsTerminating = false;
};

#endif
