#pragma once 

#if defined(PLATFORM_WINDOWS)
#include "Core/Input/InputCodes.h"
#include "Core/Application/Generic/GenericApplicationMessageListener.h"
#include "Core/Application/Generic/GenericApplication.h"
#include "Core/Containers/Array.h"

#include "Core/Threading/Platform/CriticalSection.h"

#include "WindowsWindow.h"
#include "IWindowsMessageListener.h"
#include "WindowsCursor.h"
#include "WindowsKeyboard.h"

/* Strict used to store messages between calls to PumpMessages and CWindowsApplication::Tick */
struct SWindowsMessage
{
    FORCEINLINE SWindowsMessage( HWND InWindow, uint32 InMessage, WPARAM InwParam, LPARAM InlParam )
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

/* Pointer to the windows-application needed in the static */
extern class CWindowsApplication* GWindowsApplication;

/* Class representing an application on the windows- platform */
class CWindowsApplication final : public CGenericApplication
{
    friend class CWindowsApplicationMisc;

public:

    /* Creates an instance of the WindowsApplication, also loads the icon */
    static CWindowsApplication* Make();

    /* Creates a window */
    virtual CGenericWindow* MakeWindow() override final;

    /* Initialized the application */
    virtual bool Init() override final;

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) override final;

    /* Releases the application */
    virtual void Release() override final;

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

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( CGenericWindow* Window ) override final;

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( CGenericWindow* Window ) override final;

    /* Retrieves the window that currently has the keyboard focus */
    virtual CGenericWindow* GetCapture() const override final;

    /* Retrieves the window that is currently active */
    virtual CGenericWindow* GetActiveWindow() const override final;

    /* Searches all the created windows and return the one with the specified handle */
    CWindowsWindow* GetWindowsWindowFromHWND( HWND Window ) const;

    /* Add a native message listener */
    void AddWindowsMessageListener( IWindowsMessageListener* NewWindowsMessageListener );

    /* Remove a native message listener */
    void RemoveWindowsMessageListener( IWindowsMessageListener* WindowsMessageListener );

    /* Check if a native message listener is added */
    bool IsWindowsMessageListener( IWindowsMessageListener* WindowsMessageListener ) const;

    /* Returns the HINSTANCE of the application */
    FORCEINLINE HINSTANCE GetInstance() const
    {
        return Instance;
    }

    /* Retrieve the window-class name */
    static FORCEINLINE LPCSTR GetWindowClassName()
    {
        return "WindowClass";
    }

    /* Returns the HINSTANCE of the application or retrieves it in case the application is not initialized */
    static FORCEINLINE HINSTANCE GetInstanceStatic()
    {
        return GWindowsApplication ? GWindowsApplication->GetInstance() : static_cast<HINSTANCE>(GetModuleHandle( 0 ));
    }

private:

    CWindowsApplication( HINSTANCE InInstance );
    ~CWindowsApplication();

    /* Registers the window class used to create windows */
    bool RegisterWindowClass();

    /* Stores messages for handling in the future */
    void StoreMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam );

    /* Message-proc which handles the messages for the instance */
    LRESULT MessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam );

    /* Static message-proc sent into registerwindowclass */
    static LRESULT StaticMessageProc( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam );

    /* Handles stored messages in Tick */
    void HandleStoredMessage( HWND Window, UINT Message, WPARAM wParam, LPARAM lParam );

    /* The windows that has been created by the application */
    TArray<TSharedRef<CWindowsWindow>> Windows;

    /* buffered events, this is done since not all events are fired in the calls to PumpMessages */
    TArray<SWindowsMessage> Messages;

    /* In case some message is fired from another thread */
    CCriticalSection MessagesCriticalSection;

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

#endif