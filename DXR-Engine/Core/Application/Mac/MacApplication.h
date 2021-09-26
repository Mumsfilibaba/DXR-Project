#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Application/Generic/GenericApplication.h"
#include "Core/Application/Mac/MacCursor.h"
#include "Core/Application/Mac/MacKeyboard.h"
#include "Core/Containers/Array.h"

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

/* Mac specific implementation of the application */
class CMacApplication final : public CGenericApplication
{
public:

    /* Public destructor for TSharedPtr */
    ~CMacApplication();

    /* Creates the mac application */
    static FORCEINLINE TSharedPtr<CMacApplication> Make()
    {
        return TSharedPtr<CMacApplication>( new CMacApplication() );
    }

    /* Create a window */
    virtual TSharedRef<CGenericWindow> MakeWindow() override final;

    /* Initialized the application */
    virtual bool Init() override final;

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) override final;

    /* Retrieve the cursor interface */
    virtual ICursor* GetCursor() override final
    {
        return &Cursor;
    }

    /* Retrieve the keyboard interface */
    virtual IKeyboard* GetKeyboard() override final
    {
        return &Keyboard;
    }

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( const TSharedRef<CGenericWindow>& Window ) override final;

    /* Retrieves the window that is currently active */
    virtual TSharedRef<CGenericWindow> GetActiveWindow() const override final;

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
    bool InitAppMenu();

    /* Delegate to talk with macOS */
    CCocoaAppDelegate* AppDelegate = nullptr;

    /* All the windows of the application */
    TArray<TSharedRef<CMacWindow>> Windows;

    /* Cursor interface */
    CMacCursor Cursor;

    /* Keyboard Interface */
    CMacKeyboard Keyboard;

    /* If the application has been terminating or not */
    bool IsTerminating = false;
};

#endif
