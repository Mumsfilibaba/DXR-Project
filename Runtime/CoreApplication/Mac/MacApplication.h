#pragma once

#if PLATFORM_MACOS
#include "MacCursor.h"

#include "Core/Containers/Array.h"

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
    {
    }

    FORCEINLINE SMacApplicationEvent( const SMacApplicationEvent& Other )
    {
    }

    FORCEINLINE ~SMacApplicationEvent()
    {
    }
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

    /* Handles a notification */
    void HandleNotification( const struct SNotification& Notification );

    /* Handles an event */
    void HandleEvent( NSEvent* Event );

    /* Handle key typed event */
    void HandleKeyTypedEvent( NSString* Text );

    /* Returs the native appdelegate */
    FORCEINLINE CCocoaAppDelegate* GetAppDelegate() const
    {
        return AppDelegate;
    }

private:

    CMacApplication();

    /* Initializes the applications menu in the menubar */
    bool InitializeAppMenu();

    /* Delegate to talk with macOS */
    CCocoaAppDelegate* AppDelegate = nullptr;

    /* All the windows of the application */
    TArray<TSharedRef<CMacWindow>> Windows;

    /* If the application has been terminating or not */
    bool IsTerminating = false;
};

#endif
