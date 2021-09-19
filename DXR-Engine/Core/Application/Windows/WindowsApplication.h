#pragma once 

#if defined(PLATFORM_WINDOWS)
#include "Core/Input/InputCodes.h"
#include "Core/Application/Generic/GenericApplicationMessageListener.h"
#include "Core/Application/Generic/GenericApplication.h"
#include "Core/Containers/Array.h"

#include "WindowsWindow.h"
#include "IWindowsMessageListener.h"

class CWindowsCursor;

struct SWindowsEvent
{
    SWindowsEvent( HWND InWindow, uint32 InMessage, WPARAM InwParam, LPARAM InlParam )
        : Window( InWindow )
        , Message( InMessage )
        , wParam( InwParam )
        , lParam( InlParam )
    {
    }

    HWND   Window;
    uint32 Message;
    WPARAM wParam;
    LPARAM lParam;
};

class CWindowsApplication final : public CGenericApplication
{
public:

    /* Creates an instance of the WindowsApplication, also loads the icon */
    static FORCEINLINE CWindowsApplication* Make()
    {
        HINSTANCE Instance = (HINSTANCE)GetModuleHandleA( 0 );
        // TODO: Load icon here
        return new CWindowsApplication( Instance );
    }

    /* Creates a window */
    virtual CGenericWindow* MakeWindow() override final;

    /* Initialized the application */
    virtual bool Init() override final;

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) override final;

    /* Releases the application */
    virtual void Release() override final;

    /* Retrieve the cursor interface */
    virtual ICursor* GetCursor() override final;

    /* Retrieve the keyboard interface */
    virtual IKeyboard* GetKeyboard() override final;

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( CGenericWindow* ) override final;

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( CGenericWindow* Window ) override final;

    /* Retrieves the window that currently has the keyboard focus */
    virtual CGenericWindow* GetCapture() const override final;

    /* Retrieves the window that is currently active */
    virtual CGenericWindow* GetActiveWindow() const override final;

    /* Add a native message listener */
    void AddWindowsMessageListener( IWindowsMessageListener* NewWindowsMessageListener );
    
    /* Remove a native message listener */
    void RemoveWindowsMessageListener( IWindowsMessageListener* WindowsMessageListener );

    /* Check if a native message listener is added */
    bool IsWindowsMessageListener( IWindowsMessageListener* WindowsMessageListener ) const;

    /* Retrieve the window-class name */
    static LPCSTR GetWindowClassName()
    {
        return "WindowClass";
    }

    /* Returns the HINSTANCE of the application */
    static HINSTANCE GetInstance();

private:

    CWindowsApplication( HINSTANCE InInstance );
    ~CWindowsApplication();

    static bool RegisterWindowClass();

    /* Stores messages for handling in the future */
    void StoreMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam );

    /* Static message proc sent into registerwindowclass */
    static LRESULT MessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam );
    
    /* Handles stored messages in Tick */
    void HandleStoredMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam );

    /* The windows that has been created by the application */
    TArray<TSharedRef<CWindowsWindow>> Windows;

    /* buffered events, this is done since not all events are fired in the calls to PumpMessages */
    TArray<SWindowsEvent> Messages;

    /* buffered events, this is done since not all events are fired in the calls to PumpMessages */
    TArray<IWindowsMessageListener*> WindowsMessageListeners;

    /* Cursor interface */
    CWindowsCursor Cursor;

    /* Cursor interface */
    CWindowsKeyboard Keyboard;

    /* Checks weather or not the mouse-cursor is tracked, this is for MouseEntered/MouseLeft events */
    bool IsTrackingMouse;

    /* Instance of the application */
    HINSTANCE Instance;
};

/* Pointer to the windowsapplication */
extern CWindowsApplication* GWindowsApplication;

#endif