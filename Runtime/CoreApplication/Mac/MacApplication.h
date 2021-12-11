#pragma once

#if PLATFORM_MACOS
#include "MacCursor.h"
#include "ScopedAutoreleasePool.h"

#include "Core/Containers/Array.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Interface/PlatformApplication.h"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

@class CCocoaAppDelegate;
@class CCocoaWindow;

class CMacWindow;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

// TODO: Finish
struct SMacApplicationEvent
{
    FORCEINLINE SMacApplicationEvent()
        : NotificationName( nullptr )
        , Event( nullptr )
        , Window( nullptr )
		, Size()
		, Position()
		, Character( uint32(-1) )
    {
    }

    FORCEINLINE SMacApplicationEvent( const SMacApplicationEvent& Other )
        : NotificationName( Other.NotificationName ? [Other.NotificationName retain] : nullptr )
        , Event( Other.Event ? [Other.Event retain] : nullptr )
        , Window( Other.Window ? [Other.Window retain] : nullptr )
		, Size( Other.Size )
		, Position( Other.Position )
		, Character( Other.Character )
    {
    }

    FORCEINLINE ~SMacApplicationEvent()
    {
		SCOPED_AUTORELEASE_POOL();
		
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
	NSNotificationName NotificationName;
    // Event object, nullptr if not an event
    NSEvent* Event;
	
    // Window for the event, nullptr if no associated window exists
	CCocoaWindow* Window;
	// Size of the window
	CGSize Size;
	// Position of the window
	CGPoint Position;
	
	// On Character typed event
	uint32 Character;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

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
    void DeferEvent( NSObject* EventOrNotificationObject );
	
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
    void HandleEvent( const SMacApplicationEvent& Notification );

    /* Delegate to talk with macOS */
    CCocoaAppDelegate* AppDelegate = nullptr;

    /* All the windows of the application */
    TArray<TSharedRef<CMacWindow>> Windows;
    mutable CCriticalSection       WindowsMutex;

    /* Deferred events, events are not processed directly */
    TArray<SMacApplicationEvent> DeferredEvents;
    CCriticalSection             DeferredEventsMutex;

    /* If the application has been terminating or not */
    bool bIsTerminating = false;
};

#endif
