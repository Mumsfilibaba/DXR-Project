#pragma once 

#if PLATFORM_WINDOWS
#include "WindowsWindow.h"
#include "WindowsCursor.h"
#include "IWindowsMessageListener.h"

#include "Core/Input/InputCodes.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Threading/Platform/CriticalSection.h"

#include "CoreApplication/Interface/PlatformApplication.h"

#ifdef GetMonitorInfo
#undef GetMonitorInfo
#endif

#define ENABLE_DPI_AWARENESS (0)

///////////////////////////////////////////////////////////////////////////////////////////////////

/* Struct used to store messages between calls to PumpMessages and CWindowsApplication::Tick */
struct SWindowsMessage
{
    FORCEINLINE SWindowsMessage( HWND InWindow, uint32 InMessage, WPARAM InwParam, LPARAM InlParam )
        : Window( InWindow )
        , Message( InMessage )
        , wParam( InwParam )
        , lParam( InlParam )
    {
    }

    // Standard Window Message
    HWND   Window;
    uint32 Message;
    WPARAM wParam;
    LPARAM lParam;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class CWindowsRawInputBuffer
{
public:
    
    CWindowsRawInputBuffer( CWindowsRawInputBuffer&& ) = default;
    CWindowsRawInputBuffer( const CWindowsRawInputBuffer& ) = default;
    CWindowsRawInputBuffer& operator=( CWindowsRawInputBuffer&& ) = default;
    CWindowsRawInputBuffer& operator=( const CWindowsRawInputBuffer& ) = default;
    ~CWindowsRawInputBuffer() = default;

    FORCEINLINE CWindowsRawInputBuffer( uint32 InSize )
        : Buffer( InSize )
    {
    }

    FORCEINLINE RAWINPUT& GetRawInput()
    {
        Assert( !Buffer.Empty() );
        return *reinterpret_cast<RAWINPUT*>( Buffer.Data() ); 
    }

    FORCEINLINE const RAWINPUT& GetRawInput() const 
    {
        Assert( !Buffer.Empty() );
        return *reinterpret_cast<const RAWINPUT*>( Buffer.Data() ); 
    }
   
    FORCEINLINE RAWINPUTHEADER& GetHeader()
    {
        Assert( !Buffer.Empty() );
        return *(reinterpret_cast<RAWINPUT*>( Buffer.Data() )->header);
    }

    FORCEINLINE const RAWINPUTHEADER& GetHeader() const
    {
        Assert( !Buffer.Empty() );
        return *(reinterpret_cast<const RAWINPUT*>( Buffer.Data() )->header);
    }

    FORCEINLINE bool IsMouse() const
    {
        Assert( !Buffer.Empty() );
        return *(reinterpret_cast<const RAWINPUT*>( Buffer.Data() )->header.dwType == RIM_TYPEMOUSE);
    }

    FORCEINLINE bool IsKeyboard() const
    {
        Assert( !Buffer.Empty() );
        return *(reinterpret_cast<const RAWINPUT*>( Buffer.Data() )->header.dwType == RIM_TYPEKEYBOARD);
    }

    FORCEINLINE bool IsHID() const
    {
        Assert( !Buffer.Empty() );
        return *(reinterpret_cast<const RAWINPUT*>( Buffer.Data() )->header.dwType == RIM_TYPEHID);
    }

private:
    TArray<Byte> Buffer;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

/* Class representing an application on the windows- platform */
class COREAPPLICATION_API CWindowsApplication final : public CPlatformApplication
{
    friend class CWindowsApplicationMisc;

public:

    /* Creates an instance of the WindowsApplication, also loads the icon */
    static TSharedPtr<CWindowsApplication> Make();

    /* Retrieve the window-class name */
    static FORCEINLINE LPCSTR GetWindowClassName()
    {
        return "WindowClass";
    }

    /* Returns the HINSTANCE of the application or retrieves it in case the application is not initialized */
    static FORCEINLINE HINSTANCE GetStaticInstance()
    {
        return InstancePtr ? InstancePtr->GetInstance() : static_cast<HINSTANCE>(GetModuleHandle( 0 ));
    }

    /* Returns the instance of the windows application */
    static FORCEINLINE CWindowsApplication* Get()
    {
        return InstancePtr;
    }

    /* Returns true if the WindowsApplication has been created */
    static FORCEINLINE bool IsInitialized()
    {
        return (InstancePtr != nullptr);
    }

    /* Public destructor for TSharedPtr */
    ~CWindowsApplication();

    /* Creates a window */
    virtual TSharedRef<CPlatformWindow> MakeWindow() override final;

    /* Initialized the application */
    virtual bool Initialize() override final;

    /* Tick the application, this handles messages that has been queued up after calls to PumpMessages */
    virtual void Tick( float Delta ) override final;

    /* Returns true if the platform supports Raw mouse movement */
    virtual bool SupportsRawMouse() const;

    /* Enables Raw mouse movement for a certain window */
    virtual bool EnableRawMouse( const TSharedRef<CPlatformWindow>& Window );

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( const TSharedRef<CPlatformWindow>& Window ) override final;

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( const TSharedRef<CPlatformWindow>& Window ) override final;

    /* Retrieves the window that currently has the keyboard focus */
    virtual TSharedRef<CPlatformWindow> GetCapture() const override final;

    /* Retrieves the window that is currently active */
    virtual TSharedRef<CPlatformWindow> GetActiveWindow() const override final;

    /* Searches all the created windows and return the one with the specified handle */
    TSharedRef<CWindowsWindow> GetWindowsWindowFromHWND( HWND Window ) const;

    /* Add a native message listener */
    void AddWindowsMessageListener( const TSharedPtr<IWindowsMessageListener>& NewWindowsMessageListener );

    /* Remove a native message listener */
    void RemoveWindowsMessageListener( const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener );

    /* Check if a native message listener is added */
    bool IsWindowsMessageListener( const TSharedPtr<IWindowsMessageListener>& WindowsMessageListener ) const;

    /* Returns the HINSTANCE of the application */
    FORCEINLINE HINSTANCE GetInstance() const
    {
        return Instance;
    }

private:

    CWindowsApplication( HINSTANCE InInstance );

    /* Registers the window class used to create windows */
    bool RegisterWindowClass();

    /* Registers raw input devices for a specific window */
    bool RegisterRawInputDevices( HWND Window );   

    /* Unregister all raw input devices TODO: Investigate how to do this for a specific window */
    bool UnregisterRawInputDevices();

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

    /* Buffered events, this is done since not all events are fired in the calls to PumpMessages */
    TArray<SWindowsMessage> Messages;

    /* Array storing buffers for RawInput events, stored in order and are pop-ed when the events are processed in Tick*/
    TArray<CWindowsRawInputBuffer> RawInputStack;

    /* In case some message is fired from another thread */
    CCriticalSection MessagesCriticalSection;

    /* buffered events, this is done since not all events are fired in the calls to PumpMessages */
    TArray<TSharedPtr<IWindowsMessageListener>> WindowsMessageListeners;

    /* Checks weather or not the mouse-cursor is tracked, this is for MouseEntered/MouseLeft events */
    bool IsTrackingMouse;

    /* Instance of the application */
    HINSTANCE Instance;

    static CWindowsApplication* InstancePtr;
};

#endif